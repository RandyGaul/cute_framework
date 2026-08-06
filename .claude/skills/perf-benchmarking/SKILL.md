---
name: perf-benchmarking
description: Benchmark methodology for Cute Framework performance work on this Mac. Use before ANY perf claim, comparison, or optimization — no perf statement without same-harness numbers.
---

# Performance Benchmarking Methodology

Hard rule: **no perf claim without same-harness before/after numbers.**
"Should be faster" is a hypothesis, not a result.

## Why the obvious approach lies on this machine

Frame times on this Mac are **bimodal** — ~3× swings from P-core vs E-core
scheduling and compositor throttle, and runs sometimes vsync-lock to
~16.6 ms even with `cf_app_set_vsync(false)`. Two sequential runs of the
same binary can differ more than the optimization you're measuring.

## The method

1. **Define the metric first.** Usually frame-time p50 at a fixed workload;
   for CPU-side work prefer phase timers (submit / batch / present split) or
   a microbench median — end-to-end frame deltas at small workloads hide
   under a ~1.9 ms present/compositor floor.
2. **Interleave A/B.** Never run all of A then all of B. Alternate the two
   binaries round-robin within one session so thermal/scheduler drift hits
   both sides equally.
3. **Min-of-rounds.** Compare the minimum of each round's p50s (or medians
   for microbenches). The minimum is the least-noisy estimator here.
4. **Fixed workload tiers: 100 / 1k / 10k / 100k draws.**
   **Never exceed 100k draws/frame on this machine** — 100k-churn and 1M
   workloads hard-crash the Mac. Meaningful end-to-end deltas only show at
   100k; ≤10k sits under the present floor.
5. **Record numbers in the report** — actual µs/ms values for both sides,
   the workload, and which estimator you used. Also record **refuted
   hypotheses** so nobody re-chases them (e.g. hash lookups ~16 ns/sprite —
   not worth chasing; the report-phase memset was ~2–3%).
6. **Watch bystander metrics.** A win in batch time that regresses submit
   time by 10% must be called out, not buried.

## Tools

- Phase timers / microbenches in the code beat external sampling for A/B.
- `xctrace record --template 'Time Profiler'` (Instruments CLI) for finding
  where time goes when you don't yet have a hypothesis.
- Crash forensics: `.ips` reports land in `~/Library/Logs/DiagnosticReports/`
  (auto-moved to `Retired/` within minutes); JSON after the first line.

## Known landscape (don't rediscover)

- Draw batching: no-op sorter + geometry-by-pointer + memo cache already
  landed; spritebatch CPU at 10k sprites ~12× faster than pre-2026-07.
- Render-pass churn: per-batch vertex re-upload forces a render-pass
  teardown per batch; batching uploads before draws was validated ~10× but
  check current state before assuming it landed. Raising SDL
  frames-in-flight makes it WORSE.
- Command-stream churn workload shows superlinear cost (separate, unexplored).
- Upstream perf issues on file: #47 (vertex-data ceiling), #501
  (canvas-size fps regression).
