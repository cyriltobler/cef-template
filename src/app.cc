#include "app.h"

#include <string>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "handler.h"

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
 public:
  explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {}

  cef_runtime_style_t GetWindowRuntimeStyle() override {
    return CEF_RUNTIME_STYLE_ALLOY;
  }

  void OnWindowCreated(CefRefPtr<CefWindow> window) override {
    window->AddChildView(browser_view_);
    window->Show();
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) override {
    browser_view_ = nullptr;
  }

  bool CanClose(CefRefPtr<CefWindow> window) override {
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser) {
      return browser->GetHost()->TryCloseBrowser();
    }
    return true;
  }

  CefSize GetPreferredSize(CefRefPtr<CefView> view) override {
    return CefSize(800, 600);
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

class SimpleBrowserViewDelegate : public CefBrowserViewDelegate {
 public:
  SimpleBrowserViewDelegate() = default;

  bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view,
                                 CefRefPtr<CefBrowserView> popup_browser_view,
                                 bool is_devtools) override {
    CefWindow::CreateTopLevelWindow(
        new SimpleWindowDelegate(popup_browser_view));
    return true;
  }

  cef_runtime_style_t GetBrowserRuntimeStyle() override {
    return CEF_RUNTIME_STYLE_ALLOY;
  }

 private:
  IMPLEMENT_REFCOUNTING(SimpleBrowserViewDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleBrowserViewDelegate);
};

}  // namespace

SimpleApp::SimpleApp() = default;

void SimpleApp::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
  // Force Alloy style to get a simple single-window browser
  // without Chrome's tab strip and address bar.
  command_line->AppendSwitch("use-alloy-style");

#if defined(OS_MAC)
  // Disable the macOS keychain prompt.
  command_line->AppendSwitch("use-mock-keychain");
#endif
}

void SimpleApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler());

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;

  std::string url;

  // Check if a "--url=" value was provided via the command-line.
  url = command_line->GetSwitchValue("url");
  if (url.empty()) {
    url = "https://www.google.com";
  }

  // Create the BrowserView.
  CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(
      handler, url, browser_settings, nullptr, nullptr,
      new SimpleBrowserViewDelegate());

  // Create the Window.
  CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
}

CefRefPtr<CefClient> SimpleApp::GetDefaultClient() {
  // Return nullptr to prevent Chrome from creating a new tabbed window
  // when a second instance connects via process singleton.
  return nullptr;
}
