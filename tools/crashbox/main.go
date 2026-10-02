// crashbox: a crash report inbox for games built on Cute Framework's cute_crash.h.
//
// Reports arrive as one multipart POST and are kept as files; an in-memory index built from
// those files groups them by signature for three pages: the groups, a group, a report.
// Everything a stranger can reach is bounded: body size, rate per IP and per install,
// total disk. Standard library only.
package main

import (
	"bufio"
	"crypto/tls"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"log"
	"mime/multipart"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"
)

type config struct {
	Addr              string
	TLS               string
	Data              string
	Token             string
	User              string
	Password          string
	MaxReportKB       int64
	MaxDumpMB         int64
	DiskMB            int64
	RateIPPerMin      int
	RateInstallPerDay int
	MaxAgeDays        int64 // Reports older than this are deleted; 0 keeps them until the disk budget needs the room.
}

func defaultConfig() config {
	return config{
		Addr:              ":8445",
		TLS:               "/opt/crashbox/tls",
		Data:              "./data",
		MaxReportKB:       512,
		MaxDumpMB:         8,
		DiskMB:            2048,
		RateIPPerMin:      10,
		RateInstallPerDay: 60,
	}
}

// key = value, one per line, # comments. Unknown keys are an error so a typo is not a silent default.
func loadConfig(path string) (config, error) {
	c := defaultConfig()
	f, err := os.Open(path)
	if err != nil {
		return c, err
	}
	defer f.Close()
	sc := bufio.NewScanner(f)
	for n := 1; sc.Scan(); n++ {
		line := strings.TrimSpace(sc.Text())
		if i := strings.IndexByte(line, '#'); i >= 0 {
			line = strings.TrimSpace(line[:i])
		}
		if line == "" {
			continue
		}
		k, v, ok := strings.Cut(line, "=")
		if !ok {
			return c, fmt.Errorf("%s:%d: expected key = value", path, n)
		}
		k, v = strings.TrimSpace(k), strings.TrimSpace(v)
		if err := c.set(k, v); err != nil {
			return c, fmt.Errorf("%s:%d: %w", path, n, err)
		}
	}
	return c, sc.Err()
}

func (c *config) set(k, v string) error {
	num := func(dst *int64) error {
		n, err := strconv.ParseInt(v, 10, 64)
		if err != nil || n < 0 {
			return fmt.Errorf("%s: not a number: %q", k, v)
		}
		*dst = n
		return nil
	}
	switch k {
	case "addr":
		c.Addr = v
	case "tls":
		c.TLS = v
	case "data":
		c.Data = v
	case "token":
		c.Token = v
	case "user":
		c.User = v
	case "password":
		c.Password = v
	case "max_report_kb":
		return num(&c.MaxReportKB)
	case "max_dump_mb":
		return num(&c.MaxDumpMB)
	case "disk_mb":
		return num(&c.DiskMB)
	case "max_age_days":
		return num(&c.MaxAgeDays)
	case "rate_ip_per_min":
		var n int64
		if err := num(&n); err != nil {
			return err
		}
		c.RateIPPerMin = int(n)
	case "rate_install_per_day":
		var n int64
		if err := num(&n); err != nil {
			return err
		}
		c.RateInstallPerDay = int(n)
	default:
		return fmt.Errorf("unknown key %q", k)
	}
	return nil
}

type server struct {
	cfg   config
	disk  int64 // the budget in bytes; tests shrink it
	mu    sync.Mutex
	idx   *index
	ips   *limiter
	insts *limiter
	now   func() time.Time
}

func newServer(cfg config) (*server, error) {
	if err := os.MkdirAll(filepath.Join(cfg.Data, "reports"), 0o755); err != nil {
		return nil, err
	}
	s := &server{cfg: cfg, disk: cfg.DiskMB << 20, now: time.Now}
	s.ips = newLimiter(cfg.RateIPPerMin, time.Minute)
	s.insts = newLimiter(cfg.RateInstallPerDay, 24*time.Hour)
	idx, err := loadIndex(filepath.Join(cfg.Data, "reports"))
	if err != nil {
		return nil, err
	}
	s.idx = idx
	return s, nil
}

func (s *server) handler() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("POST /v1/crash", s.ingest)
	mux.HandleFunc("GET /{$}", s.auth(s.groupsPage))
	mux.HandleFunc("GET /group/{key}", s.auth(s.groupPage))
	mux.HandleFunc("POST /group/{key}/delete", s.auth(s.groupDelete))
	mux.HandleFunc("POST /report/{id}/delete", s.auth(s.reportDelete))
	mux.HandleFunc("GET /report/{id}", s.auth(s.reportPage))
	return mux
}

func (s *server) auth(h http.HandlerFunc) http.HandlerFunc {
	if s.cfg.User == "" && s.cfg.Password == "" {
		return h
	}
	return func(w http.ResponseWriter, r *http.Request) {
		u, p, ok := r.BasicAuth()
		if !ok || u != s.cfg.User || p != s.cfg.Password {
			w.Header().Set("WWW-Authenticate", `Basic realm="crashbox"`)
			http.Error(w, "unauthorized", http.StatusUnauthorized)
			return
		}
		h(w, r)
	}
}

var errTooLarge = errors.New("too large")

// Reads one multipart part into memory, failing past `limit` bytes without reading the rest.
func readPart(p *multipart.Part, limit int64) ([]byte, error) {
	b, err := io.ReadAll(io.LimitReader(p, limit+1))
	if err != nil {
		return nil, err
	}
	if int64(len(b)) > limit {
		return nil, errTooLarge
	}
	return b, nil
}

func clientIP(r *http.Request) string {
	host, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		return r.RemoteAddr
	}
	return host
}

func (s *server) reject(w http.ResponseWriter, r *http.Request, code int, why string) {
	log.Printf("reject %s %s: %s", clientIP(r), r.URL.Path, why)
	http.Error(w, why, code)
}

func (s *server) ingest(w http.ResponseWriter, r *http.Request) {
	if s.cfg.Token != "" && r.Header.Get("Authorization") != "Bearer "+s.cfg.Token {
		got := r.Header.Get("Authorization")
		if len(got) > 16 {
			got = got[:16] + "..."
		}
		s.reject(w, r, http.StatusUnauthorized, "bad token (got "+strconv.Quote(got)+", "+strconv.Itoa(len(r.Header.Get("Authorization")))+" bytes)")
		return
	}
	maxReport := s.cfg.MaxReportKB << 10
	maxDump := s.cfg.MaxDumpMB << 20
	r.Body = http.MaxBytesReader(w, r.Body, maxReport+maxDump+64<<10)
	mr, err := r.MultipartReader()
	if err != nil {
		s.reject(w, r, http.StatusBadRequest, "not multipart")
		return
	}
	var reportBytes, dumpBytes []byte
	for {
		p, err := mr.NextPart()
		if err == io.EOF {
			break
		}
		if err != nil {
			var mbe *http.MaxBytesError
			if errors.As(err, &mbe) {
				s.reject(w, r, http.StatusRequestEntityTooLarge, "body too large")
			} else {
				s.reject(w, r, http.StatusBadRequest, "bad multipart")
			}
			return
		}
		var dst *[]byte
		var limit int64
		switch p.FormName() {
		case "report":
			dst, limit = &reportBytes, maxReport
		case "crash.dmp":
			dst, limit = &dumpBytes, maxDump
		default:
			io.Copy(io.Discard, p)
			continue
		}
		b, err := readPart(p, limit)
		if errors.Is(err, errTooLarge) {
			s.reject(w, r, http.StatusRequestEntityTooLarge, p.FormName()+" too large")
			return
		}
		if err != nil {
			var mbe *http.MaxBytesError
			if errors.As(err, &mbe) {
				s.reject(w, r, http.StatusRequestEntityTooLarge, "body too large")
			} else {
				s.reject(w, r, http.StatusBadRequest, "bad part")
			}
			return
		}
		*dst = b
	}
	if reportBytes == nil {
		s.reject(w, r, http.StatusBadRequest, "no report part")
		return
	}
	rep, err := parseReport(reportBytes)
	if err != nil {
		s.reject(w, r, http.StatusBadRequest, "bad report: "+err.Error())
		return
	}

	ip := clientIP(r)
	now := s.now()
	s.mu.Lock()
	defer s.mu.Unlock()
	if !s.ips.allow(ip, now) || (rep.Install != "" && !s.insts.allow(rep.Install, now)) {
		log.Printf("drop %s: rate limit (%s)", rep.ID, ip)
		w.WriteHeader(http.StatusOK)
		return
	}
	if s.idx.get(rep.ID) != nil {
		w.WriteHeader(http.StatusOK)
		return
	}
	need := int64(len(reportBytes) + len(dumpBytes))
	if !s.makeRoom(need) {
		log.Printf("drop %s: disk budget", rep.ID)
		w.WriteHeader(http.StatusOK)
		return
	}
	e, err := s.idx.store(s.reportsDir(), rep, reportBytes, dumpBytes, now)
	if err != nil {
		log.Printf("store %s: %v", rep.ID, err)
		http.Error(w, "store failed", http.StatusInternalServerError)
		return
	}
	log.Printf("report %s %s %q from %s", e.ID, e.Kind, e.Title, ip)
	w.WriteHeader(http.StatusOK)
}

func (s *server) reportsDir() string { return filepath.Join(s.cfg.Data, "reports") }

// Deletes every report received more than max_age_days ago. Run at startup and hourly.
func (s *server) expire() int {
	if s.cfg.MaxAgeDays <= 0 {
		return 0
	}
	cutoff := s.now().Add(-time.Duration(s.cfg.MaxAgeDays) * 24 * time.Hour)
	s.mu.Lock()
	defer s.mu.Unlock()
	var old []*entry
	for _, e := range s.idx.reports {
		if e.Received.Before(cutoff) {
			old = append(old, e)
		}
	}
	for _, e := range old {
		s.idx.remove(s.reportsDir(), e)
	}
	if len(old) > 0 {
		log.Printf("expire: %d reports older than %d days", len(old), s.cfg.MaxAgeDays)
	}
	return len(old)
}

// Evicts until `need` more bytes fit under the budget: dumps first, oldest first, then the oldest
// reports of the largest groups. The newest report of every group stays, dump included.
// Returns false when even that leaves no room.
func (s *server) makeRoom(need int64) bool {
	budget := s.disk
	for s.idx.bytes+need > budget {
		victim := s.idx.oldestEvictableDump()
		if victim == nil {
			break
		}
		s.idx.dropDump(s.reportsDir(), victim)
	}
	for s.idx.bytes+need > budget {
		victim := s.idx.oldestOfLargestGroup()
		if victim == nil {
			break
		}
		s.idx.remove(s.reportsDir(), victim)
	}
	return s.idx.bytes+need <= budget
}

// The certificate pair re-read when either file's mtime changes, so a renewed certificate lands
// without a restart.
func tlsConfig(dir string) (*tls.Config, error) {
	crt, key := filepath.Join(dir, "tls.crt"), filepath.Join(dir, "tls.key")
	var mu sync.Mutex
	var cert *tls.Certificate
	var stamp time.Time
	load := func() (*tls.Certificate, error) {
		mu.Lock()
		defer mu.Unlock()
		ci, err := os.Stat(crt)
		if err != nil {
			return cert, err
		}
		ki, err := os.Stat(key)
		if err != nil {
			return cert, err
		}
		newest := ci.ModTime()
		if ki.ModTime().After(newest) {
			newest = ki.ModTime()
		}
		if cert != nil && !newest.After(stamp) {
			return cert, nil
		}
		c, err := tls.LoadX509KeyPair(crt, key)
		if err != nil {
			return cert, err
		}
		cert, stamp = &c, newest
		return cert, nil
	}
	if _, err := load(); err != nil {
		return nil, err
	}
	return &tls.Config{
		MinVersion: tls.VersionTLS12,
		GetCertificate: func(*tls.ClientHelloInfo) (*tls.Certificate, error) {
			c, err := load()
			if c == nil {
				return nil, err
			}
			return c, nil
		},
	}, nil
}

func main() {
	path := flag.String("config", "crashbox.conf", "config file (key = value)")
	flag.Parse()
	cfg, err := loadConfig(*path)
	if err != nil && !errors.Is(err, os.ErrNotExist) {
		log.Fatal(err)
	}
	s, err := newServer(cfg)
	if err != nil {
		log.Fatal(err)
	}
	srv := &http.Server{
		Addr:              cfg.Addr,
		Handler:           s.handler(),
		ReadHeaderTimeout: 10 * time.Second,
		ReadTimeout:       60 * time.Second,
		WriteTimeout:      60 * time.Second,
	}
	s.expire()
	go func() {
		for range time.Tick(time.Hour) {
			s.expire()
		}
	}()
	log.Printf("crashbox: %d reports in %d groups, data %s, listening %s", len(s.idx.reports), len(s.idx.groups), cfg.Data, cfg.Addr)
	if cfg.TLS != "" {
		tc, err := tlsConfig(cfg.TLS)
		if err != nil {
			log.Fatalf("tls: %v", err)
		}
		srv.TLSConfig = tc
		log.Fatal(srv.ListenAndServeTLS("", ""))
	}
	log.Fatal(srv.ListenAndServe())
}

// Bytes as JSON for the raw view, indented once so a browser shows it readably.
func prettyJSON(b []byte) []byte {
	var v any
	if err := json.Unmarshal(b, &v); err != nil {
		return b
	}
	out, err := json.MarshalIndent(v, "", "  ")
	if err != nil {
		return b
	}
	return out
}
