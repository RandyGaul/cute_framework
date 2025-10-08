import pathlib
import re

ROOT = pathlib.Path('include')

overrides = {
	'sdyna': ['sset', 'sfree', 'smake'],
	'slen': ['scount', 'sempty', 'sset'],
	'sempty': ['slen', 'scount', 'sset'],
	'spush': ['spop', 'sset', 'sfit'],
	'sfree': ['sset', 'smake', 'sdyna'],
	'scount': ['slen', 'scap', 'sempty'],
	'scap': ['sfit', 'slen', 'sset'],
	'sfirst': ['slast', 'spush', 'spop'],
	'slast': ['sfirst', 'spush', 'spop'],
	'sclear': ['sset', 'sfree', 'spush'],
	'sfit': ['scap', 'spush', 'slen'],
	'sfmt': ['sfmt_append', 'svfmt', 'sset'],
	'sfmt_append': ['sfmt', 'svfmt_append', 'sappend'],
	'svfmt': ['sfmt', 'svfmt_append', 'sset'],
	'svfmt_append': ['sfmt_append', 'sfmt', 'svfmt'],
	'sset': ['sdup', 'smake', 'sfree'],
	'sdup': ['sset', 'smake', 'sfree'],
	'smake': ['sdup', 'sset', 'sfree'],
	'scmp': ['sequ', 'sicmp', 'siequ'],
	'sicmp': ['siequ', 'scmp', 'sequ'],
	'sequ': ['scmp', 'siequ', 'sicmp'],
	'siequ': ['sicmp', 'sequ', 'scmp'],
	'sprefix': ['ssuffix', 'scontains', 'sfind'],
	'ssuffix': ['sprefix', 'scontains', 'sfind'],
	'scontains': ['sfind', 'sprefix', 'ssuffix'],
	'stoupper': ['stolower', 'sicmp', 'siequ'],
	'stolower': ['stoupper', 'sicmp', 'siequ'],
	'sappend': ['scat', 'sappend_range', 'sfmt_append'],
	'scat': ['sappend', 'scat_range', 'sfmt_append'],
	'sappend_range': ['sappend', 'scat_range', 'sfmt_append'],
	'scat_range': ['scat', 'sappend_range', 'sfmt_append'],
	'sreplace': ['serase', 'sfind', 'sinsert'],
	'serase': ['sreplace', 'spop', 'spopn'],
	'sinsert': ['sreplace', 'sfind', 'sappend'],
	'ssplit_once': ['ssplit', 'sfind', 'scontains'],
	'ssplit': ['ssplit_once', 'sfind', 'scontains'],
	'sfind': ['scontains', 'sfirst_index_of', 'slast_index_of'],
	'sfirst_index_of': ['sfind', 'slast_index_of', 'scontains'],
	'slast_index_of': ['sfind', 'sfirst_index_of', 'scontains'],
	'sdecode_UTF8': ['sappend_UTF8', 'cf_decode_UTF8', 'cf_decode_UTF16'],
	'sdecode_UTF16': ['sappend_UTF8', 'cf_decode_UTF8', 'cf_decode_UTF16'],
	'sappend_UTF8': ['cf_decode_UTF8', 'cf_decode_UTF16', 'sdecode_UTF8'],
	'cf_core_count': ['cf_cacheline_size'],
}

def common_prefix_length(a: str, b: str) -> int:
	count = 0
	for ca, cb in zip(a.lower(), b.lower()):
		if ca != cb:
			break
		count += 1
	return count

def shared_segments(a: str, b: str) -> int:
	a_parts = a.split('_')
	b_parts = b.split('_')
	count = 0
	for ap, bp in zip(a_parts, b_parts):
		if ap != bp:
			break
		count += 1
	return count

def base_name(name: str) -> str:
	parts = name.split('_')
	if name.startswith('cf_') or name.startswith('CF_'):
		if len(parts) >= 2:
			return '_'.join(parts[:2])
	match = re.match(r'^[a-zA-Z]+', name)
	return match.group(0) if match else name

def curate_related(name: str, tokens: list[str]) -> list[str]:
	tokens = list(dict.fromkeys(tokens))
	override = overrides.get(name)
	if override:
		return override[:3]
	base = base_name(name)
	func_candidates = []
	type_candidates = []
	for idx, token in enumerate(tokens):
		if token == name:
			continue
		score = 0
		score += shared_segments(name, token) * 140
		score += common_prefix_length(name, token) * 6
		if token.lower().startswith(base.lower()):
			score += 80
		if token.startswith('CF_'):
			type_candidates.append((score + 200, idx, token))
		else:
			func_candidates.append((score, idx, token))
	func_candidates.sort(key=lambda item: (-item[0], item[1]))
	type_candidates.sort(key=lambda item: (-item[0], item[1]))
	curated: list[str] = []
	for score, idx, token in func_candidates:
		if token not in curated:
			curated.append(token)
		if len(curated) == 3:
			return curated
	for score, idx, token in type_candidates:
		if token not in curated:
			curated.append(token)
		if len(curated) == 3:
			return curated
	allow_self = not curated
	for token in tokens:
		if token in curated:
			continue
		if token == name and not allow_self:
			continue
		curated.append(token)
		allow_self = False
		if len(curated) == 3:
			break
	return curated

def process_file(path: pathlib.Path) -> None:
	lines = path.read_text().splitlines()
	changed = False
	current_name = None
	for i, line in enumerate(lines):
		match = re.search(r'\* @(function|struct|enum|typedef|macro)\s+(\w+)', line)
		if match:
			current_name = match.group(2)
		match = re.search(r'\* @related\s+(.*)', line)
		if match and current_name:
			tokens = match.group(1).split()
			curated = curate_related(current_name, tokens)
			new_line = f" * @related  {' '.join(curated)}"
			if new_line != line:
				lines[i] = new_line
				changed = True
	if changed:
		path.write_text('\n'.join(lines) + '\n')

def main() -> None:
	for path in ROOT.rglob('*.h'):
		process_file(path)

if __name__ == '__main__':
	main()
