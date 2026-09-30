"""Compile a documentation-only main against the exact validated objects.
No plug-in target, installed binary, or production source is modified.
"""
from pathlib import Path
import re,subprocess,json
HERE=Path(__file__).resolve().parent
BUILD=Path(r'D:\Codex\Temp\QQSC1234-Review-Fixes')
OUT=Path(r'D:\Codex\Temp\QQSCManual1236');OUT.mkdir(parents=True,exist_ok=True)
TLOG=BUILD/'QQSCUnityCheck.dir/Release/QQSCUnityCheck.tlog'
lines=(TLOG/'CL.command.1.tlog').read_text(encoding='utf-16').splitlines()
command=next(lines[i+1] for i,line in enumerate(lines) if line.startswith('^') and line.endswith('UNITY_CHECK.CPP'))
command=re.sub(r'/Fo"[^"]*"','/Fo"'+str(OUT/'capture.obj').replace('\\','/')+'"',command)
command=re.sub(r'/Fd"[^"]*"','/Fd"'+str(OUT/'capture-compile.pdb').replace('\\','/')+'"',command)
command=re.sub(r'D:\\CODEX\\WORKSPACES\\QQSC-1\.2\.34-REVIEW-FIXES\\TESTS\\UNITY_CHECK.CPP$',lambda _: '"'+str(HERE/'manual_capture_1_2_36.cpp')+'"',command,flags=re.I)
(OUT/'compile.rsp').write_text(command,encoding='utf-8')
link='\n'.join((TLOG/'link.command.1.tlog').read_text(encoding='utf-16').splitlines()[1:])
for flag,name in [('OUT','manual-capture.exe'),('PDB','capture-link.pdb'),('IMPLIB','capture.lib')]:
    link=re.sub('/'+flag+r':"[^"]*"','/'+flag+':"'+str(OUT/name).replace('\\','/')+'"',link)
link=link.replace('QQSCUNITYCHECK.DIR\\RELEASE\\UNITY_CHECK.OBJ','"'+str(OUT/'capture.obj')+'"')
(OUT/'link.rsp').write_text(link,encoding='utf-8')
cmd=OUT/'build-capture.cmd'
cmd.write_text('@echo off\r\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\Tools\\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul\r\n'
    +'set TEMP=D:\\Codex\\Temp\\QQSC-Toolchain-Temp\r\nset TMP=D:\\Codex\\Temp\\QQSC-Toolchain-Temp\r\n'
    +'cd /d "'+str(BUILD)+'"\r\ncl @"'+str(OUT/'compile.rsp')+'"\r\nif errorlevel 1 exit /b %errorlevel%\r\nlink @"'+str(OUT/'link.rsp')+'"\r\nexit /b %errorlevel%\r\n',encoding='utf-8')
subprocess.run([str(cmd)],check=True)
subprocess.run([str(OUT/'manual-capture.exe'),str(HERE/'assets')],check=True)
