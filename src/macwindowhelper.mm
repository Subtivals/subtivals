// macwindowhelper.mm
#import "macwindowhelper.h"

#if defined(Q_OS_MAC)
#import <AppKit/AppKit.h>

void makeWindowCoverMenuBar(QWidget *widget) {
    if (!widget) return;

    NSView *nativeView = (__bridge NSView *)widget->winId();
    if (!nativeView) return;

    NSWindow *nsWindow = [nativeView window];
    if (!nsWindow) return;

    [nsWindow setLevel:NSMainMenuWindowLevel + 1];
    [nsWindow setStyleMask:NSWindowStyleMaskBorderless];
}
#endif
