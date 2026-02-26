#include "include/cef_app.h"
#include "include/wrapper/cef_library_loader.h"

// Entry point function for sub-processes on macOS.
int main(int argc, char* argv[]) {
  // Load the CEF framework library at runtime.
  CefScopedLibraryLoader library_loader;
  if (!library_loader.LoadInHelper()) {
    return 1;
  }

  CefMainArgs main_args(argc, argv);

  // Execute the sub-process.
  return CefExecuteProcess(main_args, nullptr, nullptr);
}
