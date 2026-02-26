#include "app.h"

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"

#if defined(CEF_X11)
#include <X11/Xlib.h>

namespace {

int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
  LOG(WARNING) << "X error received: "
               << "type " << event->type << ", "
               << "serial " << event->serial << ", "
               << "error_code " << static_cast<int>(event->error_code) << ", "
               << "request_code " << static_cast<int>(event->request_code)
               << ", "
               << "minor_code " << static_cast<int>(event->minor_code);
  return 0;
}

int XIOErrorHandlerImpl(Display* display) {
  return 0;
}

}  // namespace
#endif  // defined(CEF_X11)

// Entry point function for all processes.
int main(int argc, char* argv[]) {
  CefMainArgs main_args(argc, argv);

  // Execute the sub-process logic, if any. This will return immediately for
  // the browser process.
  int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
  if (exit_code >= 0) {
    return exit_code;
  }

#if defined(CEF_X11)
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);
#endif

  CefSettings settings;
  settings.no_sandbox = true;

  CefRefPtr<SimpleApp> app(new SimpleApp);

  if (!CefInitialize(main_args, settings, app.get(), nullptr)) {
    return CefGetExitCode();
  }

  CefRunMessageLoop();
  CefShutdown();

  return 0;
}
