
#include "main_window_private.h"
#include "settings.h"
#include "constants.h"
#include "category.h"
#include "i18n.h"


void MainWindow::buildUi() {
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_window), _(IRUKA_APP_TITLE));
    gtk_window_set_default_size(GTK_WINDOW(m_window),
        Settings::instance().windowWidth(),
        Settings::instance().windowHeight());
    g_signal_connect(m_window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(m_window), vbox);

    buildMenuBar();
    buildToolbar();
    buildSearchBar();
    buildPackageTable();
    buildCategoryPanel();
    buildInfoPanel();
    buildStatusBar();
    buildProgressDialog();

    gtk_widget_set_margin_start(m_menuBar, 8);
    gtk_widget_set_margin_end(m_menuBar, 8);
    gtk_box_pack_start(GTK_BOX(vbox), m_menuBar, FALSE, FALSE, 0);

    GtkWidget *topRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(topRow, 8);
    gtk_widget_set_margin_end(topRow, 8);
    gtk_box_pack_start(GTK_BOX(vbox), topRow, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(topRow), m_searchBar, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(topRow), m_toolbar, FALSE, FALSE, 0);

    m_mainPaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    GtkWidget *listScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(listScroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(listScroll), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(listScroll), m_treeView);

    gtk_paned_pack1(GTK_PANED(m_mainPaned), listScroll, TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(m_mainPaned), m_notebook, TRUE, TRUE);

    m_outerPaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_start(m_outerPaned, 8);
    gtk_widget_set_margin_end(m_outerPaned, 8);
    gtk_widget_set_margin_bottom(m_outerPaned, 4);
    gtk_paned_pack1(GTK_PANED(m_outerPaned), m_categoryScroll, FALSE, FALSE);
    gtk_paned_pack2(GTK_PANED(m_outerPaned), m_mainPaned, TRUE, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), m_outerPaned, TRUE, TRUE, 0);

    gtk_widget_set_margin_start(m_statusbar, 8);
    gtk_widget_set_margin_end(m_statusbar, 8);
    gtk_widget_set_margin_top(m_statusbar, 2);
    gtk_widget_set_margin_bottom(m_statusbar, 4);
    gtk_box_pack_start(GTK_BOX(vbox), m_statusbar, FALSE, FALSE, 0);
}


void MainWindow::buildMenuBar() {
    m_menuBar = gtk_menu_bar_new();

    GtkWidget *viewMenu = gtk_menu_new();
    GtkWidget *viewItem = gtk_menu_item_new_with_label(_("View"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewItem), viewMenu);

    GtkWidget *allItem = gtk_menu_item_new_with_label(_("All packages"));
    g_signal_connect(allItem, "activate", G_CALLBACK(cb_menu_all), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), allItem);

    GtkWidget *instItem = gtk_menu_item_new_with_label(_("Installed only"));
    g_signal_connect(instItem, "activate", G_CALLBACK(cb_menu_installed), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), instItem);

    GtkWidget *notInstItem = gtk_menu_item_new_with_label(_("Not installed"));
    g_signal_connect(notInstItem, "activate", G_CALLBACK(cb_menu_notinstalled), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), notInstItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), gtk_separator_menu_item_new());

    m_viewDetailsItem = gtk_check_menu_item_new_with_label(_("Show details panel"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_viewDetailsItem), TRUE);
    g_signal_connect(m_viewDetailsItem, "toggled", G_CALLBACK(cb_toggle_details), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), m_viewDetailsItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(GTK_MENU_BAR(m_menuBar)), viewItem);

    GtkWidget *txMenu = gtk_menu_new();
    GtkWidget *txItem = gtk_menu_item_new_with_label(_("Packages"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(txItem), txMenu);

    GtkWidget *markInstallItem = gtk_menu_item_new_with_label(_("Mark for install"));
    g_signal_connect(markInstallItem, "activate", G_CALLBACK(cb_popup_install), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), markInstallItem);

    GtkWidget *markUninstallItem = gtk_menu_item_new_with_label(_("Mark for uninstall"));
    g_signal_connect(markUninstallItem, "activate", G_CALLBACK(cb_popup_remove), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), markUninstallItem);

    GtkWidget *unmarkItem = gtk_menu_item_new_with_label(_("Unmark"));
    g_signal_connect(unmarkItem, "activate", G_CALLBACK(cb_popup_unmark), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), unmarkItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), gtk_separator_menu_item_new());

    GtkWidget *commitItem = gtk_menu_item_new_with_label(_("Apply"));
    g_signal_connect(commitItem, "activate", G_CALLBACK(cb_menu_commit), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), commitItem);

    GtkWidget *clearItem = gtk_menu_item_new_with_label(_("Clear actions"));
    g_signal_connect(clearItem, "activate", G_CALLBACK(cb_menu_clear), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), clearItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), gtk_separator_menu_item_new());

    GtkWidget *syncItem = gtk_menu_item_new_with_label(_("Sync databases"));
    g_signal_connect(syncItem, "activate", G_CALLBACK(cb_menu_sync), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), syncItem);

    GtkWidget *upItem = gtk_menu_item_new_with_label(_("System upgrade"));
    g_signal_connect(upItem, "activate", G_CALLBACK(cb_menu_upgrade), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(txMenu), upItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(GTK_MENU_BAR(m_menuBar)), txItem);

    GtkWidget *optMenu = gtk_menu_new();
    GtkWidget *optItem = gtk_menu_item_new_with_label(_("Options"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(optItem), optMenu);

    GtkWidget *reposItem = gtk_menu_item_new_with_label(_("Repositories"));
    g_signal_connect(reposItem, "activate", G_CALLBACK(cb_menu_repositories), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(optMenu), reposItem);

    GtkWidget *langMenu = gtk_menu_new();
    GtkWidget *langItem = gtk_menu_item_new_with_label(_("Language"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(langItem), langMenu);

    GtkWidget *langSystem = gtk_radio_menu_item_new_with_label(nullptr, _("System (default)"));
    g_object_set_data_full(G_OBJECT(langSystem), "lang", g_strdup(""), g_free);
    g_signal_connect(langSystem, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langSystem);

    GtkWidget *langEn = gtk_radio_menu_item_new_with_label(
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(langSystem)), _("English"));
    g_object_set_data_full(G_OBJECT(langEn), "lang", g_strdup("en"), g_free);
    g_signal_connect(langEn, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langEn);

    GtkWidget *langEs = gtk_radio_menu_item_new_with_label(
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(langSystem)), _("Español"));
    g_object_set_data_full(G_OBJECT(langEs), "lang", g_strdup("es"), g_free);
    g_signal_connect(langEs, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langEs);

    GtkWidget *langRu = gtk_radio_menu_item_new_with_label(
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(langSystem)), _("Русский"));
    g_object_set_data_full(G_OBJECT(langRu), "lang", g_strdup("ru"), g_free);
    g_signal_connect(langRu, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langRu);

    GtkWidget *langJa = gtk_radio_menu_item_new_with_label(
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(langSystem)), _("日本語"));
    g_object_set_data_full(G_OBJECT(langJa), "lang", g_strdup("ja"), g_free);
    g_signal_connect(langJa, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langJa);
   
    GtkWidget *langHu = gtk_radio_menu_item_new_with_label(
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(langSystem)), _("Magyar"));
    g_object_set_data_full(G_OBJECT(langHu), "lang", g_strdup("hu"), g_free);
    g_signal_connect(langHu, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langHu);
    
    GtkWidget *langIt = gtk_radio_menu_item_new_with_label(
        gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(langSystem)), _("Italiano"));
    g_object_set_data_full(G_OBJECT(langIt), "lang", g_strdup("it"), g_free);
    g_signal_connect(langIt, "activate", G_CALLBACK(cb_menu_language), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(langMenu), langIt);

    const std::string &uiLang = Settings::instance().uiLanguage();
    g_signal_handlers_block_by_func(langSystem, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_block_by_func(langEn, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_block_by_func(langEs, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_block_by_func(langRu, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_block_by_func(langJa, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_block_by_func(langHu, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_block_by_func(langIt, (gpointer)G_CALLBACK(cb_menu_language), this);

    if (uiLang == "en") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langEn), TRUE);
    } else if (uiLang == "es") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langEs), TRUE);
    } else if (uiLang == "ru") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langRu), TRUE);
    } else if (uiLang == "ja") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langJa), TRUE);
    } else if (uiLang == "hu") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langHu), TRUE);
    } else if (uiLang == "it") {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langIt), TRUE);
    } else {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(langSystem), TRUE);
    }

    g_signal_handlers_unblock_by_func(langSystem, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_unblock_by_func(langEn, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_unblock_by_func(langEs, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_unblock_by_func(langRu, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_unblock_by_func(langJa, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_unblock_by_func(langHu, (gpointer)G_CALLBACK(cb_menu_language), this);
    g_signal_handlers_unblock_by_func(langIt, (gpointer)G_CALLBACK(cb_menu_language), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(optMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(optMenu), langItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(GTK_MENU_BAR(m_menuBar)), optItem);

    GtkWidget *helpMenu = gtk_menu_new();
    GtkWidget *helpItem = gtk_menu_item_new_with_label(_("Help"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpItem), helpMenu);

    GtkWidget *aboutItem = gtk_menu_item_new_with_label(_("About"));
    g_signal_connect(aboutItem, "activate", G_CALLBACK(cb_menu_about), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(GTK_MENU_BAR(m_menuBar)), helpItem);
}


void MainWindow::buildToolbar() {
    m_toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(m_toolbar, 4);
    gtk_widget_set_margin_bottom(m_toolbar, 4);
    gtk_widget_set_margin_start(m_toolbar, 8);

    auto makeButton = [](const char *icon, const char *label,
                         GCallback cb, gpointer data) -> GtkWidget* {
        GtkWidget *btn = gtk_button_new();
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget *img = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_BUTTON);
        GtkWidget *lbl = gtk_label_new(label);
        gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(btn), box);
        g_signal_connect(btn, "clicked", cb, data);
        return btn;
    };

    m_btnUpgrade = makeButton("software-update-available-symbolic", _("Update"),
                              G_CALLBACK(cb_btn_upgrade), this);
    gtk_widget_set_tooltip_text(m_btnUpgrade, _("Upgrade all system packages"));

    m_btnRefresh = makeButton("view-refresh-symbolic", _("Refresh"),
                              G_CALLBACK(cb_btn_refresh), this);
    gtk_widget_set_tooltip_text(m_btnRefresh, _("Refresh package list"));

    m_btnApply = makeButton("object-select-symbolic", _("Apply"),
                            G_CALLBACK(cb_btn_apply), this);
    gtk_widget_set_tooltip_text(m_btnApply, _("Apply pending transaction"));

    gtk_box_pack_start(GTK_BOX(m_toolbar), m_btnUpgrade, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(m_toolbar), m_btnRefresh, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(m_toolbar), m_btnApply,   FALSE, FALSE, 4);
}


void MainWindow::buildSearchBar() {
    m_searchBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_top(m_searchBar, 4);
    gtk_widget_set_margin_bottom(m_searchBar, 4);

    m_searchEntry = gtk_search_entry_new();
    g_signal_connect(m_searchEntry, "search-changed", G_CALLBACK(cb_search_changed), this);

    gtk_box_pack_start(GTK_BOX(m_searchBar), m_searchEntry, TRUE, TRUE, 5);
}


void MainWindow::buildPackageTable() {
    m_listStore = gtk_list_store_new(COL_COUNT,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_POINTER
    );

    m_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_listStore));
    g_object_unref(m_listStore);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *colStatus = gtk_tree_view_column_new_with_attributes(
        "S", renderer, "text", COL_STATUS, nullptr);
    gtk_tree_view_column_set_fixed_width(colStatus, 30);
    gtk_tree_view_column_set_sizing(colStatus, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), colStatus);

    GtkTreeViewColumn *colName = gtk_tree_view_column_new_with_attributes(
        _("Name"), renderer, "text", COL_NAME, nullptr);
    gtk_tree_view_column_set_min_width(colName, Settings::instance().nameColumnWidth());
    gtk_tree_view_column_set_resizable(colName, TRUE);
    g_signal_connect(colName, "notify::width", G_CALLBACK(cb_col_width_changed), this);
    m_colName = colName;
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), colName);

    GtkTreeViewColumn *colVersion = gtk_tree_view_column_new_with_attributes(
        _("Version"), renderer, "text", COL_VERSION, nullptr);
    gtk_tree_view_column_set_min_width(colVersion, Settings::instance().versionColumnWidth());
    gtk_tree_view_column_set_resizable(colVersion, TRUE);
    g_signal_connect(colVersion, "notify::width", G_CALLBACK(cb_col_width_changed), this);
    m_colVersion = colVersion;
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), colVersion);

    GtkTreeViewColumn *colDesc = gtk_tree_view_column_new_with_attributes(
        _("Description"), renderer, "text", COL_DESCRIPTION, nullptr);
    gtk_tree_view_column_set_resizable(colDesc, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_treeView), colDesc);

    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(m_treeView), TRUE);

    m_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_treeView));
    gtk_tree_selection_set_mode(m_selection, GTK_SELECTION_SINGLE);
    g_signal_connect(m_selection, "changed",           G_CALLBACK(cb_table_select),      this);
    g_signal_connect(m_treeView,  "row-activated",     G_CALLBACK(cb_row_activated),     this);
    g_signal_connect(m_treeView,  "button-press-event",G_CALLBACK(cb_tree_button_press), this);

    buildPopupMenu();
}

void MainWindow::buildCategoryPanel() {
    m_categoryList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(m_categoryList), GTK_SELECTION_SINGLE);
    g_signal_connect(m_categoryList, "row-selected",
                     G_CALLBACK(cb_category_selected), this);

    addCategoryRow(_("All packages"), 0, "");
    addCategoryRow(_("Installed"), 1, "");
    addCategoryRow(_("Not installed"), 2, "");
    addCategoryRow(_("Outdated"), 3, "");

    GtkListBoxRow *catHeader = addCategoryRow(_("Categories"), -1, "");
    gtk_list_box_row_set_selectable(catHeader, FALSE);
    gtk_list_box_row_set_activatable(catHeader, FALSE);

    for (const auto &cat : category::all())
        addCategoryRow(_(cat.c_str()), 5, cat);

    m_categoryScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(m_categoryScroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(m_categoryScroll),
        GTK_SHADOW_NONE);
    gtk_widget_set_size_request(m_categoryScroll, 170, -1);
    gtk_container_add(GTK_CONTAINER(m_categoryScroll), m_categoryList);

    int index = 0;
    switch (Settings::instance().viewOption()) {
        case ViewOption::Installed:    index = 1; break;
        case ViewOption::NotInstalled: index = 2; break;
        case ViewOption::Outdated:     index = 3; break;
        default:                       index = 0; break;
    }
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_categoryList), index);
    if (row) {
        g_signal_handlers_block_by_func(m_categoryList,
            (gpointer)G_CALLBACK(cb_category_selected), this);
        gtk_list_box_select_row(GTK_LIST_BOX(m_categoryList), row);
        g_signal_handlers_unblock_by_func(m_categoryList,
            (gpointer)G_CALLBACK(cb_category_selected), this);
    }
}

GtkListBoxRow *MainWindow::addCategoryRow(const std::string &label, int kind,
                                          const std::string &repo) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *lbl = gtk_label_new(label.c_str());
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);
    gtk_widget_set_margin_start(lbl, 10);
    gtk_widget_set_margin_end(lbl, 10);
    gtk_widget_set_margin_top(lbl, 5);
    gtk_widget_set_margin_bottom(lbl, 5);
    if (kind == -1) {
        GtkStyleContext *sc = gtk_widget_get_style_context(lbl);
        gtk_style_context_add_class(sc, "dim-label");
    }
    gtk_container_add(GTK_CONTAINER(row), lbl);

    g_object_set_data(G_OBJECT(row), "kind", GINT_TO_POINTER(kind));
    if (!repo.empty())
        g_object_set_data_full(G_OBJECT(row), "value", g_strdup(repo.c_str()), g_free);

    gtk_list_box_insert(GTK_LIST_BOX(m_categoryList), row, -1);
    return GTK_LIST_BOX_ROW(row);
}

void MainWindow::buildPopupMenu() {
    m_popupMenu = gtk_menu_new();

    GtkWidget *installItem = gtk_menu_item_new_with_label(_("Install"));
    g_signal_connect(installItem, "activate", G_CALLBACK(cb_popup_install), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(m_popupMenu), installItem);

    GtkWidget *removeItem = gtk_menu_item_new_with_label(_("Remove"));
    g_signal_connect(removeItem, "activate", G_CALLBACK(cb_popup_remove), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(m_popupMenu), removeItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(m_popupMenu), gtk_separator_menu_item_new());

    GtkWidget *unmarkItem = gtk_menu_item_new_with_label(_("Unmark"));
    g_signal_connect(unmarkItem, "activate", G_CALLBACK(cb_popup_unmark), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(m_popupMenu), unmarkItem);

    gtk_widget_show_all(m_popupMenu);
}


void MainWindow::buildInfoPanel() {
    m_notebook = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(m_notebook), FALSE);

    m_infoBuffer = gtk_text_buffer_new(nullptr);
    m_infoView   = gtk_text_view_new_with_buffer(m_infoBuffer);
    g_object_unref(m_infoBuffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(m_infoView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(m_infoView), GTK_WRAP_WORD);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(m_infoView), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(m_infoView), 8);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(m_infoView), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(m_infoView), 10);
    GtkWidget *infoScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(infoScroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(infoScroll), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(infoScroll), m_infoView);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), infoScroll, gtk_label_new(_("Info")));

    m_filesBuffer = gtk_text_buffer_new(nullptr);
    m_filesView   = gtk_text_view_new_with_buffer(m_filesBuffer);
    g_object_unref(m_filesBuffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(m_filesView), FALSE);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(m_filesView), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(m_filesView), 8);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(m_filesView), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(m_filesView), 10);
    GtkWidget *filesScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(filesScroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(filesScroll), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(filesScroll), m_filesView);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), filesScroll, gtk_label_new(_("Files")));

    m_sizeBuffer = gtk_text_buffer_new(nullptr);
    m_sizeView   = gtk_text_view_new_with_buffer(m_sizeBuffer);
    g_object_unref(m_sizeBuffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(m_sizeView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(m_sizeView), GTK_WRAP_WORD);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(m_sizeView), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(m_sizeView), 8);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(m_sizeView), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(m_sizeView), 10);
    GtkWidget *sizeScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sizeScroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sizeScroll), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(sizeScroll), m_sizeView);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), sizeScroll, gtk_label_new(_("Size")));

    m_transStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    m_transTree  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_transStore));
    g_object_unref(m_transStore);
    GtkCellRenderer *transRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(m_transTree),
        -1, _("Op"), transRenderer, "text", 0, nullptr);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(m_transTree),
        -1, _("Package"), transRenderer, "text", 1, nullptr);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(m_transTree), TRUE);
    gtk_widget_set_margin_start(m_transTree, 8);
    gtk_widget_set_margin_end(m_transTree, 8);
    gtk_widget_set_margin_top(m_transTree, 6);
    gtk_widget_set_margin_bottom(m_transTree, 6);
    GtkWidget *transScroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(transScroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(transScroll), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(transScroll), m_transTree);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), transScroll,
        gtk_label_new(_("Transaction")));
}


void MainWindow::buildStatusBar() {
    m_statusbar      = gtk_statusbar_new();
    m_statusContextId = gtk_statusbar_get_context_id(GTK_STATUSBAR(m_statusbar), "iruka");
}


void MainWindow::buildProgressDialog() {
    m_progressDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(m_progressDialog), _(IRUKA_APP_NAME));
    gtk_window_set_transient_for(GTK_WINDOW(m_progressDialog), GTK_WINDOW(m_window));
    gtk_window_set_modal(GTK_WINDOW(m_progressDialog), TRUE);
    gtk_window_set_position(GTK_WINDOW(m_progressDialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_default_size(GTK_WINDOW(m_progressDialog), 420, 220);
    g_signal_connect(m_progressDialog, "delete-event",
        G_CALLBACK(cb_progress_delete), this);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_container_add(GTK_CONTAINER(m_progressDialog), vbox);

    m_progressLabel = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(m_progressLabel), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), m_progressLabel, FALSE, FALSE, 0);

    m_progressBar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), m_progressBar, FALSE, FALSE, 0);

    m_progressExpander = gtk_expander_new(_("Show details"));
    gtk_box_pack_start(GTK_BOX(vbox), m_progressExpander, TRUE, TRUE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll, 380, 160);

    m_progressBuffer = gtk_text_buffer_new(nullptr);
    m_progressView   = gtk_text_view_new_with_buffer(m_progressBuffer);
    g_object_unref(m_progressBuffer);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(m_progressView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(m_progressView), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(m_progressView), FALSE);

    gtk_container_add(GTK_CONTAINER(scroll), m_progressView);
    gtk_container_add(GTK_CONTAINER(m_progressExpander), scroll);

    GtkWidget *btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(btnBox, GTK_ALIGN_END);
    m_progressButton = gtk_button_new_with_label(_("Cancel"));
    g_signal_connect(m_progressButton, "clicked", G_CALLBACK(cb_progress_btn), this);
    gtk_box_pack_start(GTK_BOX(btnBox), m_progressButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), btnBox, FALSE, FALSE, 0);
}


void MainWindow::toggleDetails() {
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(m_viewDetailsItem))) {
        gtk_widget_show(m_notebook);
        gtk_paned_set_position(GTK_PANED(m_mainPaned), m_lastPanedPosition);
    } else {
        m_lastPanedPosition = gtk_paned_get_position(GTK_PANED(m_mainPaned));
        gtk_widget_hide(m_notebook);
    }
}
