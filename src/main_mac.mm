#import <Cocoa/Cocoa.h>

#include "include/cef_application_mac.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_library_loader.h"
#include "app.h"
#include "handler.h"

// Receives notifications from the application.
@interface SimpleAppDelegate : NSObject <NSApplicationDelegate>
- (void)tryToTerminateApplication:(NSApplication*)app;
@end

// Provide the CefAppProtocol implementation required by CEF.
@interface SimpleApplication : NSApplication <CefAppProtocol> {
 @private
  BOOL handlingSendEvent_;
}
@end

@implementation SimpleApplication
- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event {
  CefScopedSendingEvent sendingEventScoper;
  [super sendEvent:event];
}

- (void)terminate:(id)sender {
  SimpleAppDelegate* delegate =
      static_cast<SimpleAppDelegate*>([NSApp delegate]);
  [delegate tryToTerminateApplication:self];
}
@end

@implementation SimpleAppDelegate

- (void)tryToTerminateApplication:(NSApplication*)app {
  SimpleHandler* handler = SimpleHandler::GetInstance();
  if (handler && !handler->IsClosing()) {
    handler->CloseAllBrowsers(false);
  }
}

- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication*)sender {
  return NSTerminateNow;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
  return YES;
}
@end

// Entry point function for the browser process.
int main(int argc, char* argv[]) {
  // Load the CEF framework library at runtime.
  CefScopedLibraryLoader library_loader;
  if (!library_loader.LoadInMain()) {
    return 1;
  }

  CefMainArgs main_args(argc, argv);

  @autoreleasepool {
    [SimpleApplication sharedApplication];
    CHECK([NSApp isKindOfClass:[SimpleApplication class]]);

    CefSettings settings;
    settings.no_sandbox = true;

    // Set cache path to avoid process singleton issues.
    NSString* appSupport = [NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory, NSUserDomainMask, YES) firstObject];
    NSString* cachePath =
        [appSupport stringByAppendingPathComponent:@"examshell"];
    CefString(&settings.root_cache_path) = [cachePath UTF8String];

    CefRefPtr<SimpleApp> app(new SimpleApp);

    if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
      return CefGetExitCode();
    }

    // Set the delegate for application events.
    SimpleAppDelegate* delegate = [[SimpleAppDelegate alloc] init];
    NSApp.delegate = delegate;

    CefRunMessageLoop();
    CefShutdown();

#if !__has_feature(objc_arc)
    [delegate release];
#endif
    delegate = nil;
  }  // @autoreleasepool

  return 0;
}
