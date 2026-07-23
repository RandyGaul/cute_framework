#!/usr/bin/env bash
#
# Smoke test a Cute Framework sample.
#
# Launches a sample, lets it run for a few seconds, and verifies that it does
# not crash or exit early. This catches regressions that only surface at
# runtime -- most notably native shader compilation failures (e.g. the MSL
# backend on macOS), which a plain build cannot detect.
#
# A sample runs its own infinite loop until the window is closed, so the
# strategy is:
#   * launch it in the background,
#   * poll for the requested duration,
#   * if it dies before the timer elapses  -> FAIL (it crashed),
#   * if it is still alive when time is up  -> PASS (terminate it cleanly).
#
# Usage: smoke_test.sh <path-to-sample> [duration-seconds]
#
# On Linux this is expected to be wrapped in xvfb-run for a headless display.
# On macOS and Windows the runners provide a usable desktop session, so the
# sample can be launched directly.

set -u

APP="${1:?usage: smoke_test.sh <path-to-sample> [duration-seconds]}"
DURATION="${2:-8}"

# Windows builds produce an .exe; allow callers to omit the suffix. Test with
# -e rather than -x: Git Bash on Windows reports the executable bit
# unreliably, which could otherwise skip this fallback for a file that exists.
if [[ ! -e "$APP" && -e "$APP.exe" ]]; then
	APP="$APP.exe"
fi

if [[ ! -e "$APP" ]]; then
	echo "smoke test: sample '$APP' not found" >&2
	exit 1
fi

echo "smoke test: launching '$APP' for ${DURATION}s..."
"$APP" &
PID=$!

# Poll once per second. If the process disappears before the timer elapses it
# exited on its own -- for these samples that means it crashed.
for ((i = 0; i < DURATION; i++)); do
	if ! kill -0 "$PID" 2>/dev/null; then
		wait "$PID"
		code=$?
		echo "smoke test: FAILED -- '$APP' exited early after ~${i}s with code ${code}" >&2
		exit 1
	fi
	sleep 1
done

# Still running after the full duration -- success. Shut it down.
echo "smoke test: '$APP' survived ${DURATION}s, terminating."
kill "$PID" 2>/dev/null
wait "$PID" 2>/dev/null
echo "smoke test: PASSED"
exit 0
