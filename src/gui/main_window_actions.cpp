
#include "main_window_private.h"
#include "package.h"
#include "settings.h"
#include "repository_editor.h"
#include "package_model.h"
#include "constants.h"
#include "utils.h"
#include "logo.h"
#include "i18n.h"
#include <mutex>

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
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(
            GTK_LIST_BOX(m_categoryList), index);
        if (row) {
            g_signal_handlers_block_by_func(m_categoryList,
                (gpointer)G_CALLBACK(cb_category_selected), this);
            gtk_list_box_select_row(GTK_LIST_BOX(m_categoryList), row);
            g_signal_handlers_unblock_by_func(m_categoryList,
                (gpointer)G_CALLBACK(cb_category_selected), this);
        }
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

    GtkTreeIter  iter;
    GtkTreeModel *model = GTK_TREE_MODEL(m_listStore);
    if (!gtk_tree_selection_get_selected(m_selection, &model, &iter)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("Please select a package to update."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }

    gpointer ptr = nullptr;
    gtk_tree_model_get(model, &iter, COL_PKG_PTR, &ptr, -1);
    if (!ptr) return;

    auto *pkg = static_cast<PackageRepository::PackageData*>(ptr);
    if (!pkg->installed()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("Package is not installed."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }

    m_commandQueue.clear();
    m_commandQueue.push_back({{ XBPS_INSTALL_BIN, "-Syu", pkg->name },
                              _("Updating package...")});
    runCommandQueue();
}

void MainWindow::do_install() {
    GtkTreeIter  iter;
    GtkTreeModel *model = GTK_TREE_MODEL(m_listStore);
    if (!gtk_tree_selection_get_selected(m_selection, &model, &iter)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("Please select a package to install."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }

    gpointer ptr = nullptr;
    gtk_tree_model_get(model, &iter, COL_PKG_PTR, &ptr, -1);
    if (!ptr) return;

    auto *pkg = static_cast<PackageRepository::PackageData*>(ptr);
    if (pkg->installed()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("Package is already installed."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }
    addToTransaction(pkg->name, true);
}

void MainWindow::do_remove() {
    GtkTreeIter  iter;
    GtkTreeModel *model = GTK_TREE_MODEL(m_listStore);
    if (!gtk_tree_selection_get_selected(m_selection, &model, &iter)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("Please select a package to remove."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }

    gpointer ptr = nullptr;
    gtk_tree_model_get(model, &iter, COL_PKG_PTR, &ptr, -1);
    if (!ptr) return;

    auto *pkg = static_cast<PackageRepository::PackageData*>(ptr);
    if (!pkg->installed()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("Package is not installed."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }
    if (Package::isForbidden(pkg->name)) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
            _("Cannot remove protected package: %s"), pkg->name.c_str());
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }
    addToTransaction(pkg->name, false);
}

void MainWindow::openRepositories() {
    if (!m_repoEditor) m_repoEditor = new RepositoryEditor(m_window);
    m_repoEditor->show();
}

void MainWindow::do_about() {
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), IRUKA_APP_NAME);

    GdkPixbuf *logo = logo_pixbuf();
    if (logo) {
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), logo);
        g_object_unref(logo);
    }

    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog),
        _("A graphical package manager for XBPS.\nBuilt with GTK3."));

    const char *authors[] = { "JavierC", nullptr };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);

    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog),
        _("GUI: WTFPL\nCore/backend: BSD 3-Clause\n\n"
          "Copyright (C) 2026 JavierC"));
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_CUSTOM);

    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
    gtk_widget_show(dialog);
}

void MainWindow::setLanguage(const std::string &lang) {
    if (m_transactionRunning) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "%s", _("Cannot change the language while an operation is running."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
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
    g_idle_add(reload_ui_cb, data);
}
