package main

import (
	"bytes"
	"fmt"
	"io"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

const resolvedReport = `{
  "format": 1, "id": "%s", "kind": "crash", "app": "Of Eld", "version": "0.3.1", "build": "9a694c5",
  "time": "2026-10-01T22:14:07Z", "uptime": 271.3, "install": "%s",
  "machine": { "os": "Windows 11 10.0.26200", "arch": "x86_64", "cpu": "AMD Ryzen 7 7800X3D", "threads": 16, "ram_mb": 32768 },
  "fault": { "exception": "EXCEPTION_ACCESS_VIOLATION", "code": "0xC0000005", "access": "write", "address": "0x18", "thread": 19604, "thread_name": "main" },
  "modules": [
    { "name": "oe.exe", "path": "C:\\games\\oe.exe", "base": "0x7ff6b2a40000", "size": 2031616, "build_id": "8c2b0f314d2a4f0b9e510c7a3b1d9f2201", "symbols": "embedded" },
    { "name": "ntdll.dll", "base": "0x7ffd0e5c0000", "size": 2166784, "build_id": "aa", "system": true } ],
  "stack": [
    { "module": 1, "offset": 100, "pc": "0x7ffd0e5c0064" },
    { "module": 0, "offset": 77642, "pc": "0x7ff6b2a52f4a", "function": "s_strike", "file": "scripts/shellback.cpp", "line": 118,
      "inlined": [ { "function": "s_bite", "file": "scripts/shellback.cpp", "line": 90 } ] },
    { "module": 0, "offset": 7126, "pc": "0x7ff6b2a41bd6", "function": "main", "file": "src/main.cpp", "line": 512 } ],
  "signature": { "raw": "4e1a", "symbolic": "b77c" },
  "state": { "map": "arena", "seed": "7", "turn": "1412" },
  "breadcrumbs": [ { "t": 270.12, "msg": "script: gen 3 live" }, { "t": 271.30, "msg": "turn 1411" } ],
  "attachments": [ { "name": "crash.dmp", "type": "minidump", "bytes": 5 } ]
}`

const rawReport = `{
  "format": 1, "id": "%s", "kind": "crash", "app": "Of Eld", "version": "0.2.9", "install": "%s",
  "machine": { "os": "macOS 15.1", "arch": "aarch64" },
  "fault": { "signal": "SIGSEGV", "code": "SEGV_MAPERR", "address": "0x18", "thread": 1, "thread_name": "main" },
  "modules": [ { "name": "oe", "base": "0x104a3c000", "size": 3000000, "build_id": "7b1e5c02" } ],
  "stack": [ { "module": 0, "offset": 244181, "pc": "0x104a77ad5" } ],
  "signature": { "raw": "0ff1" },
  "state": { "map": "arena" }
}`

func newTest(t *testing.T, mutate func(*config)) (*server, *httptest.Server) {
	t.Helper()
	cfg := defaultConfig()
	cfg.TLS = ""
	cfg.Data = t.TempDir()
	if mutate != nil {
		mutate(&cfg)
	}
	s, err := newServer(cfg)
	if err != nil {
		t.Fatal(err)
	}
	ts := httptest.NewServer(s.handler())
	t.Cleanup(ts.Close)
	return s, ts
}

func post(t *testing.T, ts *httptest.Server, token string, reportJSON string, dump []byte) *http.Response {
	t.Helper()
	var body bytes.Buffer
	mw := multipart.NewWriter(&body)
	if reportJSON != "" {
		w, _ := mw.CreateFormFile("report", "crash.json")
		w.Write([]byte(reportJSON))
	}
	if dump != nil {
		w, _ := mw.CreateFormFile("crash.dmp", "crash.dmp")
		w.Write(dump)
	}
	mw.Close()
	req, _ := http.NewRequest("POST", ts.URL+"/v1/crash", &body)
	req.Header.Set("Content-Type", mw.FormDataContentType())
	if token != "" {
		req.Header.Set("Authorization", "Bearer "+token)
	}
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	resp.Body.Close()
	return resp
}

func get(t *testing.T, ts *httptest.Server, path string) (int, string, http.Header) {
	t.Helper()
	resp, err := http.Get(ts.URL + path)
	if err != nil {
		t.Fatal(err)
	}
	b, _ := io.ReadAll(resp.Body)
	resp.Body.Close()
	return resp.StatusCode, string(b), resp.Header
}

func TestIngestAndPages(t *testing.T) {
	s, ts := newTest(t, nil)
	if r := post(t, ts, "", fmt.Sprintf(resolvedReport, "r1", "inst-a"), []byte("hello")); r.StatusCode != 200 {
		t.Fatalf("resolved: %d", r.StatusCode)
	}
	if r := post(t, ts, "", fmt.Sprintf(resolvedReport, "r2", "inst-a"), nil); r.StatusCode != 200 {
		t.Fatalf("resolved again: %d", r.StatusCode)
	}
	if r := post(t, ts, "", fmt.Sprintf(rawReport, "x1", "inst-b"), nil); r.StatusCode != 200 {
		t.Fatalf("raw: %d", r.StatusCode)
	}
	for _, name := range []string{"r1.json", "r1.dmp", "r2.json", "x1.json"} {
		if _, err := os.Stat(filepath.Join(s.reportsDir(), name)); err != nil {
			t.Errorf("missing %s", name)
		}
	}
	if _, err := os.Stat(filepath.Join(s.reportsDir(), "r2.dmp")); err == nil {
		t.Error("r2.dmp should not exist")
	}

	code, page, _ := get(t, ts, "/")
	if code != 200 {
		t.Fatalf("groups: %d", code)
	}
	if !strings.Contains(page, "s_strike  shellback.cpp:118") {
		t.Errorf("resolved title missing:\n%s", page)
	}
	if !strings.Contains(page, "0x3b9d5") || s.idx.groups["0ff1"].Title != "oe+0x3b9d5" { // html/template renders + as &#43;
		t.Errorf("raw title missing:\n%s", page)
	}
	g := s.idx.groups["b77c"]
	if g == nil || g.Count() != 2 || g.Users() != 1 || g.VersionList() != "0.3.1" {
		t.Fatalf("group b77c: %+v", g)
	}
	if s.idx.groups["0ff1"].Count() != 1 {
		t.Fatal("raw group")
	}

	code, page, _ = get(t, ts, "/group/b77c")
	if code != 200 || !strings.Contains(page, "s_bite") || !strings.Contains(page, "(inlined)") || !strings.Contains(page, "map arena") {
		t.Errorf("group page: %d\n%s", code, page)
	}
	code, page, _ = get(t, ts, "/report/r1")
	if code != 200 || !strings.Contains(page, "EXCEPTION_ACCESS_VIOLATION") || !strings.Contains(page, "script: gen 3 live") {
		t.Errorf("report page: %d\n%s", code, page)
	}
	code, body, hdr := get(t, ts, "/report/r1.dmp")
	if code != 200 || body != "hello" || hdr.Get("Content-Type") != "application/octet-stream" {
		t.Errorf("dump: %d %q %s", code, body, hdr.Get("Content-Type"))
	}
	code, body, hdr = get(t, ts, "/report/r1.json")
	if code != 200 || !strings.Contains(body, `"id": "r1"`) || !strings.HasPrefix(hdr.Get("Content-Type"), "application/json") {
		t.Errorf("json: %d", code)
	}
	if code, _, _ := get(t, ts, "/report/r2.dmp"); code != 404 {
		t.Errorf("r2.dmp: %d", code)
	}
	if code, _, _ := get(t, ts, "/?q=strike"); code != 200 {
		t.Errorf("search: %d", code)
	}
	_, page, _ = get(t, ts, "/?q=nothinghere")
	if strings.Contains(page, "s_strike") {
		t.Error("search did not filter")
	}
}

func TestIdempotentAndValidation(t *testing.T) {
	s, ts := newTest(t, nil)
	post(t, ts, "", fmt.Sprintf(resolvedReport, "same", "i"), nil)
	post(t, ts, "", fmt.Sprintf(resolvedReport, "same", "i"), nil)
	if len(s.idx.reports) != 1 {
		t.Fatalf("stored %d", len(s.idx.reports))
	}
	if r := post(t, ts, "", "{not json", nil); r.StatusCode != 400 {
		t.Errorf("bad json: %d", r.StatusCode)
	}
	if r := post(t, ts, "", `{"format":2,"id":"a","kind":"crash","signature":{"raw":"x"},"modules":[{"build_id":"b"}]}`, nil); r.StatusCode != 400 {
		t.Errorf("format 2: %d", r.StatusCode)
	}
	if r := post(t, ts, "", `{"format":1,"id":"a","kind":"crash","signature":{"raw":"x"},"modules":[]}`, nil); r.StatusCode != 400 {
		t.Errorf("no modules: %d", r.StatusCode)
	}
	if r := post(t, ts, "", `{"format":1,"id":"../evil","kind":"crash","signature":{"raw":"x"},"modules":[{"build_id":"b"}]}`, nil); r.StatusCode != 400 {
		t.Errorf("bad id: %d", r.StatusCode)
	}
	if r := post(t, ts, "", "", nil); r.StatusCode != 400 {
		t.Errorf("no report part: %d", r.StatusCode)
	}
}

func TestOversize(t *testing.T) {
	_, ts := newTest(t, func(c *config) { c.MaxReportKB = 1; c.MaxDumpMB = 1 })
	big := fmt.Sprintf(resolvedReport, "big", strings.Repeat("x", 2048))
	if r := post(t, ts, "", big, nil); r.StatusCode != 413 {
		t.Errorf("big report: %d", r.StatusCode)
	}
	if r := post(t, ts, "", fmt.Sprintf(resolvedReport, "d", "i"), bytes.Repeat([]byte{1}, 2<<20)); r.StatusCode != 413 {
		t.Errorf("big dump: %d", r.StatusCode)
	}
}

func TestToken(t *testing.T) {
	s, ts := newTest(t, func(c *config) { c.Token = "secret" })
	if r := post(t, ts, "", fmt.Sprintf(resolvedReport, "a", "i"), nil); r.StatusCode != 401 {
		t.Errorf("no token: %d", r.StatusCode)
	}
	if r := post(t, ts, "wrong", fmt.Sprintf(resolvedReport, "a", "i"), nil); r.StatusCode != 401 {
		t.Errorf("wrong token: %d", r.StatusCode)
	}
	if r := post(t, ts, "secret", fmt.Sprintf(resolvedReport, "a", "i"), nil); r.StatusCode != 200 {
		t.Errorf("right token: %d", r.StatusCode)
	}
	if len(s.idx.reports) != 1 {
		t.Error("stored count")
	}
}

func TestBasicAuth(t *testing.T) {
	_, ts := newTest(t, func(c *config) { c.User = "u"; c.Password = "p" })
	if code, _, _ := get(t, ts, "/"); code != 401 {
		t.Errorf("no auth: %d", code)
	}
	req, _ := http.NewRequest("GET", ts.URL+"/", nil)
	req.SetBasicAuth("u", "p")
	resp, _ := http.DefaultClient.Do(req)
	resp.Body.Close()
	if resp.StatusCode != 200 {
		t.Errorf("auth: %d", resp.StatusCode)
	}
	if r := post(t, ts, "", fmt.Sprintf(resolvedReport, "a", "i"), nil); r.StatusCode != 200 {
		t.Errorf("ingest never needs basic auth: %d", r.StatusCode)
	}
}

func TestRateLimit(t *testing.T) {
	s, ts := newTest(t, func(c *config) { c.RateIPPerMin = 3 })
	for i := 0; i < 5; i++ {
		if r := post(t, ts, "", fmt.Sprintf(resolvedReport, fmt.Sprint("id", i), "i"), nil); r.StatusCode != 200 {
			t.Fatalf("%d: %d", i, r.StatusCode)
		}
	}
	if len(s.idx.reports) != 3 {
		t.Fatalf("stored %d, want 3", len(s.idx.reports))
	}
	s.now = func() time.Time { return time.Now().Add(2 * time.Minute) }
	post(t, ts, "", fmt.Sprintf(resolvedReport, "later", "i"), nil)
	if len(s.idx.reports) != 4 {
		t.Fatalf("window did not slide: %d", len(s.idx.reports))
	}
}

func TestInstallRateLimit(t *testing.T) {
	s, ts := newTest(t, func(c *config) { c.RateInstallPerDay = 2; c.RateIPPerMin = 100 })
	for i := 0; i < 4; i++ {
		post(t, ts, "", fmt.Sprintf(resolvedReport, fmt.Sprint("n", i), "one-install"), nil)
	}
	post(t, ts, "", fmt.Sprintf(resolvedReport, "other", "another-install"), nil)
	if len(s.idx.reports) != 3 {
		t.Fatalf("stored %d, want 3", len(s.idx.reports))
	}
}

func TestDiskBudget(t *testing.T) {
	// Budget: ~5 reports of ~1.5 KB each plus small dumps. The newest of each group survives.
	s, ts := newTest(t, nil)
	budget := int64(9000)
	s.disk = budget
	base := time.Date(2026, 10, 1, 12, 0, 0, 0, time.UTC)
	tick := 0
	s.now = func() time.Time { tick++; return base.Add(time.Duration(tick) * time.Minute) }
	for i := 0; i < 6; i++ {
		post(t, ts, "", fmt.Sprintf(resolvedReport, fmt.Sprint("g1-", i), "i"), []byte("dumpdumpdump"))
	}
	post(t, ts, "", fmt.Sprintf(rawReport, "g2-0", "j"), []byte("dump"))
	if s.idx.bytes > budget {
		t.Fatalf("over budget: %d", s.idx.bytes)
	}
	g1, g2 := s.idx.groups["b77c"], s.idx.groups["0ff1"]
	if g1 == nil || g2 == nil {
		t.Fatal("a group vanished")
	}
	if g1.Reports[0].ID != "g1-5" || g2.Reports[0].ID != "g2-0" {
		t.Fatalf("newest not kept: %s %s", g1.Reports[0].ID, g2.Reports[0].ID)
	}
	if !g1.Reports[0].HasDump {
		t.Error("newest dump of the big group was evicted")
	}
	for _, e := range g1.Reports[1:] {
		if e.HasDump {
			t.Errorf("old dump kept: %s", e.ID)
		}
	}
	if g1.Count() >= 6 {
		t.Error("no report evicted")
	}
}

func TestDeleteAndRestart(t *testing.T) {
	s, ts := newTest(t, nil)
	post(t, ts, "", fmt.Sprintf(resolvedReport, "a", "i"), []byte("d"))
	post(t, ts, "", fmt.Sprintf(resolvedReport, "b", "k"), nil)
	post(t, ts, "", fmt.Sprintf(rawReport, "c", "i"), nil)

	again, err := newServer(s.cfg)
	if err != nil {
		t.Fatal(err)
	}
	if len(again.idx.reports) != 3 || again.idx.groups["b77c"].Count() != 2 || again.idx.groups["b77c"].Users() != 2 || !again.idx.reports["a"].HasDump {
		t.Fatalf("rebuilt index differs: %d reports", len(again.idx.reports))
	}

	resp, err := http.Post(ts.URL+"/group/b77c/delete", "application/x-www-form-urlencoded", nil)
	if err != nil {
		t.Fatal(err)
	}
	resp.Body.Close()
	if _, ok := s.idx.groups["b77c"]; ok {
		t.Error("group not deleted")
	}
	if _, err := os.Stat(filepath.Join(s.reportsDir(), "a.dmp")); err == nil {
		t.Error("dump not deleted")
	}
	if len(s.idx.reports) != 1 {
		t.Errorf("other group touched: %d", len(s.idx.reports))
	}
	if code, _, _ := get(t, ts, "/group/b77c"); code != 404 {
		t.Errorf("deleted group page: %d", code)
	}
}

func TestDeleteReport(t *testing.T) {
	s, ts := newTest(t, nil)
	post(t, ts, "", fmt.Sprintf(resolvedReport, "a", "i"), []byte("d"))
	post(t, ts, "", fmt.Sprintf(resolvedReport, "b", "k"), nil)
	resp, err := http.Post(ts.URL+"/report/a/delete", "application/x-www-form-urlencoded", nil)
	if err != nil {
		t.Fatal(err)
	}
	resp.Body.Close()
	if s.idx.get("a") != nil || s.idx.groups["b77c"].Count() != 1 || s.idx.groups["b77c"].Users() != 1 {
		t.Fatal("report not removed from its group")
	}
	if _, err := os.Stat(filepath.Join(s.reportsDir(), "a.dmp")); err == nil {
		t.Error("dump not deleted")
	}
	if !strings.HasSuffix(resp.Request.URL.Path, "/group/b77c") {
		t.Errorf("redirect to %s, want the group", resp.Request.URL.Path)
	}
	resp, _ = http.Post(ts.URL+"/report/b/delete", "application/x-www-form-urlencoded", nil)
	resp.Body.Close()
	if len(s.idx.groups) != 0 || resp.Request.URL.Path != "/" {
		t.Error("last report should take the group with it and land on the list")
	}
}

func TestExpire(t *testing.T) {
	s, ts := newTest(t, func(c *config) { c.MaxAgeDays = 30 })
	post(t, ts, "", fmt.Sprintf(resolvedReport, "old", "i"), []byte("d"))
	post(t, ts, "", fmt.Sprintf(resolvedReport, "new", "k"), nil)
	s.mu.Lock()
	s.idx.reports["old"].Received = time.Now().Add(-31 * 24 * time.Hour)
	s.mu.Unlock()
	if n := s.expire(); n != 1 {
		t.Fatalf("expired %d, want 1", n)
	}
	if s.idx.get("old") != nil || s.idx.get("new") == nil {
		t.Fatal("wrong report expired")
	}
	if _, err := os.Stat(filepath.Join(s.reportsDir(), "old.json")); err == nil {
		t.Error("expired file still on disk")
	}
	s.cfg.MaxAgeDays = 0
	s.mu.Lock()
	s.idx.reports["new"].Received = time.Now().Add(-1000 * 24 * time.Hour)
	s.mu.Unlock()
	if s.expire() != 0 {
		t.Error("max_age_days 0 must keep everything")
	}
}

func TestConfig(t *testing.T) {
	path := filepath.Join(t.TempDir(), "c.conf")
	os.WriteFile(path, []byte("addr = :1\n# comment\ntoken = abc # trailing\nmax_dump_mb = 3\n"), 0o644)
	c, err := loadConfig(path)
	if err != nil || c.Addr != ":1" || c.Token != "abc" || c.MaxDumpMB != 3 || c.DiskMB != 2048 {
		t.Fatalf("%+v %v", c, err)
	}
	os.WriteFile(path, []byte("nope = 1\n"), 0o644)
	if _, err := loadConfig(path); err == nil {
		t.Error("unknown key accepted")
	}
}
