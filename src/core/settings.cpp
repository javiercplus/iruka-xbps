#include "settings.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

// Returns the process-wide singleton Settings instance.
Settings& Settings::instance() {
  static Settings s;
  return s;
}

// Returns the absolute path to the user config file, falling back to /tmp if HOME is not set.
std::string Settings::getConfigPath() {
  const char *home = getenv("HOME");
  if (!home) home = "/tmp";
  return std::string(home) + "/" + IRUKA_CONFIG_DIR_NAME + "/" + IRUKA_CONFIG_FILE_NAME;
}

void Settings::load() {
    // Determine the config file path and attempt to open it.
    std::string path = getConfigPath();
    std::ifstream f(path);
    if (!f.is_open()) return;

    // Helper: parse an integer safely, returning a fallback value on parse failure.
    auto toInt = [](const std::string &s, int fallback) -> int {
        try { return std::stoi(s); }
        catch (...) { return fallback; }
    };

    std::string line;
    while (std::getline(f, line)) {
        // Skip blank lines and comment lines starting with '#'.
        if (line.empty() || line[0] == '#') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        if      (key == "window_width")       m_windowWidth        = toInt(val, IRUKA_WINDOW_WIDTH);
        else if (key == "window_height")      m_windowHeight       = toInt(val, IRUKA_WINDOW_HEIGHT);
        else if (key == "view_option")        m_viewOption         = static_cast<ViewOption>(toInt(val, 0));
        else if (key == "repository")         m_selectedRepository = val;
        else if (key == "ui_language")        m_uiLanguage         = val;
        else if (key == "name_col_width")     m_nameColumnWidth    = toInt(val, 250);
        else if (key == "version_col_width")  m_versionColumnWidth = toInt(val, 120);
        else if (key == "size_col_width")     m_sizeColumnWidth    = toInt(val, 80);
    }
}

// Serializes all settings to disk, creating the config directory if needed.
void Settings::save() {
  const char *home = getenv("HOME");
  if (!home) return;

  std::string dir = std::string(home) + "/" + IRUKA_CONFIG_DIR_NAME;
  mkdir(dir.c_str(), 0755);

  std::string path = getConfigPath();
  std::ofstream f(path);
  if (!f.is_open()) return;

  f << "# iruka-xbps configuration\n";
  f << "window_width=" << m_windowWidth << "\n";
  f << "window_height=" << m_windowHeight << "\n";
  f << "view_option=" << static_cast<int>(m_viewOption) << "\n";
  f << "repository=" << m_selectedRepository << "\n";
  f << "ui_language=" << m_uiLanguage << "\n";
  f << "name_col_width=" << m_nameColumnWidth << "\n";
  f << "version_col_width=" << m_versionColumnWidth << "\n";
  f << "size_col_width=" << m_sizeColumnWidth << "\n";
}

void Settings::setWindowSize(int w, int h) {
  m_windowWidth = w;
  m_windowHeight = h;
}

void Settings::setViewOption(ViewOption v) {
  m_viewOption = v;
}

void Settings::setSelectedRepository(const std::string &repo) {
  m_selectedRepository = repo;
}

void Settings::setUiLanguage(const std::string &lang) {
  m_uiLanguage = lang;
}

void Settings::setColumnWidths(int name, int version, int size) {
  m_nameColumnWidth = name;
  m_versionColumnWidth = version;
  m_sizeColumnWidth = size;
}
