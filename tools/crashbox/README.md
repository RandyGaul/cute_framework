# crashbox

A crash report inbox for games built on Cute Framework's `cute_crash.h`. Reports arrive as the one
POST the client already makes, land as files, and show up on a page grouped by signature with
counts, users and versions. Standard library Go, one binary, no database.

Three touches:

1. Run it once: `crashbox -config crashbox.conf` (a systemd unit on the box does this).
2. In the game: `cc.upload_url = "https://<host>:8445/v1/crash";`
3. When you wonder: open `https://<host>:8445/`. Biggest crash at the top. Click it.

## Config

`crashbox.conf`, `key = value`, every key optional:

```
addr = :8445              # listen address
tls = /opt/crashbox/tls   # directory with tls.crt + tls.key; empty = plain HTTP (local testing). Reloaded when they change.
data = ./data             # reports/<id>.json and reports/<id>.dmp
token =                   # ingest bearer token; empty = open
user =                    # dashboard basic auth; both empty = open
password =
max_report_kb = 512       # a larger report is refused (413)
max_dump_mb = 8           # a larger dump is refused (413)
disk_mb = 2048            # total for data/; past it old dumps go first, then old reports; the newest of every group stays
rate_ip_per_min = 10      # over the limit the server answers 200 and keeps nothing
rate_install_per_day = 60
```

## Endpoints

- `POST /v1/crash`: multipart with `report` (JSON, format 1) and optional `crash.dmp`. Any reply is the client's ack.
- `GET /`: the groups. `?q=` searches title, kind, version, app; `?sort=count|last|first`; `?app=`.
- `GET /group/<signature>`: the group's newest stack and its reports. `POST /group/<signature>/delete` removes it.
- `GET /report/<id>`: the report as text. `/report/<id>.json` raw, `/report/<id>.dmp` the minidump.

## Build

```
go test ./...
GOOS=linux GOARCH=amd64 CGO_ENABLED=0 go build -o build/crashbox-linux-amd64 .
```
