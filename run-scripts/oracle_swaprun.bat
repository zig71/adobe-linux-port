@echo off
C:\AdobeCap\probe_swapchain.exe > C:\AdobeCap\swap-oracle.txt 2>&1
echo RC=%ERRORLEVEL% >> C:\AdobeCap\swap-oracle.txt
