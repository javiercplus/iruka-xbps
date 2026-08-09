#ifndef IRUKA_PACKAGE_REPOSITORY_H
#define IRUKA_PACKAGE_REPOSITORY_H

#include "package.h"
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional>

class PackageRepository {
public:
  struct PackageData {
    bool required;
    std::string name;
    std::string repository;
    std::string origin;
    std::string version;
    std::string description;
    std::string comment;
    std::string outdatedVersion;
    std::string installedOn;
    double downloadSize = 0.0;
    double installedSize = 0.0;
    PackageStatus status = PackageStatus::NotInstalled;

    bool installed() const { return status != PackageStatus::NotInstalled; }
    bool outdated()  const { return status == PackageStatus::Outdated; }

    PackageData(const PackageListData &pld, bool isRequired)
      : required(isRequired) {
      name        = pld.name;
      repository  = pld.repository;
      origin      = pld.origin;
      version     = pld.version;
      description = pld.description;
      comment     = pld.comment;
      installedOn = pld.installedOn;
      outdatedVersion = pld.outdatedVersion;
      downloadSize    = pld.downloadSize;
      installedSize   = pld.installedSize;
      status      = pld.status;
    }
  };

  using TListOfPackages = std::vector<PackageData*>;

  PackageRepository();
  ~PackageRepository();

  void setData(const std::vector<PackageListData> &packages,
               const std::set<std::string> &unrequiredPackages);
  void markOutdatedPackages(const std::map<std::string, OutdatedPackageInfo> &outdatedMap);

  const TListOfPackages& getPackageList() const { return m_listOfPackages; }

  PackageData* getFirstPackageByName(const std::string &name) const;

  void clear();

private:
  TListOfPackages m_listOfPackages;
  std::map<std::string, PackageData*> m_nameIndex;
};

#endif
