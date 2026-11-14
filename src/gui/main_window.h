#ifndef IRUKA_MAIN_WINDOW_H
#define IRUKA_MAIN_WINDOW_H

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

#include <FL/Fl_Widget.H>

class Fl_Window;
class Fl_Menu_Bar;
class Fl_Button;
class Fl_Input;
class Fl_Tabs;
class Fl_Box;
class Fl_Progress;
class Fl_Browser;
class Fl_Hold_Browser;
class Fl_Text_Display;
class Fl_Text_Buffer;
class Fl_Menu_Button;
class PackageBrowser;
class RepositoryEditor;

class MainWindow {
public:
  MainWindow();
  ~MainWindow();

  void show();

  void loadPackageList();

  void do_sync();
  void do_upgrade();
  void do_update_package();
  void do_install();
  void do_remove();
  void do_about();
  void openRepositories();
  void setLanguage(const std::string &lang);
  void reloadUi();
  void setView(ViewOption v);
  void onSearch();
  void onTableSelect();
  void commitTransaction();
  void clearTransaction();

  void applyLoadedPackages(std::vector<PackageListData> &&packages,
                           std::set<std::string> &&unreqSet,
                           std::map<std::string, OutdatedPackageInfo> &&outdated);
  void applyPackageInfo(int generation, const std::string &infoText,
                        const std::string &filesText, const std::string &sizeText);
  void applyExecResult(const std::vector<std::string> &args, bool authFailed,
                       bool retried);
  void appendProgressOutput(const std::string &text);

  bool isDestroyed() const { return m_destroyed.load(); }

  void layout();
  void openContextMenu();
  void toggleDetails();

private:
  void buildUi();
  void buildMenuBar();
  void buildToolbar();
  void buildSearchBar();
  void buildPackageTable();
  void buildCategoryPanel();
  void addCategoryRow(const std::string &label, int kind,
                      const std::string &repo);
  void buildInfoPanel();
  void buildStatusBar();
  void buildPopupMenu();
  void buildProgressDialog();

  void populateTable();
  void refreshStatusBar();
  void applyFilters();
  void applyCategory(int kind, const std::string &repo);
  void onSelectionDebounced();
  void showPackageInfo(int row);
  void setPackageStatus(const std::string &pkgName, const std::string &status);
  void addToTransaction(const std::string &pkgName, bool install);
  void do_unmark();
  PackageRepository::PackageData *selectedPackage();
  void execWithPrivileges(const std::vector<std::string> &args,
                          bool retried = false);
  void runCommandQueue();
  void startNextCommand();
  void finishRun();
  void setTransactionRunning(bool running);
  void onCancelProgress();
  void onProgressToggle();

  static void cb_window_resize(Fl_Widget *w, void *d);
  static void cb_menu_all(Fl_Widget *w, void *d);
  static void cb_menu_installed(Fl_Widget *w, void *d);
  static void cb_menu_notinstalled(Fl_Widget *w, void *d);
  static void cb_menu_sync(Fl_Widget *w, void *d);
  static void cb_menu_upgrade(Fl_Widget *w, void *d);
  static void cb_menu_commit(Fl_Widget *w, void *d);
  static void cb_menu_clear(Fl_Widget *w, void *d);
  static void cb_menu_about(Fl_Widget *w, void *d);
  static void cb_menu_repositories(Fl_Widget *w, void *d);
  static void cb_menu_language_system(Fl_Widget *w, void *d);
  static void cb_menu_language_en(Fl_Widget *w, void *d);
  static void cb_menu_language_es(Fl_Widget *w, void *d);
  static void cb_menu_language_ru(Fl_Widget *w, void *d);
  static void cb_menu_language_ja(Fl_Widget *w, void *d);
  static void cb_menu_language_hu(Fl_Widget *w, void *d);
  static void cb_btn_upgrade(Fl_Widget *w, void *d);
  static void cb_btn_refresh(Fl_Widget *w, void *d);
  static void cb_btn_apply(Fl_Widget *w, void *d);
  static void cb_popup_install(Fl_Widget *w, void *d);
  static void cb_popup_update(Fl_Widget *w, void *d);
  static void cb_popup_remove(Fl_Widget *w, void *d);
  static void cb_popup_unmark(Fl_Widget *w, void *d);
  static void cb_category_selected(Fl_Widget *w, void *d);
  static void cb_search_changed(Fl_Widget *w, void *d);
  static void cb_search_debounce(void *d);
  static void cb_size_debounce(void *d);
  static void cb_table_select(Fl_Widget *w, void *d);
  static void cb_progress_pulse(void *d);
  static void cb_progress_btn(Fl_Widget *w, void *d);
  static void cb_progress_toggle(Fl_Widget *w, void *d);
  static void cb_progress_close(Fl_Widget *w, void *d);
  static void cb_toggle_details(Fl_Widget *w, void *d);
  static void cb_window_close(Fl_Widget *w, void *d);

  Fl_Window *m_window;
  Fl_Menu_Bar *m_menuBar;
  Fl_Button *m_btnUpgrade;
  Fl_Button *m_btnRefresh;
  Fl_Button *m_btnApply;
  Fl_Input *m_searchEntry;

  PackageBrowser *m_pkgBrowser;
  int m_pkgColWidths[4] = {0, 0, 0, 0};

  Fl_Hold_Browser *m_categoryList;
  struct CategoryRow {
    int kind;
    std::string value;
  };
  std::vector<CategoryRow> m_categoryRows;
  int m_selectedCategoryIndex = 0;
  std::string m_selectedCategory;
  bool m_detailsVisible = true;

  Fl_Tabs *m_notebook;
  Fl_Text_Display *m_infoView;
  Fl_Text_Buffer *m_infoBuffer;
  Fl_Text_Display *m_filesView;
  Fl_Text_Buffer *m_filesBuffer;
  Fl_Text_Display *m_sizeView;
  Fl_Text_Buffer *m_sizeBuffer;
  Fl_Browser *m_transList;

  Fl_Menu_Button *m_popupMenu;

  Fl_Box *m_statusbar;

  RepositoryEditor *m_repoEditor = nullptr;

  PackageRepository m_repo;
  PackageModel *m_model;
  std::vector<std::string> m_transactionInstall;
  std::vector<std::string> m_transactionRemove;

  std::set<std::string> m_installMarks;
  std::set<std::string> m_removeMarks;

  struct ExecStep {
    std::vector<std::string> args;
    std::string label;
  };
  std::vector<ExecStep> m_commandQueue;
  bool m_transactionRunning = false;

  Fl_Window *m_progressDialog = nullptr;
  Fl_Box *m_progressLabel;
  Fl_Progress *m_progressBar;
  Fl_Button *m_progressToggle;
  Fl_Text_Display *m_progressView;
  Fl_Text_Buffer *m_progressBuffer;
  Fl_Button *m_progressButton;
  bool m_progressDetailsVisible = false;
  double m_progressPulse = 0.0;

  std::atomic<int> m_activeChild{0};
  std::atomic<bool> m_cancelRequested{false};

  int m_infoGeneration = 0;

  std::atomic<bool> m_destroyed{false};

  std::mutex m_mutex;
  std::atomic<bool> m_loading{false};

  bool m_rebuilding = false;
};

#endif
