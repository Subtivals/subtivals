// macwindowhelper.mm
#import "macwindowhelper.h"

#if defined(Q_OS_MAC)
#import <AppKit/AppKit.h>

void makeWindowCoverMenuBar(QWidget *widget, bool state) {
    if (!widget) return;

    NSView *nativeView = (__bridge NSView *)widget->winId();
    if (!nativeView) return;

    NSWindow *nsWindow = [nativeView window];
    if (!nsWindow) return;

    static NSMutableDictionary *originalMasks;
    if (!originalMasks)
        originalMasks = [NSMutableDictionary dictionary];

    NSNumber *winKey = @((uintptr_t)nsWindow);

    if (state) {
        if (![originalMasks objectForKey:winKey]) {
            [originalMasks setObject:@([nsWindow styleMask]) forKey:winKey];
        }
        [nsWindow setLevel:NSMainMenuWindowLevel + 1];
        [nsWindow setStyleMask:NSWindowStyleMaskBorderless];
    } else {
        NSNumber *mask = [originalMasks objectForKey:winKey];
        if (mask) {
            [nsWindow setStyleMask:[mask unsignedLongLongValue]];
            [originalMasks removeObjectForKey:winKey];
        }
        [nsWindow setLevel:NSNormalWindowLevel];
    }
}
#endif