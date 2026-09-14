/* probe_metrics.c — window manager metrics catalogue.
 *
 * Why: the Creative Cloud installer sizes its window and decides what layout to
 * use from GetSystemMetrics. On Windows it logs
 *     ViewMediatorUIWin | inside onWindowResize, possible snapped window case,
 *                         width 1016 height 669
 * and only then starts its workflow; under Wine it never reaches that point.
 *
 * Probe window.c showed message delivery is identical on both platforms, so the
 * difference is not missing WM_SIZE. What does differ is the metrics: Wine
 * reported SM_CYFULLSCREEN = 786 for a 768-high screen, which is larger than the
 * screen itself. Wine derives it as
 *     SM_CYFULLSCREEN = SM_CYMAXIMIZED - SM_CYMIN     (dlls/win32u/sysparams.c)
 * so this probe dumps the whole metric set to locate the disagreement exactly.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_metrics.exe probe_metrics.c -luser32
 */
#include <windows.h>
#include <stdio.h>

struct metric { const char *name; int index; };

static const struct metric metrics[] = {
    { "SCREEN",            SM_CXSCREEN },
    { "SCREEN_Y",          SM_CYSCREEN },
    { "MAXIMIZED",         SM_CXMAXIMIZED },
    { "MAXIMIZED_Y",       SM_CYMAXIMIZED },
    { "FULLSCREEN",        SM_CXFULLSCREEN },
    { "FULLSCREEN_Y",      SM_CYFULLSCREEN },
    { "MIN",               SM_CXMIN },
    { "MIN_Y",             SM_CYMIN },
    { "SIZEFRAME",         SM_CXSIZEFRAME },
    { "SIZEFRAME_Y",       SM_CYSIZEFRAME },
    { "FRAME",             SM_CXFRAME },
    { "FRAME_Y",           SM_CYFRAME },
    { "PADDEDBORDER",      SM_CXPADDEDBORDER },
    { "CAPTION",           SM_CYCAPTION },
    { "BORDER",            SM_CXBORDER },
    { "BORDER_Y",          SM_CYBORDER },
    { "EDGE",              SM_CXEDGE },
    { "EDGE_Y",            SM_CYEDGE },
    { "MINTRACK",          SM_CXMINTRACK },
    { "MINTRACK_Y",        SM_CYMINTRACK },
    { "MAXTRACK",          SM_CXMAXTRACK },
    { "MAXTRACK_Y",        SM_CYMAXTRACK },
    { "ICON",              SM_CXICON },
    { "ICON_Y",            SM_CYICON },
    { "SMICON",            SM_CXSMICON },
    { "SMICON_Y",          SM_CYSMICON },
    { "MENU_Y",            SM_CYMENU },
    { "MENUCHECK",         SM_CXMENUCHECK },
    { "MENUCHECK_Y",       SM_CYMENUCHECK },
    { "SMSIZE",            SM_CXSMSIZE },
    { "SMSIZE_Y",          SM_CYSMSIZE },
    { "VSCROLL",           SM_CXVSCROLL },
    { "HSCROLL",           SM_CYHSCROLL },
    { "CAPTIONBUTTON",     SM_CXSIZE },
    { "CAPTIONBUTTON_Y",   SM_CYSIZE },
    { "SMCAPTION",         SM_CXSMSIZE },
    { "DOUBLECLK",         SM_CXDOUBLECLK },
    { "DOUBLECLK_Y",       SM_CYDOUBLECLK },
    { "DRAG",              SM_CXDRAG },
    { "DRAG_Y",            SM_CYDRAG },
    { "ICONSPACING",       SM_CXICONSPACING },
    { "ICONSPACING_Y",     SM_CYICONSPACING },
    { "MONITORS",          SM_CMONITORS },
    { "VIRTUALSCREEN",     SM_CXVIRTUALSCREEN },
    { "VIRTUALSCREEN_Y",   SM_CYVIRTUALSCREEN },
    { "VIRTUALSCREEN_X",   SM_XVIRTUALSCREEN },
    { "VIRTUALSCREEN_Y0",  SM_YVIRTUALSCREEN },
    { "CURSOR",            SM_CXCURSOR },
    { "CURSOR_Y",          SM_CYCURSOR },
    { "NETWORK",           SM_NETWORK },
    { "CMETRICS",          SM_CMETRICS },
};

int main(void) {
    HDC screen;
    int i;
    UINT dpi;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    for (i = 0; i < (int)(sizeof(metrics) / sizeof(metrics[0])); i++)
        printf("M_%s=%d\n", metrics[i].name, GetSystemMetrics(metrics[i].index));

    screen = GetDC(NULL);
    if (screen) {
        printf("DEVICE_horzres=%d\n", GetDeviceCaps(screen, HORZRES));
        printf("DEVICE_vertres=%d\n", GetDeviceCaps(screen, VERTRES));
        printf("DEVICE_horzsize=%d\n", GetDeviceCaps(screen, HORZSIZE));
        printf("DEVICE_vertsize=%d\n", GetDeviceCaps(screen, VERTSIZE));
        printf("DEVICE_logpixelsx=%d\n", GetDeviceCaps(screen, LOGPIXELSX));
        printf("DEVICE_logpixelsy=%d\n", GetDeviceCaps(screen, LOGPIXELSY));
        printf("DEVICE_desktop_h=%d\n", GetDeviceCaps(screen, DESKTOPHORZRES));
        printf("DEVICE_desktop_v=%d\n", GetDeviceCaps(screen, DESKTOPVERTRES));
        ReleaseDC(NULL, screen);
    } else {
        printf("DEVICE_horzres=no_dc\n");
    }

    dpi = GetDpiForSystem();
    printf("DPI_system=%u\n", (unsigned)dpi);
    printf("DPI_scaled_caption=%d\n", GetSystemMetricsForDpi(SM_CYCAPTION, dpi));
    printf("DPI_scaled_frame=%d\n", GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi));

    {
        RECT wa;
        if (SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0))
            printf("WORKAREA=%ld,%ld,%ld,%ld size=%ldx%ld\n",
                   (long)wa.left, (long)wa.top, (long)wa.right, (long)wa.bottom,
                   (long)(wa.right - wa.left), (long)(wa.bottom - wa.top));
        else
            printf("WORKAREA=unavailable\n");
    }

    /* How Windows relates the derived fullscreen metric to the screen. */
    printf("DERIVED_fullscreen_minus_screen=%d\n",
           GetSystemMetrics(SM_CYFULLSCREEN) - GetSystemMetrics(SM_CYSCREEN));
    printf("DERIVED_screen_minus_caption=%d\n",
           GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYCAPTION));

    printf("PROBE_RESULT=OK\n");
    return 0;
}
