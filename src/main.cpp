#include <FL/Fl.H>
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
  (void)argc;
  (void)argv;
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

  std::string x11Display = utils::detectX11Display();
  if (!x11Display.empty()) {
    setenv("DISPLAY", x11Display.c_str(), 1);
  }

  Fl::scheme("gtk+");
  theme::apply();

  // Initialise FLTK's threading support. Worker threads marshal their
  // results back to the UI thread through Fl::awake().
  Fl::lock();

  MainWindow *win = new MainWindow();
  win->show();

  Fl::run();

  Settings::instance().save();
  delete win;
  return 0;
}
