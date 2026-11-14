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
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <climits>
#include <vector>

static void init_i18n() {
    setlocale(LC_ALL, "");

    const std::string &lang = Settings::instance().uiLanguage();
    if (!lang.empty()) {
        setenv("LANGUAGE", lang.c_str(), 1);
    }

    bindtextdomain(IRUKA_TEXTDOMAIN, utils::resolveLocaledir().c_str());
    bind_textdomain_codeset(IRUKA_TEXTDOMAIN, "UTF-8");
    textdomain(IRUKA_TEXTDOMAIN);
}

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  Settings::instance().load();

  init_i18n();

  if (!utils::hasBinary(XBPS_QUERY_BIN)) {
    std::cerr << _("Error: xbps-query not found at ") << XBPS_QUERY_BIN << std::endl;
    return 1;
  }
  if (!utils::hasBinary(PKEXEC_BIN)) {
    std::cerr << _("Warning: pkexec not found. Privileged operations may not work.") << std::endl;
  }

  // Let GTK pick the correct backend (Wayland or X11). Forcing DISPLAY to a
  // guessed :0 breaks native Wayland sessions with "cannot open display".
  gtk_init(&argc, &argv);

  theme::apply();

  MainWindow *win = new MainWindow();
  win->show();

  gtk_main();

  Settings::instance().save();
  delete win;
  return 0;
}
