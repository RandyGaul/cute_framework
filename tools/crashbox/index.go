package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

// The report as the client writes it (cute_crash.h, format 1). Only what the index and the
// pages need is typed; the file on disk stays the source of truth.
type report struct {
	Format      int               `json:"format"`
	ID          string            `json:"id"`
	Kind        string            `json:"kind"`
	App         string            `json:"app"`
	Version     string            `json:"version"`
	Build       string            `json:"build"`
	Config      string            `json:"config"`
	Time        string            `json:"time"`
	Uptime      float64           `json:"uptime"`
	Install     string            `json:"install"`
	Machine     map[string]any    `json:"machine"`
	Fault       map[string]any    `json:"fault"`
	Modules     []module          `json:"modules"`
	Stack       []frame           `json:"stack"`
	Signature   signature         `json:"signature"`
	Message     string            `json:"message"`
	State       map[string]string `json:"state"`
	Breadcrumbs []breadcrumb      `json:"breadcrumbs"`
	Attachments []attachment      `json:"attachments"`
}

type module struct {
	Name    string `json:"name"`
	Path    string `json:"path"`
	Base    string `json:"base"`
	Size    int64  `json:"size"`
	BuildID string `json:"build_id"`
	System  bool   `json:"system"`
	Symbols string `json:"symbols"`
}

type frame struct {
	Module   int      `json:"module"`
	Offset   int64    `json:"offset"`
	PC       string   `json:"pc"`
	Function string   `json:"function"`
	File     string   `json:"file"`
	Line     int      `json:"line"`
	Inlined  []inline `json:"inlined"`
}

type inline struct {
	Function string `json:"function"`
	File     string `json:"file"`
	Line     int    `json:"line"`
}

type signature struct {
	Raw      string `json:"raw"`
	Symbolic string `json:"symbolic"`
}

type breadcrumb struct {
	T   float64 `json:"t"`
	Msg string  `json:"msg"`
}

type attachment struct {
	Name  string `json:"name"`
	Type  string `json:"type"`
	Bytes int64  `json:"bytes"`
}

func parseReport(b []byte) (*report, error) {
	var r report
	if err := json.Unmarshal(b, &r); err != nil {
		return nil, err
	}
	switch {
	case r.Format != 1:
		return nil, errors.New("format is not 1")
	case r.ID == "" || strings.ContainsAny(r.ID, "/\\.") || len(r.ID) > 64:
		return nil, errors.New("bad id")
	case r.Kind == "":
		return nil, errors.New("no kind")
	case r.Signature.Raw == "" && r.Kind != "abnormal_exit":
		return nil, errors.New("no signature")
	case len(r.Modules) == 0 || r.Modules[0].BuildID == "":
		return nil, errors.New("no main module build id")
	}
	return &r, nil
}

func (r *report) groupKey() string {
	if r.Signature.Symbolic != "" {
		return r.Signature.Symbolic
	}
	if r.Signature.Raw == "" {
		return r.Kind + ":" + r.App // An abnormal exit with no stack: one group per app.
	}
	return r.Signature.Raw
}

// The top frame of the crashing thread, as a title: the first frame in a module the report
// does not call system, named when resolved, module+offset when not.
func (r *report) title() string {
	if a := r.State["assert"]; a != "" {
		return "assert " + a // The message the crash is about, when there is one.
	}
	if r.Kind == "report" && r.Message != "" {
		return "report " + r.Message
	}
	if r.Kind == "abnormal_exit" || len(r.Stack) == 0 {
		return "(no stack)"
	}
	for _, f := range r.Stack {
		if f.Module >= 0 && f.Module < len(r.Modules) && r.Modules[f.Module].System {
			continue
		}
		// The reporter's own frames sit above an abort or a sampled hang; the crash is below them.
		if strings.HasSuffix(strings.ReplaceAll(f.File, "\\", "/"), "/cute_crash.h") || f.File == "cute_crash.h" {
			continue
		}
		return r.frameText(f)
	}
	return r.frameText(r.Stack[0])
}

func (r *report) frameText(f frame) string {
	if f.Function != "" {
		where := ""
		if f.File != "" {
			where = "  " + filepath.Base(strings.ReplaceAll(f.File, "\\", "/"))
			if f.Line > 0 {
				where += fmt.Sprintf(":%d", f.Line)
			}
		}
		return f.Function + where
	}
	name := "?"
	if f.Module >= 0 && f.Module < len(r.Modules) {
		name = r.Modules[f.Module].Name
	}
	return fmt.Sprintf("%s+0x%x", name, f.Offset)
}

// What was true at the time, in one line: the first few state values.
func (r *report) where() string {
	keys := make([]string, 0, len(r.State))
	for k := range r.State {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	parts := []string{}
	for _, k := range keys {
		if len(parts) == 4 {
			break
		}
		parts = append(parts, k+" "+r.State[k])
	}
	return strings.Join(parts, " · ")
}

func (r *report) os() string {
	if s, ok := r.Machine["os"].(string); ok {
		return s
	}
	return ""
}

type entry struct {
	ID        string
	Received  time.Time
	Kind      string
	App       string
	Version   string
	Build     string
	Install   string
	Key       string
	Title     string
	Where     string
	OS        string
	HasDump   bool
	JSONBytes int64
	DumpBytes int64
}

type group struct {
	Key      string
	Title    string
	Kind     string
	App      string
	Reports  []*entry // newest first
	Installs map[string]bool
	Versions map[string]bool
	First    time.Time
	Last     time.Time
}

// What a page renders: a copy taken under the lock, so an ingest or a delete racing the render
// touches nothing the template reads.
type groupView struct {
	Key, Title, Kind, App string
	Count, Users          int
	VersionList           string
	First, Last           time.Time
	Reports               []*entry
}

func (g *group) view() groupView {
	return groupView{Key: g.Key, Title: g.Title, Kind: g.Kind, App: g.App, Count: g.Count(), Users: g.Users(), VersionList: g.VersionList(), First: g.First, Last: g.Last, Reports: append([]*entry(nil), g.Reports...)}
}

func (g *group) Count() int { return len(g.Reports) }
func (g *group) Users() int { return len(g.Installs) }
func (g *group) VersionList() string {
	vs := make([]string, 0, len(g.Versions))
	for v := range g.Versions {
		vs = append(vs, v)
	}
	sort.Strings(vs)
	return strings.Join(vs, " ")
}

type index struct {
	reports map[string]*entry
	groups  map[string]*group
	bytes   int64
}

func entryOf(r *report, received time.Time, jsonBytes, dumpBytes int64) *entry {
	return &entry{
		ID: r.ID, Received: received, Kind: r.Kind, App: r.App, Version: r.Version, Build: r.Build,
		Install: r.Install, Key: r.groupKey(), Title: r.title(), Where: r.where(), OS: r.os(),
		HasDump: dumpBytes > 0, JSONBytes: jsonBytes, DumpBytes: dumpBytes,
	}
}

func loadIndex(dir string) (*index, error) {
	idx := &index{reports: map[string]*entry{}, groups: map[string]*group{}}
	names, err := filepath.Glob(filepath.Join(dir, "*.json"))
	if err != nil {
		return nil, err
	}
	for _, name := range names {
		b, err := os.ReadFile(name)
		if err != nil {
			continue
		}
		r, err := parseReport(b)
		if err != nil {
			continue
		}
		st, err := os.Stat(name)
		if err != nil {
			continue
		}
		var dump int64
		if ds, err := os.Stat(strings.TrimSuffix(name, ".json") + ".dmp"); err == nil {
			dump = ds.Size()
		}
		idx.add(entryOf(r, st.ModTime(), st.Size(), dump))
	}
	return idx, nil
}

func (idx *index) get(id string) *entry { return idx.reports[id] }

func (idx *index) add(e *entry) {
	idx.reports[e.ID] = e
	idx.bytes += e.JSONBytes + e.DumpBytes
	g := idx.groups[e.Key]
	if g == nil {
		g = &group{Key: e.Key, Installs: map[string]bool{}, Versions: map[string]bool{}, First: e.Received}
		idx.groups[e.Key] = g
	}
	g.Reports = append(g.Reports, e)
	sort.Slice(g.Reports, func(i, j int) bool { return g.Reports[i].Received.After(g.Reports[j].Received) })
	if e.Install != "" {
		g.Installs[e.Install] = true
	}
	if e.Version != "" {
		g.Versions[e.Version] = true
	}
	if e.Received.Before(g.First) {
		g.First = e.Received
	}
	if e.Received.After(g.Last) {
		g.Last = e.Received
	}
	newest := g.Reports[0]
	g.Title, g.Kind, g.App = newest.Title, newest.Kind, newest.App
}

func (idx *index) store(dir string, r *report, jsonBytes, dumpBytes []byte, now time.Time) (*entry, error) {
	path := filepath.Join(dir, r.ID+".json")
	if err := os.WriteFile(path, jsonBytes, 0o644); err != nil {
		return nil, err
	}
	if len(dumpBytes) > 0 {
		if err := os.WriteFile(filepath.Join(dir, r.ID+".dmp"), dumpBytes, 0o644); err != nil {
			return nil, err
		}
	}
	os.Chtimes(path, now, now)
	e := entryOf(r, now, int64(len(jsonBytes)), int64(len(dumpBytes)))
	idx.add(e)
	return e, nil
}

func (idx *index) remove(dir string, e *entry) {
	os.Remove(filepath.Join(dir, e.ID+".json"))
	os.Remove(filepath.Join(dir, e.ID+".dmp"))
	idx.bytes -= e.JSONBytes + e.DumpBytes
	delete(idx.reports, e.ID)
	g := idx.groups[e.Key]
	if g == nil {
		return
	}
	kept := g.Reports[:0]
	for _, x := range g.Reports {
		if x != e {
			kept = append(kept, x)
		}
	}
	g.Reports = kept
	if len(g.Reports) == 0 {
		delete(idx.groups, e.Key)
		return
	}
	g.Installs, g.Versions = map[string]bool{}, map[string]bool{}
	g.First, g.Last = g.Reports[0].Received, g.Reports[0].Received
	for _, x := range g.Reports {
		if x.Install != "" {
			g.Installs[x.Install] = true
		}
		if x.Version != "" {
			g.Versions[x.Version] = true
		}
		if x.Received.Before(g.First) {
			g.First = x.Received
		}
		if x.Received.After(g.Last) {
			g.Last = x.Received
		}
	}
	newest := g.Reports[0]
	g.Title, g.Kind, g.App = newest.Title, newest.Kind, newest.App
}

func (idx *index) removeGroup(dir, key string) {
	g := idx.groups[key]
	if g == nil {
		return
	}
	for _, e := range append([]*entry(nil), g.Reports...) {
		idx.remove(dir, e)
	}
}

func (idx *index) dropDump(dir string, e *entry) {
	os.Remove(filepath.Join(dir, e.ID+".dmp"))
	idx.bytes -= e.DumpBytes
	e.DumpBytes, e.HasDump = 0, false
}

// The oldest report that still has a dump and is not its group's newest, or nil.
func (idx *index) oldestEvictableDump() *entry {
	var best *entry
	for _, g := range idx.groups {
		for i, e := range g.Reports {
			if i == 0 || !e.HasDump {
				continue
			}
			if best == nil || e.Received.Before(best.Received) {
				best = e
			}
		}
	}
	return best
}

// The oldest report of the group with the most reports, among groups with more than one, or nil.
func (idx *index) oldestOfLargestGroup() *entry {
	var largest *group
	for _, g := range idx.groups {
		if len(g.Reports) < 2 {
			continue
		}
		if largest == nil || len(g.Reports) > len(largest.Reports) {
			largest = g
		}
	}
	if largest == nil {
		return nil
	}
	return largest.Reports[len(largest.Reports)-1]
}

func (idx *index) groupList(sortBy, q, app string) []*group {
	q = strings.ToLower(q)
	var out []*group
	for _, g := range idx.groups {
		if app != "" && g.App != app {
			continue
		}
		if q != "" {
			hay := strings.ToLower(g.Title + " " + g.Kind + " " + g.VersionList() + " " + g.App)
			if !strings.Contains(hay, q) {
				continue
			}
		}
		out = append(out, g)
	}
	sort.Slice(out, func(i, j int) bool {
		a, b := out[i], out[j]
		switch sortBy {
		case "last":
			return a.Last.After(b.Last)
		case "first":
			return a.First.After(b.First)
		}
		if a.Count() != b.Count() {
			return a.Count() > b.Count()
		}
		return a.Last.After(b.Last)
	})
	return out
}
