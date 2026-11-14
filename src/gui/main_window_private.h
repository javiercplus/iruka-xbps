#ifndef IRUKA_MAIN_WINDOW_PRIVATE_H
#define IRUKA_MAIN_WINDOW_PRIVATE_H

#include "main_window.h"
#include "package.h"
#include <FL/Fl.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
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

class PackageBrowser : public Fl_Hold_Browser {
public:
    PackageBrowser(int X, int Y, int W, int H, const char *L = nullptr)
        : Fl_Hold_Browser(X, Y, W, H, L) {}
    MainWindow *owner = nullptr;
    int handle(int event) override;
};

class MainWindowFrame : public Fl_Window {
public:
    MainWindowFrame(int W, int H, const char *L)
        : Fl_Window(W, H, L) {}
    MainWindow *owner = nullptr;
    void resize(int X, int Y, int W, int H) override {
        Fl_Window::resize(X, Y, W, H);
        if (owner) owner->layout();
    }
};

class MainTabs : public Fl_Tabs {
public:
    MainTabs(int X, int Y, int W, int H)
        : Fl_Tabs(X, Y, W, H) {}
    void resize(int X, int Y, int W, int H) override {
        Fl_Tabs::resize(X, Y, W, H);
        for (int i = 0; i < children(); ++i) {
            Fl_Widget *page = child(i);
            page->resize(X, Y + 24, W, H - 24);
        }
    }
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
};

struct ProgressChunk {
    MainWindow  *win;
    std::string  text;
};

struct ReloadData {
    MainWindow *win;
};

void load_done_awake(void *data);
void info_done_awake(void *data);
void exec_done_awake(void *data);
void progress_chunk_awake(void *data);
void reload_ui_awake(void *data);

#endif
