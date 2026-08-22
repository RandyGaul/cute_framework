# Validates every CAVLC VLC table in cute_h264.h against the codeword tables in the H.264
# specification, and checks each is prefix-free.
#
# This is not a style check. A wrong table entry is never a local symptom: the decoder reads a
# valid-but-wrong symbol, keeps going, and reports an impossible macroblock several macroblocks
# downstream. Most pictures survive it and some do not, so it hides behind a test suite that only
# checks a handful of sizes.
#
# The prefix-free check alone is not enough, and that is worth spelling out because it cost real
# time. A table can be perfectly prefix-free, have the correct Kraft sum, and still be wrong --
# one entry with the wrong length shifts the tail of a row into codewords that happen to be free,
# and every structural invariant still holds. The only way to catch that is to compare against the
# specified codewords, which is what CODEWORDS below is: Tables 9-4 to 9-7 (coeff_token), 9-16 and
# 9-17 (total_zeros) and 9-18 (run_before), transcribed as the bit strings the spec prints.
#
#   python tools/h264_table_check.py
import re, sys, os

# coeff_token, indexed [total_coeff][trailing_ones]. '-' marks a combination that cannot occur.
CT = {
'0': """1|-|-|-
000101|01|-|-
00000111|000100|001|-
000000111|00000110|0000101|00011
0000000111|000000110|00000101|000011
00000000111|0000000110|000000101|0000100
0000000001111|00000000110|0000000101|00000100
0000000001011|0000000001110|00000000101|000000100
0000000001000|0000000001010|0000000001101|0000000100
00000000001111|00000000001110|0000000001001|00000000100
00000000001011|00000000001010|00000000001101|0000000001100
000000000001111|000000000001110|00000000001001|00000000001100
000000000001011|000000000001010|000000000001101|00000000001000
0000000000001111|000000000000001|000000000001001|000000000001100
0000000000001011|0000000000001110|0000000000001101|000000000001000
0000000000000111|0000000000001010|0000000000001001|0000000000001100
0000000000000100|0000000000000110|0000000000000101|0000000000001000""",
'1': """11|-|-|-
001011|10|-|-
000111|00111|011|-
0000111|001010|001001|0101
00000111|000110|000101|0100
00000100|0000110|0000101|00110
000000111|00000110|00000101|001000
00000001111|000000110|000000101|000100
00000001011|00000001110|00000001101|0000100
000000001111|00000001010|00000001001|000000100
000000001011|000000001110|000000001101|00000001100
000000001000|000000001010|000000001001|00000001000
0000000001111|0000000001110|0000000001101|000000001100
0000000001011|0000000001010|0000000001001|0000000001100
0000000000111|00000000001011|0000000000110|0000000001000
00000000001001|00000000001000|00000000001010|0000000000001
00000000000111|00000000000110|00000000000101|00000000000100""",
'2': """1111|-|-|-
001111|1110|-|-
001011|01111|1101|-
001000|01100|01110|1100
0001111|01010|01011|1011
0001011|01000|01001|1010
0001001|001110|001101|1001
0001000|001010|001001|1000
00001111|0001110|0001101|01101
00001011|00001110|0001010|001100
000001111|00001010|00001101|0001100
000001011|000001110|00001001|00001100
000001000|000001010|000001101|00001000
0000001101|000000111|000001001|000001100
0000001001|0000001100|0000001011|0000001010
0000000101|0000001000|0000000111|0000000110
0000000001|0000000100|0000000011|0000000010""",
'C': """01|-|-|-
000111|1|-|-
000100|000110|001|-
000011|0000011|0000010|000101""",
}

# total_zeros, printed by the spec as rows = total_zeros, columns = total_coeff.
TZ = """1 111 0101 00011 0101 000001 000001 000001 000001 00001 0000 0000 000 00 0
011 110 111 111 0100 00001 00001 0001 000000 00000 0001 0001 001 01 1
010 101 110 0101 0011 111 101 00001 0001 001 001 01 1 1 -
0011 100 101 0100 111 110 100 011 11 11 010 1 01 - -
0010 011 0100 110 110 101 011 11 10 10 1 001 - - -
00011 0101 0011 101 101 100 11 10 001 01 011 - - - -
00010 0100 100 100 100 011 010 010 01 0001 - - - - -
000011 0011 011 0011 011 010 0001 001 00001 - - - - - -
000010 0010 0010 011 0010 0001 001 000000 - - - - - - -
0000011 00011 00011 0010 00001 001 000000 - - - - - - - -
0000010 00010 00010 00010 0001 000000 - - - - - - - - -
00000011 000011 000001 00001 00000 - - - - - - - - - -
00000010 000010 00001 00000 - - - - - - - - - - -
000000011 000001 000000 - - - - - - - - - - - -
000000010 000000 - - - - - - - - - - - - -
000000001 - - - - - - - - - - - - - -"""

TZC = """1 1 1
01 01 0
001 00 -
000 - -"""

# run_before, rows = run, columns = zeros_left (the last column covers every zeros_left above 6).
RB = """1 1 11 11 11 11 111
0 01 10 10 10 000 110
- 00 01 01 011 001 101
- - 00 001 010 011 100
- - - 000 001 010 011
- - - - 000 101 010
- - - - - 100 001
- - - - - - 0001
- - - - - - 00001
- - - - - - 000001
- - - - - - 0000001
- - - - - - 00000001
- - - - - - 000000001
- - - - - - 0000000001
- - - - - - 00000000001"""

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
    if bad:
        print('%-26s BROKEN' % label)
        for x in sorted(set(bad))[:6]: print('    ', x)
    return not bad

def against_spec(label, lens, codes, table, transpose):
    bad = 0
    for a, row in enumerate(r.split() if ' ' in r else r.split('|')
                            for r in table.strip().split('\n')):
        for b, word in enumerate(row):
            if word == '-': continue
            i, j = (b, a) if transpose else (a, b)
            want = (len(word), int(word, 2))
            got = (lens[i][j], codes[i][j])
            if got != want:
                bad += 1
                print('%-26s [%d][%d] is len=%d code=%d, spec says len=%d code=%d (%s)'
                      % (label, i, j, got[0], got[1], want[0], want[1], word))
    return bad == 0

ok = True
for n in '012':
    L, C = grab('ch_ct_len' + n), grab('ch_ct_code' + n)
    ok &= against_spec('coeff_token nC' + n, L, C, CT[n], True)
    ok &= prefix_free([(L[i][j], C[i][j], 't1=%d tc=%d' % (i, j))
                       for i in range(4) for j in range(17) if j >= i], 'coeff_token nC' + n)
L, C = grab('ch_ct_lenC'), grab('ch_ct_codeC')
ok &= against_spec('coeff_token chroma', L, C, CT['C'], True)
ok &= prefix_free([(L[i][j], C[i][j], 't1=%d tc=%d' % (i, j))
                   for i in range(4) for j in range(5) if j >= i], 'coeff_token chroma')
L, C = grab('ch_tz_len'), grab('ch_tz_code')
ok &= against_spec('total_zeros', L, C, TZ, True)
for i in range(15):
    ok &= prefix_free([(L[i][j], C[i][j], 'tz=%d' % j) for j in range(16 - i)], 'total_zeros tc=%d' % (i + 1))
L, C = grab('ch_tzc_len'), grab('ch_tzc_code')
ok &= against_spec('total_zeros chroma', L, C, TZC, True)
for i in range(3):
    ok &= prefix_free([(L[i][j], C[i][j], 'tz=%d' % j) for j in range(4 - i)], 'total_zeros chroma tc=%d' % (i + 1))
L, C = grab('ch_rb_len'), grab('ch_rb_code')
ok &= against_spec('run_before', L, C, RB, True)
for i in range(7):
    ok &= prefix_free([(L[i][j], C[i][j], 'run=%d' % j) for j in range(16)], 'run_before zl=%d' % (i + 1))

print('every CAVLC table matches the specification and is prefix-free' if ok
      else 'BROKEN TABLES -- see above')
sys.exit(0 if ok else 1)
