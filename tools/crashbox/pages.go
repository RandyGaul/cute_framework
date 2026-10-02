package main

import (
	"fmt"
	"html/template"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"
)

func ago(t time.Time, now time.Time) string {
	d := now.Sub(t)
	switch {
	case d < time.Minute:
		return "just now"
	case d < time.Hour:
		return fmt.Sprintf("%d min ago", int(d.Minutes()))
	case d < 48*time.Hour:
		return fmt.Sprintf("%d h ago", int(d.Hours()))
	default:
		return fmt.Sprintf("%d d ago", int(d.Hours()/24))
	}
}

func (s *server) funcs() template.FuncMap {
	return template.FuncMap{
		"ago":   func(t time.Time) string { return ago(t, s.now()) },
		"short": func(x string) string { return x[:min(8, len(x))] },
	}
}

func (s *server) render(w http.ResponseWriter, tmpl string, data any) {
	t, err := template.New("page").Funcs(s.funcs()).Parse(pageBase + tmpl)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	if err := t.Execute(w, data); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func (s *server) groupsPage(w http.ResponseWriter, r *http.Request) {
	q := r.URL.Query()
	s.mu.Lock()
	groups := s.idx.groupList(q.Get("sort"), q.Get("q"), q.Get("app"))
	total := len(s.idx.reports)
	s.mu.Unlock()
	s.render(w, groupsPage, map[string]any{
		"Groups": groups, "Query": q.Get("q"), "App": q.Get("app"), "Sort": q.Get("sort"), "Total": total,
	})
}

func (s *server) groupPage(w http.ResponseWriter, r *http.Request) {
	key := r.PathValue("key")
	s.mu.Lock()
	g := s.idx.groups[key]
	var reports []*entry
	if g != nil {
		reports = append(reports, g.Reports...)
	}
	s.mu.Unlock()
	if g == nil {
		http.NotFound(w, r)
		return
	}
	var stack []string
	if rep := s.readReport(reports[0].ID); rep != nil {
		stack = stackLines(rep)
	}
	s.render(w, groupPage, map[string]any{"G": g, "Reports": reports, "Stack": stack})
}

func (s *server) groupDelete(w http.ResponseWriter, r *http.Request) {
	key := r.PathValue("key")
	s.mu.Lock()
	s.idx.removeGroup(s.reportsDir(), key)
	s.mu.Unlock()
	http.Redirect(w, r, "/", http.StatusSeeOther)
}

func (s *server) readReport(id string) *report {
	b, err := os.ReadFile(filepath.Join(s.reportsDir(), id+".json"))
	if err != nil {
		return nil
	}
	rep, err := parseReport(b)
	if err != nil {
		return nil
	}
	return rep
}

func (s *server) reportPage(w http.ResponseWriter, r *http.Request) {
	id := r.PathValue("id")
	ext := filepath.Ext(id)
	id = strings.TrimSuffix(id, ext)
	s.mu.Lock()
	e := s.idx.get(id)
	s.mu.Unlock()
	if e == nil {
		http.NotFound(w, r)
		return
	}
	switch ext {
	case ".json":
		b, err := os.ReadFile(filepath.Join(s.reportsDir(), id+".json"))
		if err != nil {
			http.NotFound(w, r)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		w.Write(prettyJSON(b))
	case ".dmp":
		if !e.HasDump {
			http.NotFound(w, r)
			return
		}
		w.Header().Set("Content-Type", "application/octet-stream")
		w.Header().Set("Content-Disposition", fmt.Sprintf(`attachment; filename="%s.dmp"`, id))
		http.ServeFile(w, r, filepath.Join(s.reportsDir(), id+".dmp"))
	case "":
		rep := s.readReport(id)
		if rep == nil {
			http.NotFound(w, r)
			return
		}
		s.render(w, reportPage, map[string]any{"E": e, "Text": renderText(rep)})
	default:
		http.NotFound(w, r)
	}
}

func stackLines(r *report) []string {
	var out []string
	for i, f := range r.Stack {
		line := fmt.Sprintf("%3d  %s", i, r.frameText(f))
		if f.Function != "" && f.File != "" {
			line = fmt.Sprintf("%3d  %-40s %s", i, f.Function, fileLine(f.File, f.Line))
		}
		out = append(out, line)
		for _, in := range f.Inlined {
			out = append(out, fmt.Sprintf("       %-38s %s  (inlined)", in.Function, fileLine(in.File, in.Line)))
		}
	}
	return out
}

func fileLine(file string, line int) string {
	if line > 0 {
		return fmt.Sprintf("%s:%d", file, line)
	}
	return file
}

// The whole report as text, the same layout cute-sym print uses.
func renderText(r *report) string {
	var b strings.Builder
	put := func(k, v string) {
		if v != "" {
			fmt.Fprintf(&b, "%-9s %s\n", k+":", v)
		}
	}
	put("app", r.App)
	put("version", r.Version)
	put("build", strings.TrimSpace(r.Build+" "+r.Config))
	put("kind", r.Kind)
	put("time", r.Time)
	if r.Uptime > 0 {
		put("uptime", fmt.Sprintf("%.1f s", r.Uptime))
	}
	put("install", r.Install)
	for _, k := range sortedKeys(r.Machine) {
		put(k, fmt.Sprint(r.Machine[k]))
	}
	if len(r.Fault) > 0 {
		b.WriteString("\nfault:\n")
		for _, k := range sortedKeys(r.Fault) {
			fmt.Fprintf(&b, "  %s: %v\n", k, r.Fault[k])
		}
	}
	if len(r.Stack) > 0 {
		b.WriteString("\nstack:\n")
		for _, l := range stackLines(r) {
			b.WriteString(l + "\n")
		}
	}
	if len(r.State) > 0 {
		b.WriteString("\nstate:\n")
		keys := make([]string, 0, len(r.State))
		for k := range r.State {
			keys = append(keys, k)
		}
		sort.Strings(keys)
		for _, k := range keys {
			fmt.Fprintf(&b, "  %s: %s\n", k, r.State[k])
		}
	}
	if len(r.Breadcrumbs) > 0 {
		fmt.Fprintf(&b, "\nbreadcrumbs (%d):\n", len(r.Breadcrumbs))
		for _, c := range r.Breadcrumbs {
			fmt.Fprintf(&b, "  [%8.2f] %s\n", c.T, c.Msg)
		}
	}
	if len(r.Modules) > 0 {
		b.WriteString("\nmodules:\n")
		for _, m := range r.Modules {
			sys := ""
			if m.System {
				sys = "  system"
			}
			fmt.Fprintf(&b, "  %-28s %-18s %-10d %s%s\n", m.Name, m.Base, m.Size, m.BuildID, sys)
		}
	}
	if len(r.Attachments) > 0 {
		b.WriteString("\nattachments:\n")
		for _, a := range r.Attachments {
			fmt.Fprintf(&b, "  %s (%s, %d bytes)\n", a.Name, a.Type, a.Bytes)
		}
	}
	return b.String()
}

func sortedKeys(m map[string]any) []string {
	keys := make([]string, 0, len(m))
	for k := range m {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	return keys
}
