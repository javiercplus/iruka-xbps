#include "main_window_private.h"
#include "settings.h"
#include "constants.h"
#include "category.h"
#include "i18n.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_draw.H>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

void MainWindow::buildUi() {
    m_window = new MainWindowFrame(Settings::instance().windowWidth(),
                                   Settings::instance().windowHeight(),
                                   _(IRUKA_APP_TITLE));
    static_cast<MainWindowFrame *>(m_window)->owner = this;
    m_window->callback(cb_window_close, this);

    buildMenuBar();
    buildToolbar();
    buildSearchBar();
    buildPackageTable();
    buildCategoryPanel();
    buildInfoPanel();
    buildStatusBar();
    buildProgressDialog();

    m_window->end();
    layout();
}

void MainWindow::buildMenuBar() {
    m_menuBar = new Fl_Menu_Bar(0, 0, 100, 28);

    const std::string &uiLang = Settings::instance().uiLanguage();

    Fl_Menu_Item items[] = {
        {_("View"), 0, 0, 0, FL_SUBMENU},
            {_("All packages"), 0, cb_menu_all, this, 0},
            {_("Installed only"), 0, cb_menu_installed, this, 0},
            {_("Not installed"), 0, cb_menu_notinstalled, this, FL_MENU_DIVIDER},
            {_("Show details panel"), 0, cb_toggle_details, this,
                static_cast<int>(FL_MENU_TOGGLE |
                    (m_detailsVisible ? FL_MENU_VALUE : 0))},
            {0},
        {_("Packages"), 0, 0, 0, FL_SUBMENU},
            {_("Mark for install"), 0, cb_popup_install, this, 0},
            {_("Mark for uninstall"), 0, cb_popup_remove, this, 0},
            {_("Update package"), 0, cb_popup_update, this, 0},
            {_("Unmark"), 0, cb_popup_unmark, this, FL_MENU_DIVIDER},
            {_("Apply"), 0, cb_menu_commit, this, 0},
            {_("Clear actions"), 0, cb_menu_clear, this, FL_MENU_DIVIDER},
            {_("Sync databases"), 0, cb_menu_sync, this, 0},
            {_("System upgrade"), 0, cb_menu_upgrade, this, 0},
            {0},
        {_("Options"), 0, 0, 0, FL_SUBMENU},
            {_("Repositories"), 0, cb_menu_repositories, this, FL_MENU_DIVIDER},
            {_("Language"), 0, 0, 0, FL_SUBMENU},
                {_("System (default)"), 0, cb_menu_language_system, this,
                    uiLang.empty() ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO},
                {_("English"), 0, cb_menu_language_en, this,
                    uiLang == "en" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO},
                {_("\xC3\x89spanol"), 0, cb_menu_language_es, this,
                    uiLang == "es" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO},
                {_("\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9"), 0, cb_menu_language_ru, this,
                    uiLang == "ru" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO},
                {_("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E"), 0, cb_menu_language_ja, this,
                    uiLang == "ja" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO},
                {_("Magyar"), 0, cb_menu_language_hu, this,
                    uiLang == "hu" ? FL_MENU_RADIO | FL_MENU_VALUE : FL_MENU_RADIO},
                {0},
            {0},
        {_("Help"), 0, 0, 0, FL_SUBMENU},
            {_("About"), 0, cb_menu_about, this, 0},
            {0},
        {0}
    };

    m_menuBar->copy(items);
}

void MainWindow::buildToolbar() {
    m_btnUpgrade = new Fl_Button(0, 0, 10, 10, _("Update"));
    m_btnUpgrade->tooltip(_("Upgrade all system packages"));
    m_btnUpgrade->callback(cb_btn_upgrade, this);

    m_btnRefresh = new Fl_Button(0, 0, 10, 10, _("Refresh"));
    m_btnRefresh->tooltip(_("Refresh package list"));
    m_btnRefresh->callback(cb_btn_refresh, this);

    m_btnApply = new Fl_Button(0, 0, 10, 10, _("Apply"));
    m_btnApply->tooltip(_("Apply pending transaction"));
    m_btnApply->callback(cb_btn_apply, this);
}

void MainWindow::buildSearchBar() {
    m_searchEntry = new Fl_Input(0, 0, 10, 10);
    m_searchEntry->when(FL_WHEN_CHANGED);
    m_searchEntry->callback(cb_search_changed, this);
}

void MainWindow::buildPackageTable() {
    m_pkgBrowser = new PackageBrowser(0, 0, 10, 10);
    m_pkgBrowser->owner = this;
    m_pkgBrowser->column_char('\t');
    m_pkgBrowser->column_widths(m_pkgColWidths);
    m_pkgBrowser->when(FL_WHEN_CHANGED);
    m_pkgBrowser->callback(cb_table_select, this);
    m_pkgBrowser->has_scrollbar(Fl_Browser_::VERTICAL);
    m_pkgBrowser->format_char(0);

    buildPopupMenu();
}

void MainWindow::buildPopupMenu() {
    m_popupMenu = new Fl_Menu_Button(0, 0, 0, 0);
    // An invisible context menu: with FL_NO_BOX the FLTK popup() honours the
    // current mouse position instead of the (zero-sized) widget position.
    m_popupMenu->box(FL_NO_BOX);
    m_popupMenu->type(Fl_Menu_Button::POPUP3);
    Fl_Menu_Item items[] = {
        {_("Install"), 0, cb_popup_install, this, 0},
        {_("Update package"), 0, cb_popup_update, this, 0},
        {_("Remove"), 0, cb_popup_remove, this, FL_MENU_DIVIDER},
        {_("Unmark"), 0, cb_popup_unmark, this, 0},
        {0}
    };
    m_popupMenu->copy(items);
}

void MainWindow::addCategoryRow(const std::string &label, int kind,
                                const std::string &repo) {
    m_categoryRows.push_back({kind, repo});
    m_categoryList->add(label.c_str());
}

void MainWindow::buildCategoryPanel() {
    m_categoryList = new Fl_Hold_Browser(0, 0, 10, 10);
    m_categoryList->when(FL_WHEN_CHANGED);
    m_categoryList->format_char(0);
    m_categoryList->has_scrollbar(Fl_Browser_::VERTICAL);
    m_categoryList->callback(cb_category_selected, this);

    m_categoryRows.clear();
    addCategoryRow(_("All packages"), 0, "");
    addCategoryRow(_("Installed"), 1, "");
    addCategoryRow(_("Not installed"), 2, "");
    addCategoryRow(_("Outdated"), 3, "");
    addCategoryRow(_("Categories"), -1, "");

    for (const auto &cat : category::all())
        addCategoryRow(_(cat.c_str()), 5, cat);

    int index = 0;
    switch (Settings::instance().viewOption()) {
        case ViewOption::Installed:    index = 1; break;
        case ViewOption::NotInstalled: index = 2; break;
        case ViewOption::Outdated:     index = 3; break;
        default:                       index = 0; break;
    }
    m_selectedCategoryIndex = index;
    m_categoryList->value(index + 1);
}

void MainWindow::buildInfoPanel() {
    m_notebook = new MainTabs(0, 0, 10, 10);

    m_notebook->begin();

    Fl_Group *infoPage = new Fl_Group(0, 0, 10, 10, _("Info"));
    infoPage->begin();
    m_infoBuffer = new Fl_Text_Buffer();
    m_infoView = new Fl_Text_Display(0, 0, 10, 10);
    m_infoView->buffer(m_infoBuffer);
    m_infoView->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    infoPage->end();

    Fl_Group *filesPage = new Fl_Group(0, 0, 10, 10, _("Files"));
    filesPage->begin();
    m_filesBuffer = new Fl_Text_Buffer();
    m_filesView = new Fl_Text_Display(0, 0, 10, 10);
    m_filesView->buffer(m_filesBuffer);
    m_filesView->wrap_mode(Fl_Text_Display::WRAP_NONE, 0);
    filesPage->end();

    Fl_Group *sizePage = new Fl_Group(0, 0, 10, 10, _("Size"));
    sizePage->begin();
    m_sizeBuffer = new Fl_Text_Buffer();
    m_sizeView = new Fl_Text_Display(0, 0, 10, 10);
    m_sizeView->buffer(m_sizeBuffer);
    m_sizeView->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    sizePage->end();

    Fl_Group *transPage = new Fl_Group(0, 0, 10, 10, _("Transaction"));
    transPage->begin();
    m_transList = new Fl_Browser(0, 0, 10, 10);
    m_transList->column_char('\t');
    static int transWidths[] = {40, 0};
    m_transList->column_widths(transWidths);
    m_transList->format_char(0);
    transPage->end();

    m_notebook->end();
}

void MainWindow::buildStatusBar() {
    m_statusbar = new Fl_Box(0, 0, 10, 10, "");
    m_statusbar->box(FL_FLAT_BOX);
    m_statusbar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_statusbar->labelsize(12);
}

void MainWindow::buildProgressDialog() {
    m_progressDialog = new Fl_Window(420, 260, _(IRUKA_APP_NAME));
    m_progressDialog->set_modal();
    m_progressDialog->callback(cb_progress_close, this);

    m_progressDialog->begin();

    m_progressLabel = new Fl_Box(12, 12, 396, 22, "");
    m_progressLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    m_progressBar = new Fl_Progress(12, 40, 396, 22);
    m_progressBar->minimum(0.0f);
    m_progressBar->maximum(1.0f);
    m_progressBar->value(0.0f);

    m_progressToggle = new Fl_Button(12, 72, 120, 24, _("Show details"));
    m_progressToggle->callback(cb_progress_toggle, this);

    m_progressBuffer = new Fl_Text_Buffer();
    m_progressView = new Fl_Text_Display(12, 104, 396, 104);
    m_progressView->buffer(m_progressBuffer);
    m_progressView->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    m_progressView->hide();

    m_progressButton = new Fl_Button(320, 220, 88, 28, _("Cancel"));
    m_progressButton->callback(cb_progress_btn, this);

    m_progressDialog->end();
}

void MainWindow::layout() {
    if (!m_window) return;

    int W = m_window->w();
    int H = m_window->h();
    int margin = 8;
    int menuH = 28;
    int topH = 36;
    int statusH = 22;
    int catW = 170;
    int gap = 6;

    m_menuBar->resize(0, 0, W, menuH);

    int topY = menuH;
    int btnW = 84;
    int bx = W - margin - (btnW * 3 + gap * 2);
    if (bx < 220) bx = 220;

    int searchW = bx - margin - 8;
    if (searchW < 80) searchW = 80;

    m_searchEntry->resize(margin, topY + 4, searchW, 28);
    m_btnUpgrade->resize(bx, topY + 4, btnW, 28);
    m_btnRefresh->resize(bx + btnW + gap, topY + 4, btnW, 28);
    m_btnApply->resize(bx + 2 * (btnW + gap), topY + 4, btnW, 28);

    int contentY = topY + topH;
    int contentH = H - contentY - statusH - margin;
    if (contentH < 60) contentH = 60;

    m_categoryList->resize(margin, contentY, catW, contentH);

    int rightX = margin + catW + gap;
    int rightW = W - rightX - margin;
    if (rightW < 120) rightW = 120;

    int pkgH = m_detailsVisible ? static_cast<int>(contentH * 0.62) : contentH;
    m_pkgBrowser->resize(rightX, contentY, rightW, pkgH);

    if (m_detailsVisible) {
        int infoY = contentY + pkgH + 4;
        int infoH = contentH - pkgH - 4;
        if (infoH < 40) infoH = 40;
        m_notebook->resize(rightX, infoY, rightW, infoH);
        m_notebook->show();
    } else {
        m_notebook->hide();
    }

    m_pkgColWidths[0] = 34;
    m_pkgColWidths[1] = Settings::instance().nameColumnWidth();
    m_pkgColWidths[2] = Settings::instance().versionColumnWidth();
    m_pkgColWidths[3] = 0;
    m_pkgBrowser->column_widths(m_pkgColWidths);

    m_statusbar->resize(margin, H - statusH - margin / 2, W - 2 * margin, statusH);

    m_window->redraw();
}

void MainWindow::toggleDetails() {
    m_detailsVisible = !m_detailsVisible;

    Fl_Menu_Item *item = const_cast<Fl_Menu_Item *>(
        m_menuBar->find_item(_("Show details panel")));
    if (item) {
        if (m_detailsVisible) item->set();
        else item->clear();
    }

    layout();
}
