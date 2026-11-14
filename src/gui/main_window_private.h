#ifndef IRUKA_MAIN_WINDOW_PRIVATE_H
#define IRUKA_MAIN_WINDOW_PRIVATE_H


#include "main_window.h"
#include "package.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <set>
#include <map>

enum {
    COL_STATUS = 0,
    COL_NAME,
    COL_VERSION,
    COL_DESCRIPTION,
    COL_PKG_PTR,
    COL_COUNT
};


struct LoadResult {
    MainWindow                              *win;
    std::vector<PackageListData>             merged;
    std::set<std::string>                    unreqSet;
    std::map<std::string, OutdatedPackageInfo> outdated;
};

struct InfoResult {
    MainWindow  *win;
    int          generation;
    std::string  infoText;
    std::string  filesText;
    std::string  sizeText;
};

struct ExecResult {
    MainWindow *win;
    std::vector<std::string> args;
    bool authFailed = false;
    bool retried = false;
};

struct ProgressChunk {
    MainWindow  *win;
    std::string  text;
};

struct ReloadData {
    MainWindow *win;
};

gboolean load_done_cb(gpointer data);
gboolean info_done_cb(gpointer data);
gboolean exec_done_cb(gpointer data);
gboolean progress_chunk_cb(gpointer data);
gboolean reload_ui_cb(gpointer data);

#endif
