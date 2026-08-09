#include "xbps_command.h"
#include "constants.h"
#include <cstdio>
#include <memory>
#include <array>
#include <cstring>
#include <sstream>

// Spawns a child process, captures its stdout, and returns it as a string.
// When forceLocale is true, LC_ALL/LANG/LC_MESSAGES are forced to 'C' so that
// xbps output is always in English and parseable regardless of the system locale.
std::string XBPSCommand::runProcess(const std::string &bin,
                                     const std::vector<std::string> &args,
                                     bool forceLocale) {
    std::string cmd;
    if (forceLocale) {
        cmd = "env LANG=C LC_MESSAGES=C LC_ALL=C ";
    }
    cmd += bin;
    for (const auto &a : args) {
        cmd += ' ';
        cmd += a;
    }

    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;

    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    pclose(pipe);

    // Strip trailing newlines from the output.
    while (!result.empty() && result.back() == '\n')
        result.pop_back();

    return result;
}

// Runs xbps-query with the given arguments, forcing the C locale for consistent output parsing.
std::string XBPSCommand::query(const std::vector<std::string> &args) {
    return runProcess(XBPS_QUERY_BIN, args, true);
}

// Runs an arbitrary command via pkexec, which triggers a polkit privilege elevation dialog.
std::string XBPSCommand::runWithPrivileges(const std::vector<std::string> &args) {
    return runProcess(PKEXEC_BIN, args);
}

// Returns all locally installed packages parsed from 'xbps-query -l'.
std::vector<PackageListData> XBPSCommand::getLocalPackageList() {
  std::string out = query({"-l"});
  return Package::parseLocalPackageList(out);
}

// Returns all available packages from all configured remote repositories.
std::vector<PackageListData> XBPSCommand::getRemotePackageList() {
  std::string out = runProcess(XBPS_QUERY_BIN, {"-Rs", "-"}, true);
  return Package::parseRemotePackageList(out);
}

// Searches remote repositories for packages matching the given query string.
std::vector<PackageListData> XBPSCommand::searchRemotePackages(const std::string &searchString) {
  std::string out = query({"-Rs", searchString});
  return Package::parseRemotePackageList(out);
}

// Returns detailed metadata for an installed or remote package.
PackageInfoData XBPSCommand::getPackageInfo(const std::string &pkgName, bool installed) {
  std::string out;
  if (installed) {
    out = query({pkgName});
  } else {
    out = query({"-R", pkgName});
  }
  return Package::parsePackageInfo(out);
}

// Lists all files that belong to the given package.
std::vector<std::string> XBPSCommand::getPackageFiles(const std::string &pkgName, bool installed) {
  std::string extra = installed ? "" : "-R";
  std::vector<std::string> args;
  if (!extra.empty()) args.push_back(extra);
  args.push_back("-f");
  args.push_back(pkgName);
  std::string out = query(args);
  return Package::parseFileList(out);
}

// Returns the direct dependencies of a package; falls back to remote query if local returns empty.
std::vector<std::string> XBPSCommand::getDependencies(const std::string &pkgName, bool remote) {
  std::string out;
  if (remote) {
    out = query({"-Rx", pkgName});
  } else {
    out = query({"-x", pkgName});
  }
  if (out.empty() && !remote) {
    out = query({"-Rx", pkgName});
  }
  return Package::parseDependencies(out);
}

// Returns packages that are installed but not required by any other installed package.
std::vector<std::string> XBPSCommand::getUnrequiredPackages() {
  std::string out = query({"-m"});
  std::vector<std::string> result;
  std::istringstream iss(out);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.empty()) continue;
    auto dash = line.rfind('-');
    if (dash == std::string::npos) continue;
    result.push_back(line.substr(0, dash));
  }
  return result;
}

// Returns a map of package names to their outdated version info.
std::map<std::string, OutdatedPackageInfo> XBPSCommand::getOutdatedPackages() {
  std::string out = runProcess(XBPS_INSTALL_BIN, {"-un"}, true);
  return Package::parseOutdatedList(out);
}

// Returns the list of packages that would be upgraded. Pass empty string to upgrade all.
TransactionInfo XBPSCommand::getTargetUpgradeList(const std::string &pkgName) {
  std::string out;
  if (pkgName.empty()) {
    out = runProcess(XBPS_INSTALL_BIN, {"-un"}, true);
  } else {
    out = runProcess(XBPS_INSTALL_BIN, {"-n", "-Rs", pkgName}, true);
  }
  return Package::parseTargetUpgradeList(out);
}

// Returns the list of packages that would also be removed along with the given package.
std::vector<std::string> XBPSCommand::getTargetRemovalList(const std::string &pkgName) {
  std::string out = runProcess(XBPS_REMOVE_BIN, {"-R", "-n", pkgName}, true);
  return Package::parseTargetRemovalList(out);
}

// Returns true if the given package name is currently installed on the system.
bool XBPSCommand::isPackageInstalled(const std::string &pkgName) {
  std::string out = runProcess(XBPS_QUERY_BIN, {"-S", pkgName}, true);
  return !out.empty();
}

// Returns the version string reported by xbps-query.
std::string XBPSCommand::getXBPSVersion() {
  return query({"-V"});
}

// Queries a specific metadata field for the given package.
std::string XBPSCommand::getFieldFromPackage(const std::string &field,
                                              const std::string &pkgName,
                                              bool remote) {
  std::vector<std::string> args;
  if (remote) args.push_back("-R");
  args.push_back("-p");
  args.push_back(field);
  args.push_back(pkgName);
  return query(args);
}

// Finds and returns the name of the package that owns the given file path.
std::string XBPSCommand::getPackageByFilePath(const std::string &filePath) {
  std::string out = query({"-o", filePath});
  if (out.empty()) return "";

  auto pos = out.find(':');
  if (pos == std::string::npos) return "";

  std::string pkgNameVer = out.substr(0, pos);
  auto dash = pkgNameVer.rfind('-');
  if (dash == std::string::npos) return pkgNameVer;
  return pkgNameVer.substr(0, dash);
}

// Synchronizes all xbps repository databases. Requires root privileges via pkexec.
bool XBPSCommand::syncDatabase(OutputCallback cb) {
  if (cb) cb("Syncing package databases...");
  std::string out = runWithPrivileges({XBPS_INSTALL_BIN, "-S"});
  return true;
}

// Installs the given list of packages. Requires root privileges via pkexec.
bool XBPSCommand::installPackages(const std::vector<std::string> &pkgs, OutputCallback cb) {
  if (pkgs.empty()) return false;
  if (cb) cb("Installing packages...");

  std::vector<std::string> args;
  args.push_back(XBPS_INSTALL_BIN);
  args.push_back("-y");
  for (auto &p : pkgs) args.push_back(p);

  std::string out = runWithPrivileges(args);
  return true;
}

// Removes the given packages and their orphaned dependencies. Requires root via pkexec.
bool XBPSCommand::removePackages(const std::vector<std::string> &pkgs, OutputCallback cb) {
  if (pkgs.empty()) return false;
  if (cb) cb("Removing packages...");

  std::vector<std::string> args;
  args.push_back(XBPS_REMOVE_BIN);
  args.push_back("-R");
  args.push_back("-y");
  for (auto &p : pkgs) args.push_back(p);

  std::string out = runWithPrivileges(args);
  return true;
}

// Upgrades all installed packages to their latest versions. Requires root via pkexec.
bool XBPSCommand::systemUpgrade(OutputCallback cb) {
  if (cb) cb("Upgrading system...");
  std::string out = runWithPrivileges({XBPS_INSTALL_BIN, "-Syu"});
  return true;
}

// Removes cached package archive files from disk. Requires root via pkexec.
bool XBPSCommand::cleanCache(OutputCallback cb) {
  if (cb) cb("Cleaning package cache...");
  std::string out = runWithPrivileges({XBPS_REMOVE_BIN, "-O"});
  return true;
}
