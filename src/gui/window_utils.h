#ifndef IRUKA_WINDOW_UTILS_H
#define IRUKA_WINDOW_UTILS_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include "i18n.h"
#include "privileged_daemon.h"

namespace gui_utils {

// Show a top-level window centred over `parent`, clamped to the visible
// work area. This keeps secondary windows on-screen under tiling window
// managers (common on Wayland/XWayland setups).
inline void showCentered(Fl_Window *win, Fl_Window *parent) {
  if (!win) return;

  if (parent) {
    int px = parent->x() + (parent->w() - win->w()) / 2;
    int py = parent->y() + (parent->h() - win->h()) / 2;

    int sx = 0, sy = 0, sw = 0, sh = 0;
    Fl::screen_work_area(sx, sy, sw, sh, px + win->w() / 2, py + win->h() / 2);

    if (sw > win->w()) {
      if (px < sx) px = sx;
      if (px + win->w() > sx + sw) px = sx + sw - win->w();
    } else {
      px = sx;
    }
    if (sh > win->h()) {
      if (py < sy) py = sy;
      if (py + win->h() > sy + sh) py = sy + sh - win->h();
    } else {
      py = sy;
    }

    win->position(px, py);
  }

  win->show();
  win->take_focus();
}

// Ask the user for the pkexec credentials required by privileged operations.
// Accepting the warning restarts the privileged helper so pkexec prompts
// again; this recovers from cancelled/expired authentications.
inline bool requestPkexecPrivileges() {
  int choice = fl_choice(
      "%s",
      _("Cancel"),
      _("OK"),
      nullptr,
      _("This operation requires pkexec permissions.\n\n"
        "Press OK to authenticate. The privileged helper will be restarted."));
  if (choice != 1) return false;

  PrivilegedDaemon &daemon = PrivilegedDaemon::instance();
  daemon.stop();
  daemon.start();
  return true;
}

}

#endif
