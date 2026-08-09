// ----------------------------------------------------------------------------
// main_window_callbacks.cpp — GTK signal callbacks for MainWindow
//
// Contains:
//   - g_idle_add bridge callbacks (load_done_cb, info_done_cb, etc.)
//   - All static MainWindow::cb_* GTK signal handler implementations
// ----------------------------------------------------------------------------

#include "main_window_private.h"
#include "settings.h"

// ============================================================================
// g_idle_add bridge callbacks
// These are called on the GTK main thread after a worker thread completes.
// ============================================================================

// Delivers loaded package data to the main thread after background fetch.
gboolean load_done_cb(gpointer data) {
    auto *r = static_cast<LoadResult*>(data);
    // Skip if the window was destroyed while the thread was running.
    if (!r->win->isDestroyed()) {
        r->win->applyLoadedPackages(std::move(r->merged), std::move(r->unreqSet),
                                    std::move(r->outdated));
    }
    delete r;
    return G_SOURCE_REMOVE;
}

// Delivers fetched package info text to the main thread.
gboolean info_done_cb(gpointer data) {
    auto *r = static_cast<InfoResult*>(data);
    // Skip if the window was destroyed while the thread was running.
    if (!r->win->isDestroyed()) {
        r->win->applyPackageInfo(r->generation, r->infoText, r->filesText, r->sizeText);
    }
    delete r;
    return G_SOURCE_REMOVE;
}

// Signals the main thread that a privileged command has finished.
gboolean exec_done_cb(gpointer data) {
    auto *r = static_cast<ExecResult*>(data);
    // Skip if the window was destroyed while the thread was running.
    if (!r->win->isDestroyed()) {
        r->win->applyExecResult();
    }
    delete r;
    return G_SOURCE_REMOVE;
}

// Delivers a chunk of command output text to the main thread.
gboolean progress_chunk_cb(gpointer data) {
    auto *r = static_cast<ProgressChunk*>(data);
    // Skip if the window was destroyed while the thread was running.
    if (!r->win->isDestroyed()) {
        r->win->appendProgressOutput(r->text);
    }
    delete r;
    return G_SOURCE_REMOVE;
}

// Rebuilds the UI after a language change, once the menu popup has closed.
gboolean reload_ui_cb(gpointer data) {
    auto *r = static_cast<ReloadData*>(data);
    if (!r->win->isDestroyed()) {
        r->win->reloadUi();
    }
    delete r;
    return G_SOURCE_REMOVE;
}

// ============================================================================
// View menu callbacks
// ============================================================================

void MainWindow::cb_menu_all(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->setView(ViewOption::All);
}
void MainWindow::cb_menu_installed(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->setView(ViewOption::Installed);
}
void MainWindow::cb_menu_notinstalled(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->setView(ViewOption::NotInstalled);
}

// ============================================================================
// Transaction menu callbacks
// ============================================================================

void MainWindow::cb_menu_sync(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_sync();
}
void MainWindow::cb_menu_upgrade(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_upgrade();
}
void MainWindow::cb_menu_commit(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->commitTransaction();
}
void MainWindow::cb_menu_clear(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->clearTransaction();
}

// ============================================================================
// Options / Help menu callbacks
// ============================================================================

void MainWindow::cb_menu_about(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_about();
}
void MainWindow::cb_menu_repositories(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->openRepositories();
}

// Options > Language radio item; only reacts to the item that was activated.
void MainWindow::cb_menu_language(GtkWidget *w, gpointer d) {
    if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) return;
    auto *win = static_cast<MainWindow*>(d);
    const gchar *lang = static_cast<const gchar*>(g_object_get_data(G_OBJECT(w), "lang"));
    win->setLanguage(lang ? lang : "");
}

// ============================================================================
// Toolbar button callbacks
// ============================================================================

void MainWindow::cb_btn_sync(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_sync();
}
void MainWindow::cb_btn_upgrade(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_upgrade();
}
void MainWindow::cb_btn_refresh(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->loadPackageList();
}
void MainWindow::cb_btn_apply(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->commitTransaction();
}

// ============================================================================
// Context-menu (popup) callbacks
// ============================================================================

void MainWindow::cb_popup_install(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_install();
}
void MainWindow::cb_popup_remove(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->do_remove();
}

// ============================================================================
// Search callbacks
// ============================================================================

void MainWindow::cb_search(GtkWidget*, gpointer d) {
    static_cast<MainWindow*>(d)->onSearch();
}

// Resets and restarts the 300ms debounce timer on every keystroke.
void MainWindow::cb_search_changed(GtkWidget*, gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    if (win->m_searchDebounceId) {
        g_source_remove(win->m_searchDebounceId);
        win->m_searchDebounceId = 0;
    }
    win->m_searchDebounceId = g_timeout_add(300, cb_search_debounce, win);
}

// Fires when the debounce timer expires; triggers the actual search.
gboolean MainWindow::cb_search_debounce(gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    win->m_searchDebounceId = 0;
    win->onSearch();
    return G_SOURCE_REMOVE;
}

// Fires when the selection debounce expires; loads info/files/size for the
// currently selected package.
gboolean MainWindow::cb_size_debounce(gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    win->m_sizeDebounceId = 0;
    win->onSelectionDebounced();
    return G_SOURCE_REMOVE;
}

// ============================================================================
// Package table callbacks
// ============================================================================

void MainWindow::cb_table_select(GtkTreeSelection*, gpointer d) {
    static_cast<MainWindow*>(d)->onTableSelect();
}

void MainWindow::cb_row_activated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer d) {
    static_cast<MainWindow*>(d)->onTableSelect();
}

// Handles right-click on the tree view: selects the clicked row and shows the popup menu.
gboolean MainWindow::cb_tree_button_press(GtkWidget *tv, GdkEventButton *event, gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    if (event->type != GDK_BUTTON_PRESS || event->button != GDK_BUTTON_SECONDARY)
        return FALSE;

    GtkTreePath *path = nullptr;
    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tv),
            (gint)event->x, (gint)event->y, &path, nullptr, nullptr, nullptr)) {
        gtk_tree_selection_unselect_all(win->m_selection);
        gtk_tree_selection_select_path(win->m_selection, path);
        gtk_tree_path_free(path);
    }
    // Use gtk_menu_popup_at_pointer (replaces the deprecated gtk_menu_popup).
    gtk_menu_popup_at_pointer(GTK_MENU(win->m_popupMenu), (GdkEvent*)event);
    return TRUE;
}

// ============================================================================
// Progress dialog callbacks
// ============================================================================

// Pulses the progress bar animation while a command is running.
gboolean MainWindow::cb_progress_pulse(gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(win->m_progressBar));
    return G_SOURCE_CONTINUE;
}

// Prevents the progress dialog from being closed while a transaction is running.
gboolean MainWindow::cb_progress_delete(GtkWidget*, GdkEvent*, gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    if (win->m_transactionRunning) return TRUE;
    gtk_widget_hide(win->m_progressDialog);
    return TRUE;
}

// Cancels the running command or closes the dialog if idle.
void MainWindow::cb_progress_btn(GtkWidget*, gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    if (win->m_transactionRunning) {
        win->onCancelProgress();
    } else {
        gtk_widget_hide(win->m_progressDialog);
    }
}

// ============================================================================
// View menu toggle callback
// ============================================================================

void MainWindow::cb_toggle_details(GtkCheckMenuItem*, gpointer d) {
    static_cast<MainWindow*>(d)->toggleDetails();
}

// ============================================================================
// Tree-view column resize callback
// ============================================================================

// Persists the user-adjusted column widths whenever the user drags a column
// separator. The values are written to disk by Settings::save() on exit.
void MainWindow::cb_col_width_changed(GtkTreeViewColumn*, GParamSpec*, gpointer d) {
    auto *win = static_cast<MainWindow*>(d);
    Settings::instance().setColumnWidths(
        win->m_colName    ? gtk_tree_view_column_get_width(win->m_colName)    : 250,
        win->m_colVersion ? gtk_tree_view_column_get_width(win->m_colVersion) : 120,
        Settings::instance().sizeColumnWidth());
}
