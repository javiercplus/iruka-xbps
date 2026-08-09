#ifndef IRUKA_MAIN_WINDOW_PRIVATE_H
#define IRUKA_MAIN_WINDOW_PRIVATE_H

// ----------------------------------------------------------------------------
// main_window_private.h — Internal implementation header for MainWindow
//
// This header is included ONLY by main_window*.cpp files.
// It must NOT be included by any external translation unit.
// ----------------------------------------------------------------------------

#include "main_window.h"
#include "package.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <set>
#include <map>

// ----------------------------------------------------------------------------
// Column indices for the package GtkListStore.
// Must match the order passed to gtk_list_store_new() in buildPackageTable().
// ----------------------------------------------------------------------------
enum {
    COL_STATUS = 0,  // Status indicator character: ' ', '@', '!', 'I', 'X'
    COL_NAME,        // Package name string
    COL_VERSION,     // Package version string
    COL_SIZE,        // Installed size as human-readable string
    COL_DESCRIPTION, // Short package description
    COL_PKG_PTR,     // Raw PackageRepository::PackageData* pointer (G_TYPE_POINTER)
    COL_COUNT        // Total number of columns (sentinel)
};

// ----------------------------------------------------------------------------
// Thread-to-main-thread marshaling structs.
//
// Worker threads allocate these on the heap, fill them in, then pass them to
// g_idle_add() so the result is applied on the GTK main thread.
// The idle callback is responsible for deleting the struct after use.
// ----------------------------------------------------------------------------

// Carries the full package list loaded by the background refresh thread.
struct LoadResult {
    MainWindow                              *win;
    std::vector<PackageListData>             merged;
    std::set<std::string>                    unreqSet;
    std::map<std::string, OutdatedPackageInfo> outdated;
};

// Carries package info and file list fetched asynchronously on row selection.
struct InfoResult {
    MainWindow  *win;
    int          generation; // Matches MainWindow::m_infoGeneration to discard stale results
    std::string  infoText;
    std::string  filesText;
};

// Signals that a privileged command (install/remove/sync) has finished.
struct ExecResult {
    MainWindow *win;
};

// Carries a chunk of output text from a running privileged command.
struct ProgressChunk {
    MainWindow  *win;
    std::string  text;
};

// Triggers a UI rebuild on the main thread after the language menu closes.
// Deferring the rebuild avoids destroying widgets while GTK is still handling
// the menu popup.
struct ReloadData {
    MainWindow *win;
};

// ----------------------------------------------------------------------------
// g_idle_add callbacks — bridge worker-thread results to the GTK main thread.
// Defined in main_window_callbacks.cpp.
// ----------------------------------------------------------------------------
gboolean load_done_cb(gpointer data);
gboolean info_done_cb(gpointer data);
gboolean exec_done_cb(gpointer data);
gboolean progress_chunk_cb(gpointer data);
gboolean reload_ui_cb(gpointer data);

#endif // IRUKA_MAIN_WINDOW_PRIVATE_H
