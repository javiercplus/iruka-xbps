// ----------------------------------------------------------------------------
// main_window.cpp — MainWindow lifecycle (constructor, destructor, show)
//
// The MainWindow implementation is split across several files for clarity:
//
//   main_window_private.h      — internal structs, COL_* enum, idle-callback decls
//   main_window_callbacks.cpp  — all static GTK cb_* signal handlers
//   main_window_ui.cpp         — widget construction (buildXxx methods)
//   main_window_packages.cpp   — package loading, display, and info panel
//   main_window_transaction.cpp— transaction management (queue, commit, clear)
//   main_window_exec.cpp       — privileged command execution and progress view
//   main_window_actions.cpp    — user-facing actions (sync, upgrade, install…)
// ----------------------------------------------------------------------------

#include "main_window_private.h"
#include "settings.h"
#include "repository_editor.h"

// Constructs the main window: initializes the model, builds the UI, and loads
// the package list from xbps in a background thread.
MainWindow::MainWindow()
    : m_popupMenu(nullptr), m_statusContextId(0), m_model(nullptr)
{
    m_model = new PackageModel(m_repo);
    buildUi();
    loadPackageList();
}

// Destructor: signals in-flight threads, then frees model and child editor.
MainWindow::~MainWindow() {
    // Signal any in-flight worker threads that the window is gone,
    // so that pending g_idle_add callbacks will skip their work safely.
    m_destroyed = true;
    delete m_repoEditor;
    delete m_model;
}

// Shows the window and sets the initial pane split position based on saved height.
void MainWindow::show() {
    gtk_widget_show_all(m_window);
    m_lastPanedPosition = static_cast<int>(Settings::instance().windowHeight() * 0.62);
    gtk_paned_set_position(GTK_PANED(m_mainPaned), m_lastPanedPosition);
}

// Rebuilds the entire UI in place so that language changes apply immediately
// without spawning a new process (which previously caused duplicate windows).
// All background worker threads are safe here: they only post results via
// g_idle_add, which cannot run while this synchronous rebuild is executing.
void MainWindow::reloadUi() {
    if (m_rebuilding || m_transactionRunning || m_destroyed.load()) return;
    m_rebuilding = true;

    // Preserve the details-panel visibility across the rebuild.
    bool detailsVisible =
        gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(m_viewDetailsItem));

    // Cancel pending timers that reference widgets about to be destroyed.
    if (m_searchDebounceId) {
        g_source_remove(m_searchDebounceId);
        m_searchDebounceId = 0;
    }
    if (m_sizeDebounceId) {
        g_source_remove(m_sizeDebounceId);
        m_sizeDebounceId = 0;
    }
    if (m_progressTimerId) {
        g_source_remove(m_progressTimerId);
        m_progressTimerId = 0;
    }

    // Discard any stale async package-info results bound to the old buffers.
    ++m_infoGeneration;

    // Close a repository editor window if it is open.
    if (m_repoEditor) {
        m_repoEditor->close();
        delete m_repoEditor;
        m_repoEditor = nullptr;
    }

    // The old window's "destroy" handler would quit the app; drop it first.
    g_signal_handlers_disconnect_by_func(m_window, (gpointer)G_CALLBACK(gtk_main_quit), nullptr);
    gtk_widget_destroy(m_window);
    if (m_progressDialog) gtk_widget_destroy(m_progressDialog);

    // Recreate the model over the same repository data and rebuild every widget.
    delete m_model;
    m_model = new PackageModel(m_repo);

    buildUi();
    show();

    // Re-apply the preserved details-panel state (hidden unless it was visible).
    // set_active() fires the "toggled" handler, which performs the actual hide.
    if (!detailsVisible) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_viewDetailsItem), FALSE);
    }

    // Restore any still-queued transaction entries into the rebuilt tab.
    GtkTreeIter iter;
    for (const auto &name : m_transactionInstall) {
        gtk_list_store_append(m_transStore, &iter);
        gtk_list_store_set(m_transStore, &iter, 0, "I", 1, name.c_str(), -1);
    }
    for (const auto &name : m_transactionRemove) {
        gtk_list_store_append(m_transStore, &iter);
        gtk_list_store_set(m_transStore, &iter, 0, "X", 1, name.c_str(), -1);
    }

    // Repopulate the table from the cached repository data, then refresh in the
    // background (no-op if a loader thread is already in flight).
    populateTable();
    refreshStatusBar();
    loadPackageList();

    m_rebuilding = false;
}
