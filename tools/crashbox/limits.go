package main

import "time"

// A sliding window per key: at most `limit` events in the last `window`. Keys are pruned when
// their window empties, so the map only holds recent senders.
type limiter struct {
	limit  int
	window time.Duration
	seen   map[string][]time.Time
}

func newLimiter(limit int, window time.Duration) *limiter {
	return &limiter{limit: limit, window: window, seen: map[string][]time.Time{}}
}

func (l *limiter) allow(key string, now time.Time) bool {
	if l.limit <= 0 {
		return true
	}
	cut := now.Add(-l.window)
	times := l.seen[key]
	kept := times[:0]
	for _, t := range times {
		if t.After(cut) {
			kept = append(kept, t)
		}
	}
	if len(kept) >= l.limit {
		l.seen[key] = kept
		return false
	}
	l.seen[key] = append(kept, now)
	if len(l.seen) > 100000 {
		for k, ts := range l.seen {
			if len(ts) == 0 || !ts[len(ts)-1].After(cut) {
				delete(l.seen, k)
			}
		}
	}
	return true
}
