#include <windows.h>
#include <shlobj.h>

#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "app.h"

// Entry point function for all processes.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  void* sandbox_info = nullptr;

  CefMainArgs main_args(hInstance);

  int exit_code = CefExecuteProcess(main_args, nullptr, sandbox_info);
  if (exit_code >= 0) {
    return exit_code;
  }

  CefSettings settings;
  settings.no_sandbox = true;

  // Set cache path to avoid process singleton issues.
  wchar_t app_data[MAX_PATH];
  if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, app_data) ==
      S_OK) {
    std::wstring cache_path = std::wstring(app_data) + L"\\examshell";
    CefString(&settings.root_cache_path) = cache_path;
  }

  CefRefPtr<SimpleApp> app(new SimpleApp);

  if (!CefInitialize(main_args, settings, app.get(), sandbox_info)) {
    return CefGetExitCode();
  }

  CefRunMessageLoop();
  CefShutdown();

  return 0;
}
