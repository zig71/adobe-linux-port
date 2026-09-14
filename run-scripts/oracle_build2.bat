@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >NUL 2>&1
cd /d C:\AdobeCap
cl /nologo /EHsc /O2 probe_swapchain.cpp /link d3d11.lib dxgi.lib user32.lib gdi32.lib /OUT:probe_swapchain.exe
echo BUILD_RC=%ERRORLEVEL%
