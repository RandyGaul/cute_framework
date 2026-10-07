#!/usr/bin/env python3
# Loads tools/web_smoke.c's page (the cute-web-smoke target) in Chrome and checks the backend it picked, the
# pixel it read back, and that nothing logged a console error. Exits nonzero on any mismatch.
#   python3 tools/web_smoke.py --browser google-chrome --dir build --expect CF_BACKEND_TYPE_WEBGPU -- <browser flags>
import argparse, http.server, json, shutil, subprocess, sys, tempfile, threading

ap = argparse.ArgumentParser()
ap.add_argument("--browser", required=True)
ap.add_argument("--dir", required=True, help="directory holding web_smoke.html")
ap.add_argument("--expect", required=True, help="backend name cf_backend_type_to_string reports")
ap.add_argument("--allow", action="append", default=[], help="substring of a console error to tolerate")
ap.add_argument("--timeout", type=float, default=60)
ap.add_argument("--headful", action="store_true", help="run a windowed browser (needs a display, e.g. xvfb-run)")
ap.add_argument("flags", nargs="*", help="extra browser flags")
args = ap.parse_args()

result = {}
done = threading.Event()

class Handler(http.server.SimpleHTTPRequestHandler):
	def __init__(self, *a, **kw):
		super().__init__(*a, directory=args.dir, **kw)
	def do_POST(self):
		body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
		self.send_response(204)
		self.end_headers()
		if self.path == "/result":
			result.update(json.loads(body))
			done.set()
	def log_message(self, *a):
		pass

server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()
url = "http://127.0.0.1:%d/web_smoke.html" % server.server_address[1]

profile = tempfile.mkdtemp(prefix="cf-smoke-")
cmd = [args.browser] + ([] if args.headful else ["--headless=new"]) + ["--no-first-run", "--no-default-browser-check",
	"--user-data-dir=" + profile, "--enable-logging=stderr", "--v=0"] + args.flags + [url]
print("$ " + " ".join(cmd), flush=True)
browser = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, errors="replace")
log = []
threading.Thread(target=lambda: log.extend(browser.stderr), daemon=True).start()

ok = done.wait(args.timeout)
browser.kill()
browser.wait()
server.shutdown()
shutil.rmtree(profile, ignore_errors=True)

def fail(msg):
	print("".join(l for l in log if "CONSOLE" in l or "ERROR" in l)[-20000:])
	print("FAIL: " + msg)
	sys.exit(1)

if not ok:
	fail("the page never reported within %gs (crashed, hung, or failed to load)" % args.timeout)
print("report:", json.dumps(result))
errors = [e for e in result.get("errors", []) if not any(a in e for a in args.allow)]
if result.get("backend") != args.expect:
	fail("backend %s, expected %s" % (result.get("backend"), args.expect))
r, g, b = result.get("pixel", [-1, -1, -1])
if not (r > 200 and g < 60 and b < 60):
	fail("readback center pixel %d,%d,%d is not the red box" % (r, g, b))
if errors:
	fail("console errors:\n  " + "\n  ".join(errors))
print("PASS: %s, red box read back" % args.expect)
