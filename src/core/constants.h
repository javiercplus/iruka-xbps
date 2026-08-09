#ifndef IRUKA_CONSTANTS_H
#define IRUKA_CONSTANTS_H

#include <string>

constexpr const char* IRUKA_APP_NAME    = "Iruka-xbps";
constexpr const char* IRUKA_APP_VERSION = "0.1.0";
constexpr const char* IRUKA_APP_TITLE   = "Iruka-xbps Package Manager";

// gettext text domain for the message catalogs installed under LOCALEDIR.
constexpr const char* IRUKA_TEXTDOMAIN  = "iruka-xbps";

constexpr const char* IRUKA_CONFIG_DIR_NAME  = ".config/iruka-xbps";
constexpr const char* IRUKA_CONFIG_FILE_NAME = "iruka-xbps.conf";

constexpr const char* XBPS_QUERY_BIN  = "/usr/bin/xbps-query";
constexpr const char* XBPS_INSTALL_BIN = "/usr/bin/xbps-install";
constexpr const char* XBPS_REMOVE_BIN  = "/usr/bin/xbps-remove";
constexpr const char* PKEXEC_BIN       = "/usr/bin/pkexec";

constexpr const char* XBPS_DB_DIR = "/var/db/xbps";

constexpr int IRUKA_WINDOW_WIDTH  = 1100;
constexpr int IRUKA_WINDOW_HEIGHT = 700;

enum class PackageStatus {
  Installed,
  NotInstalled,
  Outdated,
  Newer,
};

enum class ViewOption {
  All,
  Installed,
  NotInstalled,
};

enum class CommandExecuting {
  NoneCmd,
  SyncDatabase,
  SystemUpgrade,
  Install,
  Remove,
  CleanCache,
};

enum class SaveReason {
  WindowSize,
  ColumnWidths,
  ViewOption,
  Repository,
};

#endif
