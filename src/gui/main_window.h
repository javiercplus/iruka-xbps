#ifndef IRUKA_MAIN_WINDOW_H
#define IRUKA_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "package_repository.h"
#include "package_model.h"
#include "constants.h"

class RepositoryEditor;

class MainWindow {
public:
  MainWindow();
  ~MainWindow();

  void show();

  // Called from worker threads (guarded, safe)
  void loadPackageList();

  // UI actions (main thread)
  void do_sync();
  void do_upgrade();
  void do_install();
  void do_remove();
  void do_about();
  void openRepositories();
  void setLanguage(const std::string &lang);
  // Rebuilds the whole UI in place (used to apply a language change without
  // restarting the process). Safe to call only from the main thread.
  void reloadUi();
  void setView(ViewOption v);
  void onSearch();
  void onTableSelect();
  void commitTransaction();
  void clearTransaction();

  // Marshaled results (main thread)
  void applyLoadedPackages(std::vector<PackageListData> &&packages,
                           std::set<std::string> &&unreqSet,
                           std::map<std::string, OutdatedPackageInfo> &&outdated);
  void applyPackageInfo(int generation, const std::string &infoText,
                        const std::string &filesText, const std::string &sizeText);
  void applyExecResult();
  void appendProgressOutput(const std::string &text);

  // Returns true if the window has been destroyed.
  // Used by free-function g_idle_add callbacks to skip work on a dead window.
  bool isDestroyed() const { return m_destroyed.load(); }

private:
  void buildUi();
  void buildMenuBar();
  void buildToolbar();
  void buildSearchBar();
  void buildPackageTable();
  void buildInfoPanel();
  void buildStatusBar();
  void buildPopupMenu();

  void populateTable();
  void refreshStatusBar();
  void onSelectionDebounced();
  void showPackageInfo(int row);
  void setPackageStatus(const std::string &pkgName, const std::string &status);
  void addToTransaction(const std::string &pkgName, bool install);
  void execWithPrivileges(const std::vector<std::string> &args);
  void runCommandQueue();
  void startNextCommand();
  void finishRun();
  void setTransactionRunning(bool running);
  void buildProgressDialog();
  void onCancelProgress();
  void toggleDetails();

  // --- Callbacks ---
  static void cb_menu_all(GtkWidget *w, gpointer d);
  static void cb_menu_installed(GtkWidget *w, gpointer d);
  static void cb_menu_notinstalled(GtkWidget *w, gpointer d);
  static void cb_menu_sync(GtkWidget *w, gpointer d);
  static void cb_menu_upgrade(GtkWidget *w, gpointer d);
  static void cb_menu_commit(GtkWidget *w, gpointer d);
  static void cb_menu_clear(GtkWidget *w, gpointer d);
  static void cb_menu_about(GtkWidget *w, gpointer d);
  static void cb_menu_repositories(GtkWidget *w, gpointer d);
  static void cb_menu_language(GtkWidget *w, gpointer d);
  static void cb_btn_sync(GtkWidget *w, gpointer d);
  static void cb_btn_upgrade(GtkWidget *w, gpointer d);
  static void cb_btn_refresh(GtkWidget *w, gpointer d);
  static void cb_btn_apply(GtkWidget *w, gpointer d);
  static void cb_popup_install(GtkWidget *w, gpointer d);
  static void cb_popup_remove(GtkWidget *w, gpointer d);
  static void cb_search(GtkWidget *w, gpointer d);
  static void cb_search_changed(GtkWidget *w, gpointer d);
  static gboolean cb_search_debounce(gpointer d);
  static gboolean cb_size_debounce(gpointer d);
  static void cb_table_select(GtkTreeSelection *sel, gpointer d);
  static void cb_row_activated(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, gpointer d);
  static gboolean cb_tree_button_press(GtkWidget *tv, GdkEventButton *event, gpointer d);
  static gboolean cb_progress_pulse(gpointer d);
  static gboolean cb_progress_delete(GtkWidget *w, GdkEvent *event, gpointer d);
  static void cb_progress_btn(GtkWidget *w, gpointer d);
  static void cb_toggle_details(GtkCheckMenuItem *item, gpointer d);
  static void cb_col_width_changed(GtkTreeViewColumn *col, GParamSpec *pspec, gpointer d);

  // --- Widgets ---
  GtkWidget *m_window;
  GtkWidget *m_menuBar;
  GtkWidget *m_toolbar;
  GtkWidget *m_searchBar;
  GtkWidget *m_btnSync;
  GtkWidget *m_btnUpgrade;
  GtkWidget *m_btnRefresh;
  GtkWidget *m_btnApply;
  GtkWidget *m_searchEntry;

  GtkWidget *m_treeView;
  GtkListStore *m_listStore;
  GtkTreeSelection *m_selection;

  // Package table columns (kept to persist user-adjusted widths).
  GtkTreeViewColumn *m_colName = nullptr;
  GtkTreeViewColumn *m_colVersion = nullptr;

  GtkWidget *m_notebook;
  GtkWidget *m_mainPaned;
  int m_lastPanedPosition = 0;
  GtkWidget *m_viewDetailsItem;
  GtkWidget *m_infoView;
  GtkTextBuffer *m_infoBuffer;
  GtkWidget *m_filesView;
  GtkTextBuffer *m_filesBuffer;
  GtkWidget *m_sizeView;
  GtkTextBuffer *m_sizeBuffer;
  GtkWidget *m_transTree;
  GtkListStore *m_transStore;

  GtkWidget *m_popupMenu;

  GtkWidget *m_statusbar;
  guint m_statusContextId;

  RepositoryEditor *m_repoEditor = nullptr;

  // --- State ---
  PackageRepository m_repo;
  PackageModel *m_model;
  std::vector<std::string> m_transactionInstall;
  std::vector<std::string> m_transactionRemove;

  // Marks shown in the package list status column
  std::set<std::string> m_installMarks;
  std::set<std::string> m_removeMarks;

  // Sequential command queue (run one at a time)
  struct ExecStep {
    std::vector<std::string> args;
    std::string label;
  };
  std::vector<ExecStep> m_commandQueue;
  bool m_transactionRunning = false;

  // Progress dialog
  GtkWidget *m_progressDialog = nullptr;
  GtkWidget *m_progressLabel;
  GtkWidget *m_progressBar;
  GtkWidget *m_progressExpander;
  GtkWidget *m_progressView;
  GtkTextBuffer *m_progressBuffer;
  GtkWidget *m_progressButton;
  guint m_progressTimerId = 0;

  // Cancel support
  std::atomic<int> m_activeChild{0};
  std::atomic<bool> m_cancelRequested{false};

  // Search debounce
  guint m_searchDebounceId = 0;

  // Selection debounce for the Size notebook tab (avoids stale lookups when
  // the user rapidly selects several rows).
  guint m_sizeDebounceId = 0;

  // Stale async package-info guard
  int m_infoGeneration = 0;

  // Guards against callbacks firing after the window has been destroyed.
  std::atomic<bool> m_destroyed{false};

  std::mutex m_mutex;
  std::atomic<bool> m_loading{false};

  // Guards against re-entrant UI rebuilds (rapid consecutive language changes).
  bool m_rebuilding = false;
};

#endif
