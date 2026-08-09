#ifndef IRUKA_SETTINGS_H
#define IRUKA_SETTINGS_H

#include "constants.h"
#include <string>

class Settings {
public:
  static Settings& instance();

  void load();
  void save();

  int windowWidth()  const { return m_windowWidth; }
  int windowHeight() const { return m_windowHeight; }
  void setWindowSize(int w, int h);

  ViewOption viewOption() const { return m_viewOption; }
  void setViewOption(ViewOption v);

  std::string selectedRepository() const { return m_selectedRepository; }
  void setSelectedRepository(const std::string &repo);

  // UI language code (e.g. "es", "ru"); empty means follow the system locale.
  std::string uiLanguage() const { return m_uiLanguage; }
  void setUiLanguage(const std::string &lang);

  int nameColumnWidth()    const { return m_nameColumnWidth; }
  int versionColumnWidth() const { return m_versionColumnWidth; }
  int sizeColumnWidth()    const { return m_sizeColumnWidth; }
  void setColumnWidths(int name, int version, int size);

private:
  Settings() = default;
  std::string getConfigPath();

  int m_windowWidth  = IRUKA_WINDOW_WIDTH;
  int m_windowHeight = IRUKA_WINDOW_HEIGHT;
  ViewOption m_viewOption = ViewOption::All;
  std::string m_selectedRepository;
  std::string m_uiLanguage;
  int m_nameColumnWidth    = 250;
  int m_versionColumnWidth = 120;
  int m_sizeColumnWidth    = 80;
};

#endif
