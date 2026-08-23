import subprocess,os,sys
SIZES=[(176,144),(320,176),(640,360)]
QPS=[14,26,38]
bad=[];n=0
for refs in (1,2,3,4):
    for cab in (0,1):
        for (w,h) in SIZES:
            for qp in QPS:
                n+=1
                e=dict(os.environ); e['REFS']=str(refs)
                if cab: e['CABAC']='1'
                else: e.pop('CABAC',None)
                subprocess.run(['./b/Release/h264test.exe',str(w),str(h),'8',str(qp),'0'],capture_output=True,env=e)
                tag='refs%d %s %dx%d@%d'%(refs,'cabac' if cab else 'cavlc',w,h,qp)
                r=subprocess.run(['ffmpeg','-v','error','-i','out.264','-f','rawvideo','-pix_fmt','yuv420p','-y','d.yuv'],capture_output=True,text=True)
                if r.stderr.strip(): bad.append(tag+':ffmpeg'); continue
                ref=open('ref.yuv','rb').read()
                if ref!=open('d.yuv','rb').read(): bad.append(tag+':ffmpeg differs'); continue
                o=subprocess.run(['./b/Release/h264dec.exe','out.264','ours.yuv'],capture_output=True,text=True)
                if o.returncode!=0: bad.append(tag+':ours '+o.stdout.strip()[:40]); continue
                if ref!=open('ours.yuv','rb').read(): bad.append(tag+':ours differs')
print('%d/%d bit-exact against ffmpeg AND our own decoder'%(n-len(bad),n))
for x in bad[:10]: print('  FAIL',x)
