@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
cd /d C:\AdobeCap
cl /nologo /EHsc /O2 probe_pipe_async.c
echo BUILD_RC=%ERRORLEVEL%
