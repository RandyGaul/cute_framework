export const meta = {
  name: 'docs-audit',
  description: 'Audit doc comments in public headers against their real implementation',
  whenToUse: 'After a feature lands or before a docs PR. args: {headers?: string[]} - omit to audit headers changed vs master.',
  phases: [
    { title: 'Scope', detail: 'determine which headers to audit' },
    { title: 'Audit', detail: 'one agent per header, report-only' },
  ],
}

const ISSUES = {
  type: 'object', required: ['header', 'issues'],
  properties: {
    header: { type: 'string' },
    issues: {
      type: 'array',
      items: {
        type: 'object',
        required: ['symbol', 'kind', 'detail'],
        properties: {
          symbol: { type: 'string' },
          kind: { enum: ['stale-claim', 'wrong-param', 'missing-related', 'one-way-related', 'undocumented', 'other'] },
          detail: { type: 'string', description: 'what is wrong and what the code actually does, with src/ file:line' },
        },
      },
    },
  },
}

const HEADERS = {
  type: 'object', required: ['headers'],
  properties: { headers: { type: 'array', items: { type: 'string' } } },
}

phase('Scope')
let headers = (args && args.headers) || null
if (!headers || !headers.length) {
  const res = await agent(
    'List the public headers changed on this branch: run ' +
    '`git diff --name-only master...HEAD -- include/` and return the paths of ' +
    'hand-written cute_*.h files (exclude *_shd.h and cute_version.h).',
    { label: 'scope', phase: 'Scope', schema: HEADERS })
  headers = res ? res.headers : []
}
if (!headers.length) {
  log('No headers to audit.')
  return { audited: [], issues: [] }
}
log(`Auditing ${headers.length} header(s)`)

phase('Audit')
const results = await pipeline(headers, h =>
  agent(
    `Audit the documentation comments in ${h} against the real implementation. ` +
    `REPORT-ONLY: do not edit anything. For each documented symbol, read the ` +
    `implementation in src/ and check: does the doc describe actual behavior ` +
    `(stale claims)? are @param descriptions right? does @related exist and is ` +
    `it bidirectional? are there public symbols with no doc block at all? ` +
    `Echo the header path in the 'header' field. Cite src/ file:line in details.`,
    { label: `audit:${h}`, phase: 'Audit', agentType: 'doc-writer', schema: ISSUES }))

const issues = results.filter(Boolean).flatMap(r => (r.issues || []).map(i => ({ header: r.header, ...i })))
log(`${issues.length} issue(s) across ${headers.length} header(s)`)
return { audited: headers, issues }
