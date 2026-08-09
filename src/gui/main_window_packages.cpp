// ----------------------------------------------------------------------------
// main_window_packages.cpp — Package list loading and display for MainWindow
//
// Contains:
//   - loadPackageList()       background thread that fetches all package data
//   - applyLoadedPackages()   applies fetched data on the main thread
//   - populateTable()         rebuilds the GtkListStore from the model
//   - refreshStatusBar()      updates the package count display
//   - onSearch()              applies filters based on the search entry
//   - onTableSelect()         handles row selection changes
//   - showPackageInfo()       shows basic info immediately + fetches details async
//   - applyPackageInfo()      updates the info panel on the main thread
// ----------------------------------------------------------------------------

#include "main_window_private.h"
#include "xbps_command.h"
#include "settings.h"
#include "package.h"
#include "package_model.h"
#include "i18n.h"
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include <cstdio>

// Starts a background thread that fetches local, remote, and outdated package lists,
// then marshals the result back to the main thread via g_idle_add.
void MainWindow::loadPackageList() {
    if (m_loading) return;
    m_loading = true;

    auto *result = new LoadResult();
    result->win = this;

    std::thread([result]() {
        // Fetch installed packages and all remote packages in parallel conceptually,
        // but sequentially here since both calls block on xbps-query.
        auto localPkgs  = XBPSCommand::getLocalPackageList();
        auto remotePkgs = XBPSCommand::getRemotePackageList();

        // Merge strategy: start from remote packages (canonical source for metadata),
        // then overlay local install status and version from the local query.
        std::map<std::string, PackageListData*> pkgMap;
        for (auto &pkg : remotePkgs) {
            auto *np = new PackageListData(std::move(pkg));
            auto it  = pkgMap.find(np->name);
            if (it != pkgMap.end()) {
                // Prefer the last seen remote entry (usually most up-to-date).
                delete it->second;
                it->second = np;
            } else {
                pkgMap[np->name] = np;
            }
        }
        // Overlay local data: update version and install status for known packages,
        // or add locally-only packages that are absent from the remote index.
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

        // Flatten the map into the result vector and free the temporary heap objects.
        for (auto &[name, pkg] : pkgMap) {
            result->merged.push_back(std::move(*pkg));
            delete pkg;
        }

        auto unrequired = XBPSCommand::getUnrequiredPackages();
        result->unreqSet.insert(unrequired.begin(), unrequired.end());

        result->outdated = XBPSCommand::getOutdatedPackages();

        g_idle_add(load_done_cb, result);
    }).detach();
}

// Called on the main thread with the results from the background loader thread.
// Updates the repository, model, and table view.
void MainWindow::applyLoadedPackages(std::vector<PackageListData> &&packages,
                                     std::set<std::string>         &&unreqSet,
                                     std::map<std::string, OutdatedPackageInfo> &&outdated) {
    // Clear the list store first to invalidate all stored PackageData pointers
    // before the repository deletes the underlying objects in setData().
    gtk_list_store_clear(m_listStore);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_repo.setData(packages, unreqSet);
        m_repo.markOutdatedPackages(outdated);
        m_model->applyFilter(Settings::instance().viewOption());
        m_model->sort(PackageModel::ColName, true);
    }

    populateTable();
    refreshStatusBar();
    m_loading = false;
}

// Repopulates the GtkListStore from the current PackageModel filtered list.
void MainWindow::populateTable() {
    gtk_list_store_clear(m_listStore);

    std::lock_guard<std::mutex> lock(m_mutex);
    int count = m_model->rowCount();
    for (int i = 0; i < count; i++) {
        auto *pkg = m_model->getData(i);
        if (!pkg) continue;

        GtkTreeIter iter;
        gtk_list_store_append(m_listStore, &iter);

        // Determine the status indicator character:
        // 'X' = queued for removal, 'I' = queued for install,
        // '!' = outdated, '@' = installed, ' ' = not installed.
        std::string status;
        if      (m_removeMarks.count(pkg->name) > 0) status = "X";
        else if (m_installMarks.count(pkg->name) > 0) status = "I";
        else if (pkg->outdated())  status = "!";
        else if (pkg->installed()) status = "@";
        else                       status = " ";

        gtk_list_store_set(m_listStore, &iter,
            COL_STATUS,      status.c_str(),
            COL_NAME,        pkg->name.c_str(),
            COL_VERSION,     pkg->version.c_str(),
            COL_DESCRIPTION, pkg->description.c_str(),
            COL_PKG_PTR,     (gpointer)pkg,
            -1);
    }
}

// Updates the status bar text with current package totals.
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

    gtk_statusbar_remove_all(GTK_STATUSBAR(m_statusbar), m_statusContextId);
    gtk_statusbar_push(GTK_STATUSBAR(m_statusbar), m_statusContextId, ss.str().c_str());
}

// Handles the search entry change: re-applies the view filter then the text filter.
void MainWindow::onSearch() {
    const char *text  = gtk_entry_get_text(GTK_ENTRY(m_searchEntry));
    std::string query = text ? text : "";
    ViewOption view   = Settings::instance().viewOption();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // View filter must be applied first; the text filter narrows the result.
        m_model->applyFilter(view);
        if (!query.empty()) {
            m_model->applyFilter(query);
        }
        m_model->sort(PackageModel::ColName, true);
    }
    populateTable();
    refreshStatusBar();
}

// Called when the selected row changes. Debounces the heavy info/size fetch so
// that rapidly selecting several rows only triggers one lookup, and clears the
// Size tab so it never shows stale data from a previous selection.
void MainWindow::onTableSelect() {
    gtk_text_buffer_set_text(m_sizeBuffer, "", -1);

    if (m_sizeDebounceId) {
        g_source_remove(m_sizeDebounceId);
        m_sizeDebounceId = 0;
    }
    m_sizeDebounceId = g_timeout_add(250, cb_size_debounce, this);
}

// Fires after the selection debounce elapses; fetches info/files/size for the
// currently selected package.
void MainWindow::onSelectionDebounced() {
    GtkTreeIter  iter;
    GtkTreeModel *model = GTK_TREE_MODEL(m_listStore);
    if (!gtk_tree_selection_get_selected(m_selection, &model, &iter)) return;

    gpointer ptr = nullptr;
    gtk_tree_model_get(model, &iter, COL_PKG_PTR, &ptr, -1);
    if (!ptr) return;

    auto *pkg = static_cast<PackageRepository::PackageData*>(ptr);
    int row = m_model->rowOfPackage(pkg);
    if (row >= 0) showPackageInfo(row);
}

// Shows basic info immediately, then launches a thread to fetch full details.
void MainWindow::showPackageInfo(int row) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto *pkg = m_model->getData(row);
    if (!pkg) return;

    // Build the quick header that is displayed immediately without blocking the UI.
    std::ostringstream ss;
    ss << _("Name:")         << " " << pkg->name        << "\n";
    ss << _("Version:")      << " " << pkg->version     << "\n";
    ss << _("Repository:")   << " " << pkg->repository  << "\n";
    ss << _("Description:")  << " " << pkg->description << "\n";

    bool        installed = pkg->installed();
    std::string pkgName   = pkg->name;

    auto *result      = new InfoResult();
    result->win        = this;
    result->generation = ++m_infoGeneration;
    result->infoText   = ss.str();

    // Show the basic header right away; the blocking xbps-query calls and the
    // file listing are gathered off the UI thread.
    gtk_text_buffer_set_text(m_infoBuffer, ss.str().c_str(), -1);
    gtk_text_buffer_set_text(m_filesBuffer, "", -1);

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
            ss << "\n" << _("Depends On:") << "   " << info.dependsOn       << "\n";
            ss << _("Opt Deps:")     << "   " << info.optDepends           << "\n";
            ss << _("Conflicts:")    << "   " << info.conflictsWith        << "\n";
        }
        result->infoText += ss.str();

        // Size tab: installed size (installed packages) and download size.
        std::ostringstream sizeSs;
        sizeSs << _("Name:")           << " " << pkgName                     << "\n";
        sizeSs << _("Download Size:")  << "   " << info.downloadSizeAsString << "\n";
        sizeSs << _("Installed Size:") << "   " << info.installedSizeAsString<< "\n";
        result->sizeText = sizeSs.str();

        auto files = XBPSCommand::getPackageFiles(pkgName, installed);
        std::ostringstream fss;
        for (auto &f : files) fss << f << "\n";
        result->filesText = fss.str();

        g_idle_add(info_done_cb, result);
    }).detach();
}

// Called on the main thread to update the info/files/size buffers with fetched data.
void MainWindow::applyPackageInfo(int generation, const std::string &infoText,
                                  const std::string &filesText,
                                  const std::string &sizeText) {
    // Discard stale results from a previous selection that arrived late.
    if (generation != m_infoGeneration) return;
    gtk_text_buffer_set_text(m_infoBuffer,  infoText.c_str(),  -1);
    gtk_text_buffer_set_text(m_filesBuffer, filesText.c_str(), -1);
    gtk_text_buffer_set_text(m_sizeBuffer,  sizeText.c_str(),  -1);
}
