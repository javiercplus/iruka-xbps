#include <gtk/gtk.h>
#include "main_window.h"
#include "constants.h"
#include "settings.h"
#include "utils.h"
#include "theme.h"
#include "i18n.h"
#include <clocale>
#include <iostream>
#include <cstdlib>

// Configures gettext so every _() call is translated at runtime:
// - respects the system locale (LANG/LC_ALL/LANGUAGE) by default;
// - honors an explicit ui_language preference saved in the config file;
// - loads the message catalogs from LOCALEDIR (or IRUKA_LOCALEDIR when set).
static void init_i18n() {
    setlocale(LC_ALL, "");

    const std::string &lang = Settings::instance().uiLanguage();
    if (!lang.empty()) {
        // LANGUAGE overrides LC_ALL/LC_MESSAGES for gettext (and GTK stock labels).
        setenv("LANGUAGE", lang.c_str(), 1);
    }

    // Locate the message catalogs (installed dir, $IRUKA_LOCALEDIR, or the
    // build tree when running straight from builddir) and bind gettext.
    bindtextdomain(IRUKA_TEXTDOMAIN, utils::resolveLocaledir().c_str());
    bind_textdomain_codeset(IRUKA_TEXTDOMAIN, "UTF-8");
    textdomain(IRUKA_TEXTDOMAIN);
}

int main(int argc, char *argv[]) {
  // Load persisted user preferences (window size, column widths, view filter).
  // Must happen before i18n setup so the saved language preference is honored.
  Settings::instance().load();

  // Initialize gettext and select the interface language.
  init_i18n();

  // Ensure the required xbps-query binary is available before proceeding.
  if (!utils::hasBinary(XBPS_QUERY_BIN)) {
    std::cerr << _("Error: xbps-query not found at ") << XBPS_QUERY_BIN << std::endl;
    return 1;
  }
  // Warn if pkexec is missing; privileged operations (install/remove) will fail.
  if (!utils::hasBinary(PKEXEC_BIN)) {
    std::cerr << _("Warning: pkexec not found. Privileged operations may not work.") << std::endl;
  }

  // Detect and set the X11 display if not already configured in the environment.
  std::string x11Display = utils::detectX11Display();
  if (!x11Display.empty()) {
    setenv("DISPLAY", x11Display.c_str(), 1);
  }

  // Initialize the GTK toolkit.
  gtk_init(&argc, &argv);

  // Apply custom CSS styling before any windows are created.
  theme::apply();

  // Create and show the main application window.
  MainWindow *win = new MainWindow();
  win->show();

  // Enter the GTK event loop; this call blocks until gtk_main_quit() is called.
  gtk_main();

  // Persist settings to disk before exit.
  Settings::instance().save();
  delete win;
  return 0;
}
