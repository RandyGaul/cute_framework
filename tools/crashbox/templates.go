package main

const pageBase = `{{define "head"}}<!doctype html>
<html><head><meta charset="utf-8"><title>crashbox</title>
<style>
body { background: #15171c; color: #d6d6d6; font: 14px/1.5 ui-monospace, Consolas, monospace; margin: 24px; }
a { color: #8ab4f8; text-decoration: none; } a:hover { text-decoration: underline; }
table { border-collapse: collapse; width: 100%; }
th, td { text-align: left; padding: 4px 10px; border-bottom: 1px solid #2a2e36; vertical-align: top; white-space: nowrap; }
th { color: #9aa0ad; font-weight: normal; }
td.title { white-space: normal; }
.num { text-align: right; }
.dim { color: #9aa0ad; }
pre { background: #0f1115; padding: 12px; overflow-x: auto; border: 1px solid #2a2e36; }
form.inline { display: inline; }
input[type=text] { background: #0f1115; color: #d6d6d6; border: 1px solid #2a2e36; padding: 4px 8px; font: inherit; }
button { background: #2a2e36; color: #d6d6d6; border: 1px solid #3a3f4a; padding: 4px 10px; font: inherit; cursor: pointer; }
h1 { font-size: 16px; margin: 0 0 16px; } h1 a { color: inherit; }
form.inline{display:inline}button.link{background:none;border:none;color:inherit;opacity:.6;cursor:pointer;padding:0;font:inherit;text-decoration:underline}
</style></head><body>
<h1><a href="/">crashbox</a></h1>
{{end}}
{{define "foot"}}</body></html>{{end}}`

const groupsPage = `{{template "head" .}}
<form method="get" class="inline">
<input type="text" name="q" value="{{.Query}}" placeholder="search title, kind, version, app">
{{if .App}}<input type="hidden" name="app" value="{{.App}}">{{end}}
<button>search</button>
<span class="dim"> · sort: <a href="/?sort=count&q={{.Query}}&app={{.App}}">count</a> <a href="/?sort=last&q={{.Query}}&app={{.App}}">last</a> <a href="/?sort=first&q={{.Query}}&app={{.App}}">first</a> · {{.Total}} reports</span>
</form>
<table>
<tr><th class="num">count</th><th class="num">users</th><th>kind</th><th>title</th><th>versions</th><th>first</th><th>last</th></tr>
{{range .Groups}}<tr>
<td class="num">{{.Count}}</td><td class="num">{{.Users}}</td><td>{{.Kind}}</td>
<td class="title"><a href="/group/{{.Key}}">{{.Title}}</a>{{if .App}} <span class="dim">{{.App}}</span>{{end}}</td>
<td>{{.VersionList}}</td><td class="dim">{{ago .First}}</td><td class="dim">{{ago .Last}}</td>
</tr>{{else}}<tr><td colspan="7" class="dim">nothing yet</td></tr>{{end}}
</table>
{{template "foot" .}}`

const groupPage = `{{template "head" .}}
<p><b>{{.G.Title}}</b> <span class="dim">· {{.G.Kind}} · {{.G.App}} · {{.G.Count}} reports · {{.G.Users}} users · {{.G.VersionList}} · first {{ago .G.First}} · last {{ago .G.Last}}</span></p>
{{if .Stack}}<pre>{{range .Stack}}{{.}}
{{end}}</pre>{{end}}
<table>
<tr><th>received</th><th>version</th><th>os</th><th>install</th><th>where</th><th></th></tr>
{{range .Reports}}<tr>
<td class="dim">{{ago .Received}}</td><td>{{.Version}}</td><td>{{.OS}}</td><td class="dim">{{short .Install}}</td>
<td class="title">{{.Where}}</td>
<td><a href="/report/{{.ID}}">report</a> {{if .HasDump}}<a href="/report/{{.ID}}.dmp">dump</a>{{end}}
<form class="inline" method="post" action="/report/{{.ID}}/delete" onsubmit="return confirm('Delete this report?')"><button class="link">delete</button></form></td>
</tr>{{end}}
</table>
<p><form method="post" action="/group/{{.G.Key}}/delete" onsubmit="return confirm('Fixed? This deletes all {{.G.Count}} reports of this callstack. A new crash here starts a fresh group.')"><button>fixed: delete all {{.G.Count}} reports</button></form></p>
{{template "foot" .}}`

const reportPage = `{{template "head" .}}
<p><b>{{.E.Title}}</b> <span class="dim">· {{.E.Kind}} · {{ago .E.Received}} · <a href="/group/{{.E.Key}}">group</a> · <a href="/report/{{.E.ID}}.json">json</a>{{if .E.HasDump}} · <a href="/report/{{.E.ID}}.dmp">dump</a>{{end}}</span></p>
<pre>{{.Text}}</pre>
<p><form method="post" action="/report/{{.E.ID}}/delete" onsubmit="return confirm('Delete this report?')"><button>delete this report</button></form></p>
{{template "foot" .}}`
