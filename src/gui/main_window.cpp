#include "main_window_private.h"
#include "settings.h"
#include "repository_editor.h"
#include "privileged_daemon.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Buffer.H>

MainWindow::MainWindow()
    : m_popupMenu(nullptr), m_model(nullptr)
{
    m_model = new PackageModel(m_repo);
    buildUi();
    loadPackageList();
}

MainWindow::~MainWindow() {
    m_destroyed = true;
    PrivilegedDaemon::instance().stop();

    if (m_repoEditor) {
        m_repoEditor->close();
        delete m_repoEditor;
        m_repoEditor = nullptr;
    }

    if (m_progressDialog) {
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    delete m_window;
    m_window = nullptr;

    delete m_infoBuffer;     m_infoBuffer = nullptr;
    delete m_filesBuffer;    m_filesBuffer = nullptr;
    delete m_sizeBuffer;     m_sizeBuffer = nullptr;
    delete m_progressBuffer; m_progressBuffer = nullptr;

    delete m_model;
    m_model = nullptr;
}

void MainWindow::show() {
    m_window->show();
    PrivilegedDaemon::instance().start();
}

void MainWindow::reloadUi() {
    if (m_rebuilding || m_transactionRunning || m_destroyed.load()) return;
    m_rebuilding = true;

    bool detailsVisible = m_detailsVisible;

    Fl::remove_timeout(cb_search_debounce, this);
    Fl::remove_timeout(cb_size_debounce, this);
    Fl::remove_timeout(cb_progress_pulse, this);

    ++m_infoGeneration;

    if (m_repoEditor) {
        m_repoEditor->close();
        delete m_repoEditor;
        m_repoEditor = nullptr;
    }

    if (m_progressDialog) {
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    delete m_window;
    m_window = nullptr;

    delete m_infoBuffer;     m_infoBuffer = nullptr;
    delete m_filesBuffer;    m_filesBuffer = nullptr;
    delete m_sizeBuffer;     m_sizeBuffer = nullptr;
    delete m_progressBuffer; m_progressBuffer = nullptr;

    delete m_model;
    m_model = new PackageModel(m_repo);

    m_detailsVisible = detailsVisible;

    buildUi();
    show();

    if (!detailsVisible) {
        m_detailsVisible = false;
        layout();
    }

    for (const auto &name : m_transactionInstall) {
        std::string line = "I\t" + name;
        m_transList->add(line.c_str());
    }
    for (const auto &name : m_transactionRemove) {
        std::string line = "X\t" + name;
        m_transList->add(line.c_str());
    }

    populateTable();
    refreshStatusBar();
    loadPackageList();

    m_rebuilding = false;
}
