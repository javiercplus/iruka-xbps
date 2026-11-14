#include "main_window_private.h"
#include "constants.h"
#include "i18n.h"
#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <FL/fl_ask.H>
#include <string>
#include <vector>
#include <algorithm>

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

PackageRepository::PackageData *MainWindow::selectedPackage() {
    int line = m_pkgBrowser->value();
    if (line < 2) return nullptr;
    return static_cast<PackageRepository::PackageData *>(m_pkgBrowser->data(line));
}

void MainWindow::addToTransaction(const std::string &pkgName, bool install) {
    if (install) {
        m_transactionInstall.push_back(pkgName);
        m_installMarks.insert(pkgName);
    } else {
        m_transactionRemove.push_back(pkgName);
        m_removeMarks.insert(pkgName);
    }

    std::string op = install ? "I" : "X";
    std::string line = op + "\t" + pkgName;
    m_transList->add(line.c_str());

    setPackageStatus(pkgName, op);
}

void MainWindow::do_unmark() {
    auto *pkg = selectedPackage();
    if (!pkg) return;

    const std::string name = pkg->name;

    bool changed = false;
    auto eraseFrom = [&](std::vector<std::string> &v) {
        auto it = std::find(v.begin(), v.end(), name);
        if (it != v.end()) { v.erase(it); changed = true; }
    };
    eraseFrom(m_transactionInstall);
    eraseFrom(m_transactionRemove);
    if (m_installMarks.erase(name) > 0) changed = true;
    if (m_removeMarks.erase(name) > 0) changed = true;
    if (!changed) return;

    for (int i = 1; i <= m_transList->size(); ) {
        const char *txt = m_transList->text(i);
        std::string row = txt ? txt : "";
        std::string rowName = row;
        auto tab = row.find('\t');
        if (tab != std::string::npos) rowName = row.substr(tab + 1);

        if (rowName == name) {
            m_transList->remove(i);
        } else {
            ++i;
        }
    }

    std::string status = pkg->outdated() ? "!" : (pkg->installed() ? "@" : " ");
    setPackageStatus(name, status);
}

void MainWindow::setPackageStatus(const std::string &pkgName,
                                  const std::string &status) {
    for (int i = 2; i <= m_pkgBrowser->size(); ++i) {
        auto *pkg = static_cast<PackageRepository::PackageData *>(
            m_pkgBrowser->data(i));
        if (pkg && pkgName == pkg->name) {
            std::string line = packageLine(status, pkg);
            m_pkgBrowser->text(i, line.c_str());
            return;
        }
    }
}

void MainWindow::commitTransaction() {
    if (m_transactionRunning) return;

    if (m_transactionInstall.empty() && m_transactionRemove.empty()) {
        fl_message("%s", _("No pending transactions."));
        return;
    }

    std::string msg = std::string(_("Execute transaction?")) + "\n\n";
    if (!m_transactionInstall.empty()) {
        msg += _("Install: ");
        for (auto &p : m_transactionInstall) msg += p + " ";
        msg += "\n";
    }
    if (!m_transactionRemove.empty()) {
        msg += _("Remove: ");
        for (auto &p : m_transactionRemove) msg += p + " ";
        msg += "\n";
    }

    int choice = fl_choice("%s", _("Cancel"), _("OK"), nullptr, msg.c_str());
    if (choice != 1) return;

    m_commandQueue.clear();
    if (!m_transactionInstall.empty()) {
        std::vector<std::string> installArgs = { XBPS_INSTALL_BIN, "-y" };
        for (auto &p : m_transactionInstall) installArgs.push_back(p);
        m_commandQueue.push_back({ installArgs, _("Installing packages...") });
    }
    if (!m_transactionRemove.empty()) {
        std::vector<std::string> removeArgs = { XBPS_REMOVE_BIN, "-R", "-y" };
        for (auto &p : m_transactionRemove) removeArgs.push_back(p);
        m_commandQueue.push_back({ removeArgs, _("Removing packages...") });
    }

    clearTransaction();
    runCommandQueue();
}

void MainWindow::clearTransaction() {
    m_transactionInstall.clear();
    m_transactionRemove.clear();
    m_installMarks.clear();
    m_removeMarks.clear();
    m_transList->clear();
    populateTable();
}
