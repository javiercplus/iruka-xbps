#include "package.h"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// Splits a multi-line string into individual non-empty lines.
static std::vector<std::string> splitLines(const std::string &s) {
  std::vector<std::string> lines;
  std::istringstream iss(s);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

// Tokenizes a string on whitespace, returning each token as a vector element.
static std::vector<std::string> splitSpaces(const std::string &s) {
  std::vector<std::string> parts;
  std::istringstream iss(s);
  std::string part;
  while (iss >> part) parts.push_back(part);
  return parts;
}

// Extracts the base package name by stripping the trailing -<version> suffix.
// Example: "firefox-120.0_1" returns "firefox".
std::string Package::getBaseName(const std::string &pkgNameVersion) {
  std::string s = pkgNameVersion;
  auto lastDash = s.rfind('-');
  if (lastDash == std::string::npos) return s;
  return s.substr(0, lastDash);
}

// Extracts the version part after the last dash in a name-version string.
// Example: "firefox-120.0_1" returns "120.0_1".
std::string Package::getVersionPart(const std::string &pkgNameVersion) {
  auto lastDash = pkgNameVersion.rfind('-');
  if (lastDash == std::string::npos) return "";
  return pkgNameVersion.substr(lastDash + 1);
}

// Converts a size in kibibytes to a human-readable string (B, KiB, MiB, GiB).
std::string Package::kbytesToSize(double kbytes) {
  char buf[64];
  if (kbytes >= 1073741824.0)
    snprintf(buf, sizeof(buf), "%.2f GiB", kbytes / 1048576.0);
  else if (kbytes >= 1048576.0)
    snprintf(buf, sizeof(buf), "%.2f MiB", kbytes / 1024.0);
  else if (kbytes >= 1024.0)
    snprintf(buf, sizeof(buf), "%.2f KiB", kbytes);
  else
    snprintf(buf, sizeof(buf), "%.0f B", kbytes * 1024.0);
  return buf;
}

// Returns true if the package name is in the protected list and must not be removed.
bool Package::isForbidden(const std::string &pkgName) {
  static const char* forbidden[] = {
    "base-files", "base-system", "libxbps", "xbps", "xbps-triggers", nullptr
  };
  for (const char** f = forbidden; *f; ++f) {
    if (pkgName == *f) return true;
  }
  return false;
}

// Searches pkgInfo text for a line beginning with "<field>:" and returns the trimmed value.
// Returns an empty string if the field is not found.
std::string Package::extractField(const std::string &field, const std::string &pkgInfo) {
  auto pos = pkgInfo.find(field + ":");
  if (pos == std::string::npos) return "";

  pos += field.size() + 1;
  while (pos < pkgInfo.size() && pkgInfo[pos] == ' ') pos++;

  auto end = pkgInfo.find('\n', pos);
  if (end == std::string::npos) end = pkgInfo.size();

  return pkgInfo.substr(pos, end - pos);
}

// Parses the output of 'xbps-query -l' into a vector of PackageListData entries.
// Line format: <status-flags> <name-version> [description...]
// Status indicators containing '*', or equal to 'i', 'ii', or '[*]' mean installed.
std::vector<PackageListData> Package::parseLocalPackageList(const std::string &output) {
  std::vector<PackageListData> result;
  auto lines = splitLines(output);

  for (auto &line : lines) {
    PackageListData pkg;
    auto parts = splitSpaces(line);
    if (parts.size() < 2) continue;

    std::string statusStr = parts[0];
    std::string nameVer = parts[1];

    pkg.name = getBaseName(nameVer);
    pkg.version = getVersionPart(nameVer);

    if (statusStr.find('*') != std::string::npos ||
        statusStr == "i" || statusStr == "ii" || statusStr == "[*]") {
      pkg.status = PackageStatus::Installed;
    } else {
      pkg.status = PackageStatus::NotInstalled;
    }

    std::string comment;
    for (size_t c = 2; c < parts.size(); c++) {
      if (!comment.empty()) comment += " ";
      comment += parts[c];
    }
    pkg.comment = comment;
    pkg.description = pkg.name + " " + comment;

    result.push_back(std::move(pkg));
  }
  return result;
}

// Parses the output of 'xbps-query -Rs' into a vector of PackageListData entries.
// The '*' status indicator means the package is already installed locally.
std::vector<PackageListData> Package::parseRemotePackageList(const std::string &output) {
  std::vector<PackageListData> result;
  auto lines = splitLines(output);

  for (auto &line : lines) {
    PackageListData pkg;
    auto parts = splitSpaces(line);
    if (parts.size() < 2) continue;

    std::string statusStr = parts[0];
    std::string nameVer = parts[1];

    pkg.name = getBaseName(nameVer);
    pkg.version = getVersionPart(nameVer);

    if (statusStr.find('*') != std::string::npos) {
      pkg.status = PackageStatus::Installed;
    } else {
      pkg.status = PackageStatus::NotInstalled;
    }

    std::string comment;
    for (size_t c = 2; c < parts.size(); c++) {
      if (!comment.empty()) comment += " ";
      comment += parts[c];
    }
    pkg.comment = comment;
    pkg.description = pkg.name + " " + comment;

    result.push_back(std::move(pkg));
  }
  return result;
}

PackageInfoData Package::parsePackageInfo(const std::string &output) {
  PackageInfoData info;
  info.name             = extractField("Name", output);
  info.version          = extractField("Version", output);
  info.repository       = extractField("Repository", output);
  info.url              = extractField("homepage", output);
  info.license          = extractField("license", output);
  info.group            = extractField("Categories", output);
  info.description      = extractField("Description", output);
  info.comment          = extractField("Comment", output);
  info.maintainer       = extractField("maintainer", output);
  info.arch             = extractField("architecture", output);
  info.buildDate        = extractField("build-date", output);
  info.installDate      = extractField("install-date", output);
  info.downloadSizeAsString = extractField("filename-size", output);
  info.installedSizeAsString = extractField("installed_size", output);
  info.options          = extractField("options", output);
  info.dependsOn        = extractField("Depends On", output);
  info.optDepends       = extractField("Optional Deps", output);
  info.requiredBy       = extractField("Required By", output);
  info.optionalFor      = extractField("Optional For", output);
  info.conflictsWith    = extractField("Conflicts With", output);
  info.replaces         = extractField("Replaces", output);
  return info;
}

std::vector<std::string> Package::parseFileList(const std::string &output) {
  return splitLines(output);
}

std::vector<std::string> Package::parseDependencies(const std::string &output) {
  return splitLines(output);
}

// Parses the output of 'xbps-install -un' to find packages with available upgrades.
// Only lines containing the keyword "update" are processed.
std::map<std::string, OutdatedPackageInfo> Package::parseOutdatedList(const std::string &output) {
  std::map<std::string, OutdatedPackageInfo> result;
  auto lines = splitLines(output);

  for (auto &line : lines) {
    if (line.find("update") == std::string::npos) continue;

    auto parts = splitSpaces(line);
    if (parts.empty()) continue;

    std::string nameVer = parts[0];
    auto name = getBaseName(nameVer);
    auto ver  = getVersionPart(nameVer);

    OutdatedPackageInfo opi;
    opi.newVersion = ver;
    result[name] = opi;
  }
  return result;
}

TransactionInfo Package::parseTargetUpgradeList(const std::string &output) {
  TransactionInfo ti;
  auto lines = splitLines(output);

  for (auto &line : lines) {
    auto parts = splitSpaces(line);
    if (parts.empty()) continue;

    std::string nameVer = parts[0];
    auto name = getBaseName(nameVer);
    ti.packages.push_back(name);
  }
  return ti;
}

std::vector<std::string> Package::parseTargetRemovalList(const std::string &output) {
  std::vector<std::string> result;
  auto lines = splitLines(output);

  for (auto &line : lines) {
    auto pos = line.find("remove");
    if (pos == std::string::npos) continue;

    auto space = line.find(' ');
    if (space == std::string::npos || space >= pos) continue;

    std::string nameVer = line.substr(0, space);
    auto name = getBaseName(nameVer);
    result.push_back(name);
  }
  return result;
}
