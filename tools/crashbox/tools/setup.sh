#!/bin/sh
# One-shot provisioning for crashbox on a box (Ubuntu, run as root): its own user, tree, unit and
# port, nothing else on the box touched. Re-running is safe; the config file is kept once it exists.
#
#   ssh root@box 'sh -s' < tools/setup.sh
#
# Then tools/deploy.sh puts the binary in place. The link is https://<box>:8445/
# CERT_SRC is a directory holding <domain>.crt and <domain>.key for the box's name (here, where a
# CertMagic gateway on the same box keeps its Let's Encrypt pair); point it wherever yours are.
set -eu
PORT=8445
CERT_SRC="${CERT_SRC:-/opt/friendslop/gateway/certs/certificates/acme-v02.api.letsencrypt.org-directory}"
id crashbox >/dev/null 2>&1 || useradd -r -s /usr/sbin/nologin crashbox
mkdir -p /opt/crashbox/bin /opt/crashbox/data /opt/crashbox/tls
chown -R crashbox:crashbox /opt/crashbox/data /opt/crashbox/bin

# The certificate: the box's own pair, copied daily as root into a directory only crashbox reads;
# the service reloads it by mtime. Read-only toward its source.
cat > /opt/crashbox/bin/tls_sync.sh <<SH
#!/bin/sh
set -eu
DOMAIN=\$(hostname -I | awk '{ print \$1 }' | tr . -).sslip.io
SRC=$CERT_SRC/\$DOMAIN
DST=/opt/crashbox/tls
[ -f "\$SRC/\$DOMAIN.crt" ] && [ -f "\$SRC/\$DOMAIN.key" ] || { echo "tls_sync: no certificate at \$SRC"; exit 1; }
for ext in crt key; do
	if ! cmp -s "\$SRC/\$DOMAIN.\$ext" "\$DST/tls.\$ext"; then
		cp "\$SRC/\$DOMAIN.\$ext" "\$DST/tls.\$ext"
		echo "tls_sync: tls.\$ext updated"
	fi
done
chown -R crashbox:crashbox "\$DST"
chmod 700 "\$DST"
chmod 600 "\$DST/tls.key"
chmod 644 "\$DST/tls.crt"
SH
chmod 755 /opt/crashbox/bin/tls_sync.sh

cat > /etc/systemd/system/crashbox-tls-sync.service <<'UNIT'
[Unit]
Description=crashbox: copy the box certificate
[Service]
Type=oneshot
ExecStart=/bin/sh /opt/crashbox/bin/tls_sync.sh
UNIT
cat > /etc/systemd/system/crashbox-tls-sync.timer <<'UNIT'
[Unit]
Description=crashbox: daily certificate sync
[Timer]
OnCalendar=daily
RandomizedDelaySec=1h
Persistent=true
[Install]
WantedBy=timers.target
UNIT
sh /opt/crashbox/bin/tls_sync.sh

# The config, written once with fresh secrets. The ingest token goes into the game's
# CF_CrashConfig.upload_token; the login is the dashboard's.
if [ ! -f /opt/crashbox/crashbox.conf ]; then
	TOKEN=$(head -c 24 /dev/urandom | od -An -tx1 | tr -d ' \n')
	PASS=$(head -c 12 /dev/urandom | od -An -tx1 | tr -d ' \n')
	cat > /opt/crashbox/crashbox.conf <<CONF
addr = :$PORT
tls = /opt/crashbox/tls
data = /opt/crashbox/data
token = $TOKEN
user = randy
password = $PASS
CONF
	chown crashbox:crashbox /opt/crashbox/crashbox.conf
	chmod 600 /opt/crashbox/crashbox.conf
fi

cat > /etc/systemd/system/crashbox.service <<'UNIT'
[Unit]
Description=crashbox: crash reports from Cute Framework games
After=network.target

[Service]
User=crashbox
WorkingDirectory=/opt/crashbox
ExecStart=/opt/crashbox/bin/crashbox -config /opt/crashbox/crashbox.conf
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
UNIT

systemctl daemon-reload
systemctl enable --now crashbox-tls-sync.timer
systemctl enable crashbox
[ -x /opt/crashbox/bin/crashbox ] && systemctl restart crashbox || echo "setup: no binary yet; tools/deploy.sh puts it in place"
echo "setup: done. config:"
cat /opt/crashbox/crashbox.conf
