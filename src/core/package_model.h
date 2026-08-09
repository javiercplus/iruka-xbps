#ifndef IRUKA_PACKAGE_MODEL_H
#define IRUKA_PACKAGE_MODEL_H

#include "package_repository.h"
#include "constants.h"
#include <string>
#include <vector>
#include <regex>

class PackageModel {
public:
  enum Column {
    ColStatus = 0,
    ColName,
    ColVersion,
    ColSize,
    ColDescription,
    ColumnCount
  };

  PackageModel(const PackageRepository &repo);

  void applyFilter(ViewOption view, const std::string &repo = "");
  void applyFilter(const std::string &filterText, int filterColumn = -1);

  int rowCount() const { return static_cast<int>(m_filtered.size()); }
  const PackageRepository::PackageData* getData(int row) const;
  int rowOfPackage(const PackageRepository::PackageData *pkg) const;

  void sort(int column, bool ascending);
  void clear();

  int getInstalledCount() const { return m_installedCount; }

  static std::string getColumnName(int col);
  static std::string getColumnHeader(int col);

private:
  const PackageRepository &m_repo;
  std::vector<PackageRepository::PackageData*> m_filtered;
  int m_installedCount = 0;
};

#endif
