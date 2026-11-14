#include "main_window_private.h"
#include "constants.h"
#include "i18n.h"
#include "privileged_daemon.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <cstdio>
#include <cstdlib>

void MainWindow::runCommandQueue() {
    if (m_commandQueue.empty() || m_transactionRunning) return;

    m_cancelRequested.store(false);
    m_activeChild.store(0);
    m_progressButton->label(_("Cancel"));

    setTransactionRunning(true);

    m_progressBuffer->text("");

    // Centre the progress window over the main window, clamped to the
    // visible work area, and make sure it is mapped and raised before the
    // privileged command starts.
    if (m_window) {
        int px = m_window->x() + (m_window->w() - m_progressDialog->w()) / 2;
        int py = m_window->y() + (m_window->h() - m_progressDialog->h()) / 2;

        int sx = 0, sy = 0, sw = 0, sh = 0;
        Fl::screen_work_area(sx, sy, sw, sh,
                             px + m_progressDialog->w() / 2,
                             py + m_progressDialog->h() / 2);
        if (sw > m_progressDialog->w()) {
            if (px < sx) px = sx;
            if (px + m_progressDialog->w() > sx + sw)
                px = sx + sw - m_progressDialog->w();
        } else {
            px = sx;
        }
        if (sh > m_progressDialog->h()) {
            if (py < sy) py = sy;
            if (py + m_progressDialog->h() > sy + sh)
                py = sy + sh - m_progressDialog->h();
        } else {
            py = sy;
        }
        m_progressDialog->position(px, py);
    }
    m_progressDialog->show();
    m_progressDialog->take_focus();

    startNextCommand();
}

void MainWindow::startNextCommand() {
    if (m_cancelRequested.load() || m_commandQueue.empty()) {
        finishRun();
        return;
    }

    ExecStep step = m_commandQueue.front();
    m_commandQueue.erase(m_commandQueue.begin());

    char headerBuf[512];
    std::snprintf(headerBuf, sizeof(headerBuf), _("=== %s ===\n\n"), step.label.c_str());
    m_progressBuffer->append(headerBuf);
    m_progressView->insert_position(m_progressBuffer->length());
    m_progressView->show_insert_position();

    m_progressLabel->copy_label(step.label.c_str());
    m_progressBar->value(0.0f);
    m_progressPulse = 0.0;

    execWithPrivileges(step.args);
}

void MainWindow::finishRun() {
    setTransactionRunning(false);
    m_progressButton->label(_("OK"));
    loadPackageList();
}

void MainWindow::setTransactionRunning(bool running) {
    m_transactionRunning = running;
    if (running) {
        m_btnApply->deactivate();
        m_btnUpgrade->deactivate();
        m_btnRefresh->deactivate();
        m_progressPulse = 0.0;
        Fl::remove_timeout(cb_progress_pulse, this);
        Fl::add_timeout(0.12, cb_progress_pulse, this);
    } else {
        m_btnApply->activate();
        m_btnUpgrade->activate();
        m_btnRefresh->activate();
        Fl::remove_timeout(cb_progress_pulse, this);
        m_progressBar->value(1.0f);
    }
}

void MainWindow::onCancelProgress() {
    if (m_cancelRequested.load()) return;
    m_cancelRequested.store(true);
    PrivilegedDaemon::instance().cancel();
}

void MainWindow::onProgressToggle() {
    m_progressDetailsVisible = !m_progressDetailsVisible;
    if (m_progressDetailsVisible) {
        m_progressView->show();
        m_progressToggle->label(_("Hide details"));
    } else {
        m_progressView->hide();
        m_progressToggle->label(_("Show details"));
    }
    m_progressDialog->redraw();
}

static std::string shellQuote(const std::string &arg) {
    std::string out = "'";
    for (char c : arg) {
        if (c == '\'') out += "'\\''";
        else out.push_back(c);
    }
    out += "'";
    return out;
}

void MainWindow::execWithPrivileges(const std::vector<std::string> &args) {
    std::string cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) cmd += ' ';
        cmd += shellQuote(args[i]);
    }

    auto *result = new ExecResult();
    result->win = this;

    std::thread([result, cmd]() {
        MainWindow *win = result->win;

        auto sink = [win](const std::string &text) {
            auto *chunk = new ProgressChunk();
            chunk->win = win;
            chunk->text = text;
            Fl::awake(progress_chunk_awake, chunk);
        };

        int code = PrivilegedDaemon::instance().run(cmd, sink);
        if (code == -1) {
            sink(_("Error: privileged helper is not available.\n"));
        }

        Fl::awake(exec_done_awake, result);
    }).detach();
}

void MainWindow::appendProgressOutput(const std::string &text) {
    if (!m_progressBuffer || !m_progressView) return;

    m_progressBuffer->append(text.c_str());
    m_progressView->insert_position(m_progressBuffer->length());
    m_progressView->show_insert_position();
}

void MainWindow::applyExecResult() {
    startNextCommand();
}
