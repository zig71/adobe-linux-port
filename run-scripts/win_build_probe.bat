@echo off
setlocal
set LAB=C:\Users\winnie\adobe-wine-lab
set VSW="C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`%VSW% -latest -property installationPath`) do set VSDIR=%%i
echo VSINSTALL=%VSDIR%
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" (
  echo VCVARS_MISSING
  exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 ( echo VCVARS_FAILED & exit /b 1 )
where cl
cl /nologo /W3 /O2 /Fe:"%LAB%\tests\build\hello_win32.exe" /Fo:"%LAB%\tests\build\\" "%LAB%\tests\hello_win32.c"
cl /nologo /W3 /O2 /Fe:"%LAB%\tests\build\probe_gpu.exe" /Fo:"%LAB%\tests\build\\" "%LAB%\tests\probe_gpu.c"
echo BUILD_DONE
