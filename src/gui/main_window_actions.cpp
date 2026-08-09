// ----------------------------------------------------------------------------
// main_window_actions.cpp — User-triggered actions for MainWindow
//
// Contains the high-level action handlers invoked by menu items and toolbar
// buttons. Each action either modifies the view state, queues an xbps command,
// or opens a dialog.
//
// Actions:
//   - setView()           changes the active package filter
//   - do_sync()           queues a database sync
//   - do_upgrade()        queues a full system upgrade
//   - do_install()        queues the selected package for installation
//   - do_remove()         queues the selected package for removal
//   - openRepositories()  opens the repository editor
//   - do_about()          shows the About dialog
// ----------------------------------------------------------------------------

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

// Changes the active view filter and refreshes the table.
void MainWindow::setView(ViewOption v) {
    Settings::instance().setViewOption(v);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_model->applyFilter(v);
        m_model->sort(PackageModel::ColName, true);
    }
    populateTable();
    refreshStatusBar();
}

// Queues a database sync command and runs the command queue.
void MainWindow::do_sync() {
    if (m_transactionRunning) return;
    m_commandQueue.clear();
    m_commandQueue.push_back({{ XBPS_INSTALL_BIN, "-S" }, _("Syncing package databases...")});
    runCommandQueue();
}

// Queues a full system upgrade command and runs the command queue.
void MainWindow::do_upgrade() {
    if (m_transactionRunning) return;
    m_commandQueue.clear();
    m_commandQueue.push_back({{ XBPS_INSTALL_BIN, "-Syu" }, _("Upgrading system...")});
    runCommandQueue();
}

// Adds the selected package to the install transaction.
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

// Validates and adds the selected package to the remove transaction.
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

// Opens the repository editor window (creates it lazily on first call).
void MainWindow::openRepositories() {
    if (!m_repoEditor) m_repoEditor = new RepositoryEditor(m_window);
    m_repoEditor->show();
}

// Shows the About dialog.
void MainWindow::do_about() {
    GtkWidget *dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), IRUKA_APP_NAME);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), IRUKA_APP_VERSION);

    // Use the logo embedded in the binary so the dialog works even without an
    // installed icon theme.
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

// Persists the chosen UI language and rebuilds the interface so the change is
// applied immediately, without restarting the process.
void MainWindow::setLanguage(const std::string &lang) {
    if (m_transactionRunning) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "%s", _("Cannot change the language while an operation is running."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }

    // Update the LANGUAGE environment variable so gettext picks the right
    // catalog, then rebind the text domain to flush any cached lookup.
    if (!lang.empty()) {
        setenv("LANGUAGE", lang.c_str(), 1);
    } else {
        // Empty lang means "system default"; unset LANGUAGE to fall back to the
        // system locale (LC_ALL/LC_MESSAGES/LANG).
        unsetenv("LANGUAGE");
    }
    bindtextdomain(IRUKA_TEXTDOMAIN, utils::resolveLocaledir().c_str());
    bind_textdomain_codeset(IRUKA_TEXTDOMAIN, "UTF-8");
    textdomain(IRUKA_TEXTDOMAIN);

    Settings::instance().setUiLanguage(lang);
    Settings::instance().save();

    // Defer the UI rebuild until the language menu has fully closed; rebuilding
    // from inside the menu's own signal handler would destroy widgets that GTK
    // is still processing.
    auto *data = new ReloadData();
    data->win = this;
    g_idle_add(reload_ui_cb, data);
}
