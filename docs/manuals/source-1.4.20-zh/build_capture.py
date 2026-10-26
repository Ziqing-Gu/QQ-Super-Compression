from pathlib import Path
import re,subprocess,sys,argparse
a=argparse.ArgumentParser()
a.add_argument("--build", type=Path, required=True, help="Existing validated MSVC build with QQSCVisualCheck command logs")
a.add_argument("--output", type=Path, required=True, help="Temporary capture build directory")
args=a.parse_args()
HERE=Path(__file__).resolve().parent
BUILD=args.build.resolve()
OUT=args.output.resolve()
OUT.mkdir(parents=True,exist_ok=True)
cpp=HERE/'manual_capture_1_4_20.cpp'
tlog=BUILD/'QQSCVisualCheck.dir/Release/QQSCVisualCheck.tlog'
lines=(tlog/'CL.command.1.tlog').read_text(encoding='utf-16').splitlines()
command=next(lines[i+1] for i,line in enumerate(lines) if line.startswith('^') and line.endswith('VISUAL_CHECK.CPP'))
command=re.sub(r'/Fo"[^"]*"',lambda _: '/Fo"'+str(OUT/'capture.obj')+'"',command)
command=re.sub(r'/Fd"[^"]*"',lambda _: '/Fd"'+str(OUT/'capture.pdb')+'"',command)
command=re.sub(r'[^ ]+\\TESTS\\VISUAL_CHECK.CPP$',lambda _: '"'+str(cpp)+'"',command,flags=re.I)
(OUT/'compile.rsp').write_text(command,encoding='utf-8')
link='\n'.join((tlog/'link.command.1.tlog').read_text(encoding='utf-16').splitlines()[1:])
for flag,name in [('OUT','manual-capture.exe'),('PDB','capture-link.pdb'),('IMPLIB','capture.lib')]:
    link=re.sub('/'+flag+r':"[^"]*"',lambda _,name=name:'/'+flag+':"'+str(OUT/name)+'"',link)
link=link.replace('QQSCVISUALCHECK.DIR\\RELEASE\\VISUAL_CHECK.OBJ','"'+str(OUT/'capture.obj')+'"')
(OUT/'link.rsp').write_text(link,encoding='utf-8')
cmd=OUT/'build-capture.cmd'
cmd.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul\nset TEMP='+str(OUT)+'\nset TMP='+str(OUT)+'\ncd /d "'+str(BUILD)+'"\ncl @"'+str(OUT/'compile.rsp')+'"\nif errorlevel 1 exit /b %errorlevel%\nlink @"'+str(OUT/'link.rsp')+'"\nexit /b %errorlevel%\n',encoding='utf-8')
with (OUT/'capture-build.log').open('w') as f: subprocess.run([str(cmd)],stdout=f,stderr=subprocess.STDOUT,check=True)
with (OUT/'capture.log').open('w') as f: subprocess.run([str(OUT/'manual-capture.exe'),str(HERE/'assets')],stdout=f,stderr=subprocess.STDOUT,check=True)
print('Current production UI captures complete:',len(list((HERE/'assets').glob('*.png'))))
