#include "package_model.h"
#include <algorithm>
#include <cctype>

// Constructs a PackageModel bound to the given PackageRepository.
PackageModel::PackageModel(const PackageRepository &repo) : m_repo(repo) {}

// Filters the package list by install status and optional repository name.
// This must be called before the text-filter overload.
void PackageModel::applyFilter(ViewOption view, const std::string &repo) {
  m_filtered.clear();
  m_installedCount = 0;

  for (auto *pkg : m_repo.getPackageList()) {
    if (pkg->installed()) m_installedCount++;

    bool match = true;
    switch (view) {
      case ViewOption::Installed:
        match = pkg->installed();
        break;
      case ViewOption::NotInstalled:
        match = !pkg->installed();
        break;
      case ViewOption::All:
      default:
        break;
    }

    if (match && !repo.empty()) {
      match = (pkg->repository == repo);
    }

    if (match) m_filtered.push_back(pkg);
  }
}

// Further filters the current m_filtered list by a case-insensitive text search.
// Searches package name, version, and description. Must be called after the view filter.
void PackageModel::applyFilter(const std::string &filterText, int filterColumn) {
  if (filterText.empty()) {
    applyFilter(ViewOption::All);
    return;
  }

  std::string lowerFilter = filterText;
  std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(),
    [](unsigned char c) { return std::tolower(c); });

  std::vector<PackageRepository::PackageData*> filtered;

  for (auto *pkg : m_filtered) {
    bool match = false;
    auto toLower = [](const std::string &s) {
      std::string res = s;
      std::transform(res.begin(), res.end(), res.begin(),
        [](unsigned char c) { return std::tolower(c); });
      return res;
    };

    if (filterColumn == -1 || filterColumn == ColName) {
      if (toLower(pkg->name).find(lowerFilter) != std::string::npos) { match = true; }
    }
    if (!match && (filterColumn == -1 || filterColumn == ColDescription)) {
      if (toLower(pkg->description).find(lowerFilter) != std::string::npos) { match = true; }
    }
    if (!match && (filterColumn == -1 || filterColumn == ColVersion)) {
      if (toLower(pkg->version).find(lowerFilter) != std::string::npos) { match = true; }
    }

    if (match) filtered.push_back(pkg);
  }

  m_filtered = std::move(filtered);
}

// Returns the PackageData for the given row index, or nullptr if out of bounds.
const PackageRepository::PackageData* PackageModel::getData(int row) const {
  if (row < 0 || row >= static_cast<int>(m_filtered.size())) return nullptr;
  return m_filtered[row];
}

// Returns the row index of the given package pointer in the filtered list, or -1 if not found.
int PackageModel::rowOfPackage(const PackageRepository::PackageData *pkg) const {
  for (size_t i = 0; i < m_filtered.size(); ++i) {
    if (m_filtered[i] == pkg) return static_cast<int>(i);
  }
  return -1;
}

// Sorts the filtered list by the given column in ascending or descending order.
void PackageModel::sort(int column, bool ascending) {
  auto cmp = [column, ascending](const PackageRepository::PackageData *a,
                                  const PackageRepository::PackageData *b) -> bool {
    int r = 0;
    switch (column) {
      case ColStatus: {
        int sa = static_cast<int>(a->status);
        int sb = static_cast<int>(b->status);
        r = sa - sb;
        break;
      }
      case ColName:
        r = a->name.compare(b->name);
        break;
      case ColVersion:
        r = a->version.compare(b->version);
        break;
      case ColSize:
        r = (a->installedSize < b->installedSize) ? -1 :
            (a->installedSize > b->installedSize) ? 1 : 0;
        break;
      case ColDescription:
        r = a->description.compare(b->description);
        break;
      default:
        r = a->name.compare(b->name);
    }
    return ascending ? (r < 0) : (r > 0);
  };

  std::sort(m_filtered.begin(), m_filtered.end(), cmp);
}

// Clears the filtered list and resets the installed count to zero.
void PackageModel::clear() {
  m_filtered.clear();
  m_installedCount = 0;
}

// Returns the short column identifier string (e.g., "S", "Name").
std::string PackageModel::getColumnName(int col) {
  switch (col) {
    case ColStatus:     return "S";
    case ColName:       return "Name";
    case ColVersion:    return "Version";
    case ColSize:       return "Size";
    case ColDescription:return "Description";
    default:            return "";
  }
}

// Returns the display header string for the given column.
std::string PackageModel::getColumnHeader(int col) {
  return getColumnName(col);
}
