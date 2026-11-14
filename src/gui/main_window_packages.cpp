#include "main_window_private.h"
#include "xbps_command.h"
#include "settings.h"
#include "package.h"
#include "package_model.h"
#include "i18n.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Buffer.H>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdio>

namespace {

std::string sanitizeField(const std::string &text) {
    std::string out = text;
    for (char &c : out) {
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    }
    return out;
}

std::string packageLine(const std::string &status,
                        const PackageRepository::PackageData *pkg) {
    std::string line = status;
    line += '\t';
    line += sanitizeField(pkg->name);
    line += '\t';
    line += sanitizeField(pkg->version);
    line += '\t';
    line += sanitizeField(pkg->description);
    return line;
}

}

void MainWindow::loadPackageList() {
    if (m_loading) return;
    m_loading = true;

    auto *result = new LoadResult();
    result->win = this;

    std::thread([result]() {
        auto localPkgs  = XBPSCommand::getLocalPackageList();
        auto remotePkgs = XBPSCommand::getRemotePackageList();

        std::map<std::string, PackageListData *> pkgMap;
        for (auto &pkg : remotePkgs) {
            auto *np = new PackageListData(std::move(pkg));
            auto it = pkgMap.find(np->name);
            if (it != pkgMap.end()) {
                delete it->second;
                it->second = np;
            } else {
                pkgMap[np->name] = np;
            }
        }
        for (auto &pkg : localPkgs) {
            auto it = pkgMap.find(pkg.name);
            if (it != pkgMap.end()) {
                it->second->version     = pkg.version;
                it->second->status      = pkg.status;
                it->second->comment     = pkg.comment;
                it->second->description = pkg.description;
            } else {
                auto *np = new PackageListData(std::move(pkg));
                pkgMap[np->name] = np;
            }
        }

        for (auto &entry : pkgMap) {
            result->merged.push_back(std::move(*entry.second));
            delete entry.second;
        }

        auto unrequired = XBPSCommand::getUnrequiredPackages();
        result->unreqSet.insert(unrequired.begin(), unrequired.end());

        result->outdated = XBPSCommand::getOutdatedPackages();

        Fl::awake(load_done_awake, result);
    }).detach();
}

void MainWindow::applyLoadedPackages(std::vector<PackageListData> &&packages,
                                     std::set<std::string> &&unreqSet,
                                     std::map<std::string, OutdatedPackageInfo> &&outdated) {
    m_pkgBrowser->clear();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_repo.setData(packages, unreqSet);
        m_repo.markOutdatedPackages(outdated);
    }

    m_loading = false;
    applyFilters();
}

void MainWindow::populateTable() {
    m_pkgBrowser->clear();

    std::string header = std::string("S\t") + _("Name") + "\t" +
                         _("Version") + "\t" + _("Description");
    m_pkgBrowser->add(header.c_str());

    std::lock_guard<std::mutex> lock(m_mutex);
    int count = m_model->rowCount();
    for (int i = 0; i < count; i++) {
        const auto *pkg = m_model->getData(i);
        if (!pkg) continue;

        std::string status;
        if      (m_removeMarks.count(pkg->name) > 0) status = "X";
        else if (m_installMarks.count(pkg->name) > 0) status = "I";
        else if (pkg->outdated())  status = "!";
        else if (pkg->installed()) status = "@";
        else                       status = " ";

        std::string line = packageLine(status, pkg);
        m_pkgBrowser->add(line.c_str(),
                          const_cast<PackageRepository::PackageData *>(pkg));
    }
}

void MainWindow::refreshStatusBar() {
    std::lock_guard<std::mutex> lock(m_mutex);

    int total = static_cast<int>(m_repo.getPackageList().size());

    int outdated = 0;
    for (auto *p : m_repo.getPackageList()) {
        if (p->outdated()) outdated++;
    }

    char buf[64];
    std::ostringstream ss;
    std::snprintf(buf, sizeof(buf), _("Packages: %d"), total);
    ss << buf;
    std::snprintf(buf, sizeof(buf), _("Installed: %d"), m_model->getInstalledCount());
    ss << "    " << buf;
    std::snprintf(buf, sizeof(buf), _("Outdated: %d"), outdated);
    ss << "    " << buf;

    m_statusbar->copy_label(ss.str().c_str());
    m_statusbar->redraw();
}

void MainWindow::applyFilters() {
    const char *text = m_searchEntry ? m_searchEntry->value() : nullptr;
    std::string query = text ? text : "";
    ViewOption view = Settings::instance().viewOption();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_model->applyFilter(view, "", m_selectedCategory);
        if (!query.empty()) {
            m_model->applyFilter(query);
        }
        m_model->sort(PackageModel::ColName, true);
    }
    populateTable();
    refreshStatusBar();
}

void MainWindow::onSearch() {
    applyFilters();
}

void MainWindow::applyCategory(int kind, const std::string &value) {
    switch (kind) {
        case 0: setView(ViewOption::All); break;
        case 1: setView(ViewOption::Installed); break;
        case 2: setView(ViewOption::NotInstalled); break;
        case 3: setView(ViewOption::Outdated); break;
        case 5:
            m_selectedCategory = value;
            Settings::instance().setViewOption(ViewOption::All);
            applyFilters();
            break;
        default:
            break;
    }
}

void MainWindow::onTableSelect() {
    if (m_rebuilding || m_destroyed.load()) return;

    if (m_sizeBuffer) m_sizeBuffer->text("");

    Fl::remove_timeout(cb_size_debounce, this);
    Fl::add_timeout(0.25, cb_size_debounce, this);
}

void MainWindow::onSelectionDebounced() {
    int line = m_pkgBrowser->value();
    if (line < 2) return;

    auto *pkg = static_cast<PackageRepository::PackageData *>(
        m_pkgBrowser->data(line));
    if (!pkg) return;

    int row = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        row = m_model->rowOfPackage(pkg);
    }
    if (row >= 0) showPackageInfo(row);
}

void MainWindow::showPackageInfo(int row) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto *pkg = m_model->getData(row);
    if (!pkg) return;

    std::ostringstream ss;
    ss << _("Name:")        << " " << pkg->name        << "\n";
    ss << _("Version:")     << " " << pkg->version     << "\n";
    ss << _("Repository:")  << " " << pkg->repository  << "\n";
    ss << _("Description:") << " " << pkg->description << "\n";

    bool installed = pkg->installed();
    std::string pkgName = pkg->name;

    auto *result = new InfoResult();
    result->win = this;
    result->generation = ++m_infoGeneration;
    result->infoText = ss.str();

    m_infoBuffer->text(ss.str().c_str());
    m_filesBuffer->text("");

    std::thread([result, pkgName, installed]() {
        auto info = XBPSCommand::getPackageInfo(pkgName, installed);

        std::ostringstream ss;
        if (installed) {
            ss << "\n" << _("--- Package Details ---") << "\n";
            ss << _("Homepage:")     << "   " << info.url                  << "\n";
            ss << _("License:")      << "   " << info.license              << "\n";
            ss << _("Maintainer:")   << "   " << info.maintainer           << "\n";
            ss << _("Arch:")         << "   " << info.arch                 << "\n";
            ss << _("Build Date:")   << "   " << info.buildDate            << "\n";
            ss << _("Install Date:") << "   " << info.installDate          << "\n";
            ss << "\n" << _("Depends On:") << "   " << info.dependsOn      << "\n";
            ss << _("Opt Deps:")     << "   " << info.optDepends           << "\n";
            ss << _("Required By:")  << "   " << info.requiredBy           << "\n";
            ss << _("Conflicts:")    << "   " << info.conflictsWith        << "\n";
            ss << _("Replaces:")     << "   " << info.replaces             << "\n";
            ss << _("Options:")      << "   " << info.options              << "\n";
        } else {
            ss << "\n" << _("--- Remote Package Details ---") << "\n";
            ss << _("Homepage:")     << "   " << info.url                  << "\n";
            ss << _("License:")      << "   " << info.license              << "\n";
            ss << _("Maintainer:")   << "   " << info.maintainer           << "\n";
            ss << _("Arch:")         << "   " << info.arch                 << "\n";
            ss << "\n" << _("Depends On:") << "   " << info.dependsOn      << "\n";
            ss << _("Opt Deps:")     << "   " << info.optDepends           << "\n";
            ss << _("Conflicts:")    << "   " << info.conflictsWith        << "\n";
        }
        result->infoText += ss.str();

        std::ostringstream sizeSs;
        sizeSs << _("Name:")           << " " << pkgName                     << "\n";
        sizeSs << _("Download Size:")  << "   " << info.downloadSizeAsString << "\n";
        sizeSs << _("Installed Size:") << "   " << info.installedSizeAsString<< "\n";
        result->sizeText = sizeSs.str();

        auto files = XBPSCommand::getPackageFiles(pkgName, installed);
        std::ostringstream fss;
        for (auto &f : files) fss << f << "\n";
        result->filesText = fss.str();

        Fl::awake(info_done_awake, result);
    }).detach();
}

void MainWindow::applyPackageInfo(int generation, const std::string &infoText,
                                  const std::string &filesText,
                                  const std::string &sizeText) {
    if (generation != m_infoGeneration) return;
    m_infoBuffer->text(infoText.c_str());
    m_filesBuffer->text(filesText.c_str());
    m_sizeBuffer->text(sizeText.c_str());
}
