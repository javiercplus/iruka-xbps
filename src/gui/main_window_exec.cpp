
#include "main_window_private.h"
#include "constants.h"
#include "i18n.h"
#include "privileged_daemon.h"
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

static bool confirmPkexecPrivileges(GtkWindow *parent) {
    GtkWidget *dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
        GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
        "%s", _("This operation requires pkexec permissions.\n\n"
                "Press OK to authenticate. The privileged helper will be restarted."));
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("OK"), GTK_RESPONSE_ACCEPT);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response != GTK_RESPONSE_ACCEPT) return false;

    PrivilegedDaemon::instance().stop();
    PrivilegedDaemon::instance().start();
    return true;
}

void MainWindow::runCommandQueue() {
    if (m_commandQueue.empty() || m_transactionRunning) return;

    m_cancelRequested.store(false);
    m_activeChild.store(0);
    gtk_button_set_label(GTK_BUTTON(m_progressButton), _("Cancel"));

    setTransactionRunning(true);

    gtk_text_buffer_set_text(m_progressBuffer, "", -1);
    gtk_widget_show_all(m_progressDialog);

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
    std::string header = headerBuf;
    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(m_progressBuffer, &endIter);
    gtk_text_buffer_insert(m_progressBuffer, &endIter, header.c_str(), -1);

    gtk_label_set_text(GTK_LABEL(m_progressLabel), step.label.c_str());
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), 0.0);

    execWithPrivileges(step.args);
}

void MainWindow::finishRun() {
    setTransactionRunning(false);
    gtk_button_set_label(GTK_BUTTON(m_progressButton), _("OK"));
    loadPackageList();
}

void MainWindow::setTransactionRunning(bool running) {
    m_transactionRunning = running;
    gtk_widget_set_sensitive(m_btnApply,   !running);
    gtk_widget_set_sensitive(m_btnUpgrade, !running);
    gtk_widget_set_sensitive(m_btnRefresh, !running);

    if (running) {
        m_progressTimerId = g_timeout_add(120, cb_progress_pulse, this);
    } else {
        if (m_progressTimerId) {
            g_source_remove(m_progressTimerId);
            m_progressTimerId = 0;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), 1.0);
    }
}

void MainWindow::onCancelProgress() {
    if (m_cancelRequested.load()) return;
    m_cancelRequested.store(true);
    PrivilegedDaemon::instance().cancel();
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

void MainWindow::execWithPrivileges(const std::vector<std::string> &args,
                                    bool retried) {
    std::string cmd;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i) cmd += ' ';
        cmd += shellQuote(args[i]);
    }

    auto *result = new ExecResult();
    result->win = this;
    result->args = args;
    result->retried = retried;

    std::thread([result, cmd]() {
        MainWindow *win = result->win;

        auto sink = [win](const std::string &text) {
            auto *chunk = new ProgressChunk();
            chunk->win  = win;
            chunk->text = text;
            g_idle_add(progress_chunk_cb, chunk);
        };

        int code = PrivilegedDaemon::instance().run(cmd, sink);
        if (code == -1) {
            result->authFailed = true;
        }

        g_idle_add(exec_done_cb, result);
    }).detach();
}

void MainWindow::appendProgressOutput(const std::string &text) {
    if (!m_progressBuffer) return;

    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(m_progressBuffer, &endIter);
    gtk_text_buffer_insert(m_progressBuffer, &endIter, text.c_str(), text.size());

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(m_progressBuffer, &end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(m_progressView), &end,
                                 0.0, FALSE, 0.0, 0.0);
}

void MainWindow::applyExecResult(const std::vector<std::string> &args,
                                 bool authFailed, bool retried) {
    if (authFailed) {
        // The helper was not available (pkexec cancelled or failed). Ask the
        // user once to authenticate again and retry the same command.
        if (!retried && confirmPkexecPrivileges(GTK_WINDOW(m_window))) {
            execWithPrivileges(args, true);
            return;
        }

        m_commandQueue.clear();
        appendProgressOutput(_("Error: privileged helper is not available.\n"));
        finishRun();
        return;
    }

    startNextCommand();
}
