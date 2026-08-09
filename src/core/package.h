#ifndef IRUKA_PACKAGE_H
#define IRUKA_PACKAGE_H

#include "constants.h"
#include <string>
#include <vector>
#include <map>

struct PackageListData {
  std::string name;
  std::string repository;
  std::string origin;
  std::string version;
  std::string comment;
  std::string description;
  std::string installedOn;
  std::string outdatedVersion;
  double installedSize = 0.0;
  double downloadSize = 0.0;
  PackageStatus status = PackageStatus::NotInstalled;
};

struct PackageInfoData {
  std::string name;
  std::string repository;
  std::string version;
  std::string url;
  std::string license;
  std::string group;
  std::string description;
  std::string comment;
  std::string maintainer;
  std::string arch;
  std::string buildDate;
  std::string installDate;
  std::string downloadSizeAsString;
  std::string installedSizeAsString;
  std::string options;
  std::string dependsOn;
  std::string optDepends;
  std::string requiredBy;
  std::string optionalFor;
  std::string conflictsWith;
  std::string replaces;
};

struct OutdatedPackageInfo {
  std::string oldVersion;
  std::string newVersion;
};

struct TransactionInfo {
  std::vector<std::string> packages;
  std::string sizeToDownload;
};

class Package {
public:
  static std::vector<PackageListData> parseLocalPackageList(const std::string &output);
  static std::vector<PackageListData> parseRemotePackageList(const std::string &output);
  static PackageInfoData parsePackageInfo(const std::string &output);
  static std::vector<std::string> parseFileList(const std::string &output);
  static std::vector<std::string> parseDependencies(const std::string &output);
  static std::map<std::string, OutdatedPackageInfo> parseOutdatedList(const std::string &output);
  static TransactionInfo parseTargetUpgradeList(const std::string &output);
  static std::vector<std::string> parseTargetRemovalList(const std::string &output);

  static std::string extractField(const std::string &field, const std::string &pkgInfo);
  static std::string getBaseName(const std::string &pkgNameVersion);
  static std::string getVersionPart(const std::string &pkgNameVersion);
  static std::string kbytesToSize(double kbytes);
  static bool isForbidden(const std::string &pkgName);
};

#endif
