#ifndef IRUKA_UTILS_H
#define IRUKA_UTILS_H

#include <string>
#include <vector>

namespace utils {

std::string getConfigDir();
bool hasInternetConnection();
bool hasBinary(const std::string &path);
std::string getShell();
std::string detectX11Display();

// Returns the directory containing the gettext message catalogs, searching:
//   1. $IRUKA_LOCALEDIR — explicit override
//   2. <exedir>/po      — the meson build tree (running from builddir)
//   3. LOCALEDIR        — the configured install location
std::string resolveLocaledir();

} // namespace utils

#endif
