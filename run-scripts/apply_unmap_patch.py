#!/usr/bin/env python3
# Apply the Unmap/BadWindow ignore patch to x11drv_main.c (idempotent).
import sys
p = '/home/kubuntu/adobe-wine-lab/src/wine/dlls/winex11.drv/x11drv_main.c'
s = open(p).read()
old = """    if ((event->request_code == X_SetInputFocus ||
         event->request_code == X_ChangeWindowAttributes ||
         event->request_code == X_ConfigureWindow ||
         event->request_code == X_SendEvent) &&
        (event->error_code == BadMatch ||
         event->error_code == BadWindow)) return TRUE;"""
new = """    if ((event->request_code == X_SetInputFocus ||
         event->request_code == X_ChangeWindowAttributes ||
         event->request_code == X_ConfigureWindow ||
         event->request_code == X_SendEvent ||
         event->request_code == X_UnmapWindow) &&
        (event->error_code == BadMatch ||
         event->error_code == BadWindow)) return TRUE;"""
if new in s:
    print('ALREADY_PATCHED')
elif old not in s:
    print('PATTERN_MISSING'); sys.exit(1)
else:
    open(p, 'w').write(s.replace(old, new, 1))
    print('PATCHED')
