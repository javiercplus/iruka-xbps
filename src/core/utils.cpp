#include "utils.h"
#include "constants.h"
#include "i18n.h"
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <unistd.h>
#include <dirent.h>
#include <algorithm>

namespace utils {

// Returns the directory that contains the gettext message catalogs, searching:
//   1. $IRUKA_LOCALEDIR — explicit override (e.g. staging or packaging tests);
//   2. <exedir>/po      — the meson build tree, so running straight from
//                         builddir picks up the freshly compiled .mo files;
//   3. LOCALEDIR        — the configured install location (e.g. /usr/share/locale).
std::string resolveLocaledir() {
  const char *env = getenv("IRUKA_LOCALEDIR");
  if (env && env[0]) return env;

  // Resolve the real executable path (handles symlinks) so the build-tree
  // layout is found regardless of the current working directory.
  char buf[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    std::string exe(buf);
    std::string dir = exe.substr(0, exe.find_last_of('/'));
    std::string po = dir + "/po";
    // Only use the build tree if a catalog actually exists there.
    if (access((po + "/es/LC_MESSAGES/" + IRUKA_TEXTDOMAIN + ".mo").c_str(), R_OK) == 0)
      return po;
  }

  return LOCALEDIR;
}

// Returns the path to the application config directory (~/.config/iruka-xbps).
std::string getConfigDir() {
  const char *home = getenv("HOME");
  if (!home) home = "/tmp";
  return std::string(home) + "/" + IRUKA_CONFIG_DIR_NAME;
}

// Checks for internet connectivity by pinging voidlinux.org with a 2-second timeout.
bool hasInternetConnection() {
  FILE *pipe = popen("ping -c 1 -W 2 voidlinux.org 2>/dev/null", "r");
  if (!pipe) return false;
  int ret = pclose(pipe);
  return (ret == 0);
}

// Returns true if the given file path exists and is executable.
bool hasBinary(const std::string &path) {
  return access(path.c_str(), X_OK) == 0;
}

// Returns the user's login shell from $SHELL, defaulting to /bin/sh.
std::string getShell() {
  const char *shell = getenv("SHELL");
  if (!shell) shell = "/bin/sh";
  return shell;
}

// Detects the active X11 display socket to use.
// First checks the DISPLAY environment variable; if the socket exists, uses it.
// Falls back to scanning /tmp/.X11-unix for the lowest-numbered socket.
std::string detectX11Display() {
  const char *d = getenv("DISPLAY");
  if (d && d[0]) {
    std::string sock("/tmp/.X11-unix/X");
    std::string display(d);

    size_t colon = display.find(':');
    if (colon != std::string::npos) {
      std::string num = display.substr(colon + 1);
      size_t dot = num.find('.');
      if (dot != std::string::npos) num = num.substr(0, dot);
      if (!num.empty() && access((sock + num).c_str(), X_OK) == 0) {
        return display;
      }
    }
  }

  DIR *dir = opendir("/tmp/.X11-unix");
  if (!dir) return "";

  int best = -1;
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (ent->d_name[0] != 'X') continue;
    int num = atoi(ent->d_name + 1);
    if (num >= 0 && (best == -1 || num < best)) best = num;
  }
  closedir(dir);

  if (best >= 0) return ":" + std::to_string(best);
  return "";
}

} // namespace utils
