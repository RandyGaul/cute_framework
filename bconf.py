import os, subprocess, sys
SIZES = [(16,16),(64,64),(176,144),(300,170),(352,288),(640,360),(854,480),(1280,720)]
QPS = [0,6,12,18,24,30,36,42,48,51]
bad = []; n = 0
for (w,h) in SIZES:
    for qp in QPS:
        for flat in (0,1,2):
            n += 1
            env = dict(os.environ); env['BFRAMES'] = '1'
            for a in sys.argv[1:]:
                if a == 'cabac': env['CABAC'] = '1'
                else: env['REFS'] = a
            subprocess.run(['./b/Release/h264test.exe',str(w),str(h),'9',str(qp),str(flat)],
                           capture_output=True, env=env)
            tag = '%dx%d@%d/%d' % (w,h,qp,flat)
            r = subprocess.run(['ffmpeg','-v','error','-i','out.264','-f','rawvideo',
                                '-pix_fmt','yuv420p','-y','dec.yuv'], capture_output=True, text=True)
            if r.stderr.strip(): bad.append(tag+':ffmpeg '+r.stderr.strip()[:50]); continue
            a = open('ref.yuv','rb').read(); b = open('dec.yuv','rb').read()
            if a != b:
                d = sum(1 for x,y in zip(a,b) if x != y)
                bad.append('%s:%d/%d bytes differ' % (tag,d,len(a))); continue
            r = subprocess.run(['./b/Release/h264dec.exe','out.264','ours.yuv'], capture_output=True, text=True)
            if r.returncode != 0: bad.append(tag+':ours '+r.stdout.strip()[:40]); continue
            if open('ours.yuv','rb').read() != a: bad.append(tag+':ours differs')
print('%d/%d B-picture streams (%s) bit-exact against ffmpeg AND our own decoder' % (n-len(bad), n, ' '.join(sys.argv[1:]) or 'default'))
for x in bad[:15]: print('  FAIL', x)
