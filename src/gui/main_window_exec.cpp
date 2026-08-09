// ----------------------------------------------------------------------------
// main_window_exec.cpp — Privileged command execution for MainWindow
//
// Contains:
//   - runCommandQueue()       starts sequential execution of the command queue
//   - startNextCommand()      pops and runs the next command from the queue
//   - finishRun()             called after the last command completes
//   - setTransactionRunning() toggles UI sensitivity and progress-bar timer
//   - onCancelProgress()      sends SIGTERM (then SIGKILL) to the child process
//   - execWithPrivileges()    forks pkexec and pipes its output to the progress view
//   - appendProgressOutput()  appends a text chunk to the progress text view
//   - applyExecResult()       main-thread callback that starts the next command
// ----------------------------------------------------------------------------

#include "main_window_private.h"
#include "constants.h"
#include "i18n.h"
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Starts executing the command queue sequentially, showing the progress dialog.
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

// Pops the next command from the queue and begins executing it.
void MainWindow::startNextCommand() {
    if (m_cancelRequested.load() || m_commandQueue.empty()) {
        finishRun();
        return;
    }

    ExecStep step = m_commandQueue.front();
    m_commandQueue.erase(m_commandQueue.begin());

    // Insert a section header into the progress log.
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

// Called when all queued commands have completed; reloads the package list.
void MainWindow::finishRun() {
    setTransactionRunning(false);
    gtk_button_set_label(GTK_BUTTON(m_progressButton), _("OK"));
    loadPackageList();
}

// Toggles UI sensitivity and the progress bar pulse timer.
void MainWindow::setTransactionRunning(bool running) {
    m_transactionRunning = running;
    gtk_widget_set_sensitive(m_btnApply,   !running);
    gtk_widget_set_sensitive(m_btnSync,    !running);
    gtk_widget_set_sensitive(m_btnUpgrade, !running);
    gtk_widget_set_sensitive(m_btnRefresh, !running);

    if (running) {
        // Start the pulse animation (fires every 120 ms).
        m_progressTimerId = g_timeout_add(120, cb_progress_pulse, this);
    } else {
        if (m_progressTimerId) {
            g_source_remove(m_progressTimerId);
            m_progressTimerId = 0;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), 1.0);
    }
}

// Signals the active child process group to terminate, first with SIGTERM then SIGKILL.
void MainWindow::onCancelProgress() {
    if (m_cancelRequested.load()) return;
    m_cancelRequested.store(true);

    int pid = m_activeChild.load();
    if (pid <= 0) return;

    // Send SIGTERM first to allow the process group to clean up gracefully.
    // SIGKILL is escalated after a delay via the privileged shell command below.
    kill(-pid, SIGTERM);

    // Use pkexec to also signal the privileged child process group.
    // The 'sleep 3' delay gives the process time to react to SIGTERM before SIGKILL.
    std::thread([pid]() {
        std::string cmd = std::string(PKEXEC_BIN) +
            " sh -c 'kill -TERM -" + std::to_string(pid) +
            " 2>/dev/null; sleep 3; kill -KILL -" +
            std::to_string(pid) + " 2>/dev/null'";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (pipe) pclose(pipe);
    }).detach();
}

// Forks a pkexec child, reading its output in a background thread and forwarding
// each chunk to the progress view via g_idle_add.
void MainWindow::execWithPrivileges(const std::vector<std::string> &args) {
    auto *result  = new ExecResult();
    result->win = this;

    std::thread([result, args]() {
        MainWindow *win = result->win;

        // Build the argv array for execvp: pkexec + user-supplied args + nullptr sentinel.
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(PKEXEC_BIN));
        for (auto &a : args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        // Create a pipe to capture the child's stdout and stderr.
        int pipefd[2];
        if (pipe(pipefd) != 0) {
            auto *chunk  = new ProgressChunk();
            chunk->win   = win;
            chunk->text  = _("Error: pipe() failed\n");
            g_idle_add(progress_chunk_cb, chunk);
            g_idle_add(exec_done_cb, result);
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process: create a new process group, redirect I/O, exec pkexec.
            setpgid(0, 0);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(PKEXEC_BIN, argv.data());
            _exit(127); // execvp failed
        }

        // Parent: close the write end and register the child PID for cancellation.
        close(pipefd[1]);
        win->m_activeChild.store(static_cast<int>(pid));

        // Read output in chunks and forward each chunk to the main thread.
        char  buf[4096];
        FILE *f = fdopen(pipefd[0], "r");
        if (f) {
            ssize_t n;
            bool killed = false;

            // Lambda to kill the process group; guards against double-kill.
            auto killProc = [&]() {
                if (!killed) {
                    kill(-pid, SIGTERM);
                    kill(-pid, SIGKILL);
                    killed = true;
                }
            };

            // Check for cancellation before the first read and after each chunk.
            if (win->m_cancelRequested.load()) killProc();
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
                auto *chunk = new ProgressChunk();
                chunk->win  = win;
                chunk->text.assign(buf, n);
                g_idle_add(progress_chunk_cb, chunk);
                if (win->m_cancelRequested.load()) killProc();
            }
            fclose(f);
        }

        int status = 0;
        waitpid(pid, &status, 0);
        win->m_activeChild.store(0);

        g_idle_add(exec_done_cb, result);
    }).detach();
}

// Appends text to the progress text view and auto-scrolls to the end.
void MainWindow::appendProgressOutput(const std::string &text) {
    if (!m_progressBuffer) return;

    GtkTextIter endIter;
    gtk_text_buffer_get_end_iter(m_progressBuffer, &endIter);
    gtk_text_buffer_insert(m_progressBuffer, &endIter, text.c_str(), text.size());

    // Scroll to the newly inserted text.
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(m_progressBuffer, &end);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(m_progressView), &end,
                                 0.0, FALSE, 0.0, 0.0);
}

// Called on the main thread when a command finishes; starts the next command in the queue.
void MainWindow::applyExecResult() {
    startNextCommand();
}
