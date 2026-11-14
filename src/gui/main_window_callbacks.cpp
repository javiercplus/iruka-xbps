#include "main_window_private.h"
#include "settings.h"
#include "repository_editor.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Buffer.H>
#include <cstring>

void load_done_awake(void *data) {
    auto *r = static_cast<LoadResult *>(data);
    if (!r->win->isDestroyed()) {
        r->win->applyLoadedPackages(std::move(r->merged), std::move(r->unreqSet),
                                    std::move(r->outdated));
    }
    delete r;
}

void info_done_awake(void *data) {
    auto *r = static_cast<InfoResult *>(data);
    if (!r->win->isDestroyed()) {
        r->win->applyPackageInfo(r->generation, r->infoText, r->filesText, r->sizeText);
    }
    delete r;
}

void exec_done_awake(void *data) {
    auto *r = static_cast<ExecResult *>(data);
    if (!r->win->isDestroyed()) {
        r->win->applyExecResult();
    }
    delete r;
}

void progress_chunk_awake(void *data) {
    auto *r = static_cast<ProgressChunk *>(data);
    if (!r->win->isDestroyed()) {
        r->win->appendProgressOutput(r->text);
    }
    delete r;
}

void reload_ui_awake(void *data) {
    auto *r = static_cast<ReloadData *>(data);
    if (!r->win->isDestroyed()) {
        r->win->reloadUi();
    }
    delete r;
}

void MainWindow::openContextMenu() {
    if (m_popupMenu) m_popupMenu->popup();
}

int PackageBrowser::handle(int event) {
    if (event == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE) {
        void *it = find_item(Fl::event_y());
        if (it) {
            select(lineno(it));
            do_callback();
        }
        if (owner) {
            owner->openContextMenu();
            return 1;
        }
    }
    return Fl_Hold_Browser::handle(event);
}

void MainWindow::cb_menu_all(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setView(ViewOption::All);
}
void MainWindow::cb_menu_installed(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setView(ViewOption::Installed);
}
void MainWindow::cb_menu_notinstalled(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setView(ViewOption::NotInstalled);
}

void MainWindow::cb_menu_sync(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_sync();
}
void MainWindow::cb_menu_upgrade(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_upgrade();
}
void MainWindow::cb_menu_commit(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->commitTransaction();
}
void MainWindow::cb_menu_clear(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->clearTransaction();
}

void MainWindow::cb_menu_about(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_about();
}
void MainWindow::cb_menu_repositories(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->openRepositories();
}

void MainWindow::cb_menu_language_system(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setLanguage("");
}
void MainWindow::cb_menu_language_en(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setLanguage("en");
}
void MainWindow::cb_menu_language_es(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setLanguage("es");
}
void MainWindow::cb_menu_language_ru(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setLanguage("ru");
}
void MainWindow::cb_menu_language_ja(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setLanguage("ja");
}
void MainWindow::cb_menu_language_hu(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->setLanguage("hu");
}

void MainWindow::cb_btn_upgrade(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_upgrade();
}
void MainWindow::cb_btn_refresh(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->loadPackageList();
}
void MainWindow::cb_btn_apply(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->commitTransaction();
}

void MainWindow::cb_popup_install(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_install();
}
void MainWindow::cb_popup_update(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_update_package();
}
void MainWindow::cb_popup_remove(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_remove();
}
void MainWindow::cb_popup_unmark(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->do_unmark();
}

void MainWindow::cb_category_selected(Fl_Widget *, void *d) {
    auto *win = static_cast<MainWindow *>(d);
    int line = win->m_categoryList->value();
    if (line < 1) return;

    int idx = line - 1;
    if (idx >= static_cast<int>(win->m_categoryRows.size())) return;

    CategoryRow row = win->m_categoryRows[idx];
    if (row.kind == -1) {
        win->m_categoryList->value(win->m_selectedCategoryIndex + 1);
        return;
    }

    win->m_selectedCategoryIndex = idx;
    win->applyCategory(row.kind, row.value);
}

void MainWindow::cb_search_changed(Fl_Widget *, void *d) {
    auto *win = static_cast<MainWindow *>(d);
    Fl::remove_timeout(cb_search_debounce, win);
    Fl::add_timeout(0.3, cb_search_debounce, win);
}

void MainWindow::cb_search_debounce(void *d) {
    static_cast<MainWindow *>(d)->onSearch();
}

void MainWindow::cb_size_debounce(void *d) {
    static_cast<MainWindow *>(d)->onSelectionDebounced();
}

void MainWindow::cb_table_select(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->onTableSelect();
}

void MainWindow::cb_progress_pulse(void *d) {
    auto *win = static_cast<MainWindow *>(d);
    if (!win->m_transactionRunning || win->isDestroyed()) return;
    win->m_progressPulse += 0.03;
    if (win->m_progressPulse > 0.9) win->m_progressPulse = 0.1;
    win->m_progressBar->value(static_cast<float>(win->m_progressPulse));
    Fl::repeat_timeout(0.12, cb_progress_pulse, d);
}

void MainWindow::cb_progress_btn(Fl_Widget *, void *d) {
    auto *win = static_cast<MainWindow *>(d);
    if (win->m_transactionRunning) {
        win->onCancelProgress();
    } else {
        win->m_progressDialog->hide();
    }
}

void MainWindow::cb_progress_toggle(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->onProgressToggle();
}

void MainWindow::cb_progress_close(Fl_Widget *, void *d) {
    auto *win = static_cast<MainWindow *>(d);
    if (win->m_transactionRunning) return;
    win->m_progressDialog->hide();
}

void MainWindow::cb_toggle_details(Fl_Widget *, void *d) {
    static_cast<MainWindow *>(d)->toggleDetails();
}

void MainWindow::cb_window_close(Fl_Widget *, void *d) {
    auto *win = static_cast<MainWindow *>(d);
    if (win->m_transactionRunning) return;

    Settings::instance().setWindowSize(win->m_window->w(), win->m_window->h());
    Settings::instance().save();

    win->m_destroyed = true;
    Fl::remove_timeout(cb_search_debounce, win);
    Fl::remove_timeout(cb_size_debounce, win);
    Fl::remove_timeout(cb_progress_pulse, win);

    if (win->m_progressDialog) win->m_progressDialog->hide();
    if (win->m_repoEditor) win->m_repoEditor->close();
    while (Fl::first_window()) Fl::first_window()->hide();
}
