#include "package_repository.h"

// Initializes an empty package repository.
PackageRepository::PackageRepository() {}

// Destructor: frees all owned PackageData objects.
PackageRepository::~PackageRepository() {
  clear();
}

// Deletes all stored packages and clears the name index.
void PackageRepository::clear() {
  for (auto *p : m_listOfPackages) delete p;
  m_listOfPackages.clear();
  m_nameIndex.clear();
}

// Populates the repository with the given package list and marks which are user-required.
// Ownership of PackageData objects is transferred to this class.
void PackageRepository::setData(const std::vector<PackageListData> &packages,
                                 const std::set<std::string> &unrequiredPackages) {
  clear();

  for (auto &pld : packages) {
    bool isRequired = unrequiredPackages.find(pld.name) == unrequiredPackages.end();
    auto *pkg = new PackageData(pld, isRequired);
    m_listOfPackages.push_back(pkg);
    m_nameIndex[pkg->name] = pkg;
  }

  std::sort(m_listOfPackages.begin(), m_listOfPackages.end(),
    [](const PackageData *a, const PackageData *b) {
      return a->name < b->name;
    });
}

// Marks installed packages as outdated if they appear in the given outdated map.
void PackageRepository::markOutdatedPackages(
    const std::map<std::string, OutdatedPackageInfo> &outdatedMap) {
  for (auto *pkg : m_listOfPackages) {
    auto it = outdatedMap.find(pkg->name);
    if (it != outdatedMap.end()) {
      pkg->outdatedVersion = it->second.newVersion;
      if (pkg->installed()) {
        pkg->status = PackageStatus::Outdated;
      }
    }
  }
}

// Returns the PackageData for the given name from the index, or nullptr if not found.
PackageRepository::PackageData*
PackageRepository::getFirstPackageByName(const std::string &name) const {
  auto it = m_nameIndex.find(name);
  if (it != m_nameIndex.end()) return it->second;
  return nullptr;
}
