#!/bin/sh
# Build for the box and put the binary in place: cross-compile here, copy as .next, swap and
# restart in one ssh. Run from tools/crashbox:
#
#   BOX=<host> KEY=<root ssh key> sh tools/deploy.sh
set -eu
BOX="${BOX:?set BOX to the box's host}"
KEY="${KEY:?set KEY to the root ssh key}"
GOOS=linux GOARCH=amd64 CGO_ENABLED=0 go build -trimpath -ldflags "-s -w" -o build/crashbox-linux-amd64 .
scp -q -i "$KEY" build/crashbox-linux-amd64 "root@$BOX:/opt/crashbox/bin/crashbox.next"
ssh -i "$KEY" "root@$BOX" 'chown crashbox:crashbox /opt/crashbox/bin/crashbox.next && chmod 755 /opt/crashbox/bin/crashbox.next && mv -f /opt/crashbox/bin/crashbox.next /opt/crashbox/bin/crashbox && systemctl restart crashbox && sleep 1 && systemctl is-active crashbox'
