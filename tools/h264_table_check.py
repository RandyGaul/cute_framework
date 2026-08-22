# Validates every CAVLC VLC table in cute_h264.h is prefix-free.
#
# This is not a style check. A VLC table with a duplicate or a code that is a prefix of another
# is not decodable, and the symptom is never local: the decoder reads a valid-but-wrong symbol,
# keeps going, and reports an impossible macroblock type several macroblocks downstream. Three
# real table errors were found with this that were invisible to reading the code, and each had
# been silently producing a stream that most pictures survived and some did not.
#
#   python tools/h264_table_check.py
import re, sys, os

src = open(os.path.join(os.path.dirname(__file__), '..', 'libraries', 'cute', 'cute_h264.h')).read()

def grab(name):
    m = re.search(re.escape(name) + r'\s*\[\s*\d+\s*\]\s*\[\s*\d+\s*\]\s*=\s*\{(.*?)\};', src, re.S)
    if not m: sys.exit('table not found: ' + name)
    return [[int(x) for x in r.replace('\n', ' ').split(',') if x.strip()]
            for r in re.findall(r'\{([^}]*)\}', m.group(1))]

def prefix_free(entries, label):
    bits = [(format(c, '0%db' % l), tag) for (l, c, tag) in entries if l]
    bad = ['%s "%s" is a prefix of %s "%s"' % (a[1], a[0], b[1], b[0])
           for a in bits for b in bits if a is not b and b[0].startswith(a[0])]
    print('%-24s %s' % (label, 'ok (%d codes)' % len(bits) if not bad else 'BROKEN'))
    for x in sorted(set(bad))[:6]: print('    ', x)
    return not bad

ok = True
for n in '012':
    L, C = grab('ch_ct_len' + n), grab('ch_ct_code' + n)
    ok &= prefix_free([(L[i][j], C[i][j], 't1=%d tc=%d' % (i, j))
                       for i in range(4) for j in range(17) if j >= i], 'coeff_token nC' + n)
L, C = grab('ch_ct_lenC'), grab('ch_ct_codeC')
ok &= prefix_free([(L[i][j], C[i][j], 't1=%d tc=%d' % (i, j))
                   for i in range(4) for j in range(5) if j >= i], 'coeff_token chroma')
L, C = grab('ch_tz_len'), grab('ch_tz_code')
for i in range(15):
    ok &= prefix_free([(L[i][j], C[i][j], 'tz=%d' % j) for j in range(16 - i)], 'total_zeros tc=%d' % (i + 1))
L, C = grab('ch_tzc_len'), grab('ch_tzc_code')
for i in range(3):
    ok &= prefix_free([(L[i][j], C[i][j], 'tz=%d' % j) for j in range(4 - i)], 'total_zeros chroma tc=%d' % (i + 1))
L, C = grab('ch_rb_len'), grab('ch_rb_code')
for i in range(7):
    ok &= prefix_free([(L[i][j], C[i][j], 'run=%d' % j) for j in range(16)], 'run_before zl=%d' % (i + 1))

print('\nall tables prefix-free' if ok else '\nBROKEN TABLES -- see above')
sys.exit(0 if ok else 1)
