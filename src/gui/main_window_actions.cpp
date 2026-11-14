#include "main_window_private.h"
#include "package.h"
#include "settings.h"
#include "repository_editor.h"
#include "package_model.h"
#include "constants.h"
#include "utils.h"
#include "logo.h"
#include "i18n.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Browser.H>
#include <FL/fl_ask.H>
#include <cstdlib>
#include <mutex>

namespace {

class LogoBox : public Fl_Box {
public:
    LogoBox(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H) {}
    ~LogoBox() override { delete m_owned; }
    void setOwned(Fl_Image *img) {
        m_owned = img;
        image(img);
    }
private:
    Fl_Image *m_owned = nullptr;
};

void about_close(Fl_Widget *w, void *) {
    Fl::delete_widget(w->window());
}

}

void MainWindow::setView(ViewOption v) {
    Settings::instance().setViewOption(v);
    m_selectedCategory.clear();

    if (m_categoryList) {
        int index = 0;
        switch (v) {
            case ViewOption::Installed:    index = 1; break;
            case ViewOption::NotInstalled: index = 2; break;
            case ViewOption::Outdated:     index = 3; break;
            default:                       index = 0; break;
        }
        m_selectedCategoryIndex = index;
        m_categoryList->value(index + 1);
    }

    applyFilters();
}

void MainWindow::do_sync() {
    if (m_transactionRunning) return;
    m_commandQueue.clear();
    m_commandQueue.push_back({{ XBPS_INSTALL_BIN, "-S" }, _("Syncing package databases...")});
    runCommandQueue();
}

void MainWindow::do_upgrade() {
    if (m_transactionRunning) return;
    m_commandQueue.clear();
    m_commandQueue.push_back({{ XBPS_INSTALL_BIN, "-Syu" }, _("Upgrading system...")});
    runCommandQueue();
}

void MainWindow::do_update_package() {
    if (m_transactionRunning) return;

    auto *pkg = selectedPackage();
    if (!pkg) {
        fl_message("%s", _("Please select a package to update."));
        return;
    }
    if (!pkg->installed()) {
        fl_message("%s", _("Package is not installed."));
        return;
    }

    m_commandQueue.clear();
    m_commandQueue.push_back({{ XBPS_INSTALL_BIN, "-Syu", pkg->name },
                              _("Updating package...")});
    runCommandQueue();
}

void MainWindow::do_install() {
    auto *pkg = selectedPackage();
    if (!pkg) {
        fl_message("%s", _("Please select a package to install."));
        return;
    }
    if (pkg->installed()) {
        fl_message("%s", _("Package is already installed."));
        return;
    }
    addToTransaction(pkg->name, true);
}

void MainWindow::do_remove() {
    auto *pkg = selectedPackage();
    if (!pkg) {
        fl_message("%s", _("Please select a package to remove."));
        return;
    }
    if (!pkg->installed()) {
        fl_message("%s", _("Package is not installed."));
        return;
    }
    if (Package::isForbidden(pkg->name)) {
        fl_alert(_("Cannot remove protected package: %s"), pkg->name.c_str());
        return;
    }
    addToTransaction(pkg->name, false);
}

void MainWindow::openRepositories() {
    if (!m_repoEditor) m_repoEditor = new RepositoryEditor(m_window);
    m_repoEditor->show();
}

void MainWindow::do_about() {
    Fl_Window *dlg = new Fl_Window(400, 330, _("About"));
    dlg->begin();

    LogoBox *logo = new LogoBox(160, 16, 80, 80);
    Fl_Image *img = logo_image();
    if (img) logo->setOwned(img);

    std::string text = std::string(IRUKA_APP_NAME) + " " + IRUKA_APP_VERSION;
    text += "\n\n";
    text += _("A graphical package manager for XBPS.\nBuilt with FLTK.");
    text += "\n\n";
    text += _("Authors:") + std::string(" JavierC");
    text += "\n\n";
    text += _("GUI: WTFPL\nCore/backend: BSD 3-Clause\n\n"
             "Copyright (C) 2026 JavierC");

    Fl_Box *info = new Fl_Box(20, 108, 360, 160, "");
    info->copy_label(text.c_str());
    info->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    Fl_Button *close = new Fl_Button(160, 284, 80, 28, _("Close"));
    close->callback(about_close, nullptr);

    dlg->end();
    dlg->callback(about_close, nullptr);
    dlg->show();
}

void MainWindow::setLanguage(const std::string &lang) {
    if (m_transactionRunning) {
        fl_message("%s", _("Cannot change the language while an operation is running."));
        return;
    }

    if (!lang.empty()) {
        setenv("LANGUAGE", lang.c_str(), 1);
    } else {
        unsetenv("LANGUAGE");
    }
    bindtextdomain(IRUKA_TEXTDOMAIN, utils::resolveLocaledir().c_str());
    bind_textdomain_codeset(IRUKA_TEXTDOMAIN, "UTF-8");
    textdomain(IRUKA_TEXTDOMAIN);

    Settings::instance().setUiLanguage(lang);
    Settings::instance().save();

    auto *data = new ReloadData();
    data->win = this;
    Fl::awake(reload_ui_awake, data);
}
