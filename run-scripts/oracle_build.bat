@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
cd /d C:\AdobeCap
cl /nologo /EHsc /O2 probe_d3d11.cpp /link d3d11.lib dxgi.lib /OUT:probe_d3d11.exe
echo BUILD_RC=%ERRORLEVEL%
