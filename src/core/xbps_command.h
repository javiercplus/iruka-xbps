#ifndef IRUKA_XBPS_COMMAND_H
#define IRUKA_XBPS_COMMAND_H

#include "package.h"
#include <string>
#include <vector>
#include <functional>

class XBPSCommand {
public:
  using OutputCallback = std::function<void(const std::string &)>;

  static std::string query(const std::vector<std::string> &args);
  static std::string runWithPrivileges(const std::vector<std::string> &args);

  static std::vector<PackageListData> getLocalPackageList();
  static std::vector<PackageListData> getRemotePackageList();
  static std::vector<PackageListData> searchRemotePackages(const std::string &query);
  static PackageInfoData getPackageInfo(const std::string &pkgName, bool installed);
  static std::vector<std::string> getPackageFiles(const std::string &pkgName, bool installed);
  static std::vector<std::string> getDependencies(const std::string &pkgName, bool remote);
  static std::vector<std::string> getUnrequiredPackages();
  static std::map<std::string, OutdatedPackageInfo> getOutdatedPackages();
  static TransactionInfo getTargetUpgradeList(const std::string &pkgName = "");
  static std::vector<std::string> getTargetRemovalList(const std::string &pkgName);
  static bool isPackageInstalled(const std::string &pkgName);
  static std::string getXBPSVersion();
  static std::string getFieldFromPackage(const std::string &field, const std::string &pkgName, bool remote);
  static std::string getPackageByFilePath(const std::string &filePath);

  static bool syncDatabase(OutputCallback cb = nullptr);
  static bool installPackages(const std::vector<std::string> &pkgs, OutputCallback cb = nullptr);
  static bool removePackages(const std::vector<std::string> &pkgs, OutputCallback cb = nullptr);
  static bool systemUpgrade(OutputCallback cb = nullptr);
  static bool cleanCache(OutputCallback cb = nullptr);

private:
  // Runs a child process and captures its stdout.
  // When forceLocale is true, 'env LANG=C LC_MESSAGES=C LC_ALL=C' is prepended so that
  // xbps tool output is always in English and can be reliably parsed.
  static std::string runProcess(const std::string &bin,
                                const std::vector<std::string> &args,
                                bool forceLocale = false);
};

#endif
