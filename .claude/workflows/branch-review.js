export const meta = {
  name: 'branch-review',
  description: 'Multi-agent pre-PR review of the current branch diff with adversarial verification',
  whenToUse: 'Before opening a non-trivial PR. args: {base?: string} (default "master").',
  phases: [
    { title: 'Find', detail: 'parallel dimension-scoped reviewers' },
    { title: 'Verify', detail: 'adversarial skeptic per finding' },
  ],
}

const BASE = (args && args.base) || 'master'

const FINDINGS = {
  type: 'object', required: ['findings'],
  properties: {
    findings: {
      type: 'array',
      items: {
        type: 'object',
        required: ['file', 'line', 'summary', 'scenario', 'severity'],
        properties: {
          file: { type: 'string' },
          line: { type: 'integer' },
          summary: { type: 'string', description: 'one-sentence defect statement' },
          scenario: { type: 'string', description: 'concrete inputs/state -> wrong outcome' },
          severity: { enum: ['critical', 'high', 'medium', 'low'] },
        },
      },
    },
  },
}

const VERDICT = {
  type: 'object', required: ['real', 'reason'],
  properties: { real: { type: 'boolean' }, reason: { type: 'string' } },
}

const DIMENSIONS = [
  ['memory', 'memory errors: leaks (cf_alloc/cf_free pairing on ALL paths incl. error paths), use-after-free, double free, buffer overruns, pointers into ckit dynamic arrays held across apush/afit'],
  ['correctness', 'logic errors, off-by-one, integer truncation/sign, uninitialized fields, wrong lifecycle ordering, missing null checks on public API entry points'],
  ['api-contract', 'public API breaks: changed behavior of existing cf_* functions, namespace Cute C++ wrapper out of sync with the C declaration, deprecated forwarders that no longer forward'],
  ['cross-platform', 'works-on-macOS-only hazards: Emscripten/WebGL2 (no compute shaders, async main loop), Linux/GLES3 backend, HiDPI points-vs-pixels confusion'],
  ['silent-failure', 'errors swallowed instead of returned via CF_Result, fallbacks that hide breakage, warnings suppressed'],
]

phase('Find')
const rounds = await parallel(DIMENSIONS.map(([key, focus]) => () =>
  agent(
    `Review this branch's changes vs ${BASE}, hunting ONLY for: ${focus}.\n` +
    `Get the change set yourself: git diff ${BASE}...HEAD plus git status for untracked files. ` +
    `Read the full context around each hunk before judging. Try to refute each candidate finding first; ` +
    `report only findings that survive refutation. line = 1-indexed line in the NEW file.`,
    { label: `find:${key}`, phase: 'Find', agentType: 'code-reviewer', schema: FINDINGS })))

// Barrier justified: dedupe across ALL finders before paying for verification.
const seen = new Set()
const candidates = rounds.filter(Boolean).flatMap(r => r.findings).filter(f => {
  const k = `${f.file}:${f.line}`
  if (seen.has(k)) return false
  seen.add(k)
  return true
})
log(`${candidates.length} candidate findings from ${DIMENSIONS.length} dimensions`)

phase('Verify')
const verified = await parallel(candidates.map(f => () =>
  agent(
    `Adversarially verify a claimed defect. Your default stance: it is WRONG until proven. ` +
    `Read the code, its callers, and invariants, and try hard to refute it.\n` +
    `Claim: ${f.file}:${f.line} - ${f.summary}\nScenario: ${f.scenario}\n` +
    `Set real=true ONLY if you could not refute it; explain either way in reason.`,
    { label: `verify:${f.file}:${f.line}`, phase: 'Verify', agentType: 'code-reviewer', schema: VERDICT })
    .then(v => (v && v.real ? { ...f, verified_reason: v.reason } : null))))

const confirmed = verified.filter(Boolean)
const order = { critical: 0, high: 1, medium: 2, low: 3 }
confirmed.sort((a, b) => order[a.severity] - order[b.severity])
log(`${confirmed.length}/${candidates.length} findings survived adversarial verification`)
return { confirmed, candidateCount: candidates.length, base: BASE }
