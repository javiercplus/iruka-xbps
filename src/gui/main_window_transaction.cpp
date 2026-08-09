// ----------------------------------------------------------------------------
// main_window_transaction.cpp — Transaction management for MainWindow
//
// Contains:
//   - addToTransaction()   marks a package for install or removal
//   - setPackageStatus()   updates the status column in the list store
//   - commitTransaction()  confirms and runs the pending transaction
//   - clearTransaction()   resets all pending marks and the transaction tab
// ----------------------------------------------------------------------------

#include "main_window_private.h"
#include "constants.h"
#include "i18n.h"
#include <string>
#include <vector>

// Adds a package to the pending install or remove transaction list.
void MainWindow::addToTransaction(const std::string &pkgName, bool install) {
    if (install) {
        m_transactionInstall.push_back(pkgName);
        m_installMarks.insert(pkgName);
    } else {
        m_transactionRemove.push_back(pkgName);
        m_removeMarks.insert(pkgName);
    }

    // Append the operation to the Transaction notebook tab.
    GtkTreeIter iter;
    gtk_list_store_append(m_transStore, &iter);
    gtk_list_store_set(m_transStore, &iter,
        0, install ? "I" : "X",
        1, pkgName.c_str(),
        -1);

    // Reflect the new status immediately in the main package list.
    setPackageStatus(pkgName, install ? "I" : "X");
}

// Scans the list store to find the row for the given package and updates its status column.
// NOTE: This is O(n) – consider replacing with a row-reference map for large lists.
void MainWindow::setPackageStatus(const std::string &pkgName,
                                  const std::string &status) {
    GtkTreeModel *model = GTK_TREE_MODEL(m_listStore);
    GtkTreeIter   iter;
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gchar *name = nullptr;
        gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
        if (name && pkgName == name) {
            gtk_list_store_set(m_listStore, &iter, COL_STATUS, status.c_str(), -1);
            g_free(name);
            return;
        }
        g_free(name);
        valid = gtk_tree_model_iter_next(model, &iter);
    }
}

// Prompts for confirmation, then runs the queued install/remove commands.
void MainWindow::commitTransaction() {
    if (m_transactionRunning) return;

    if (m_transactionInstall.empty() && m_transactionRemove.empty()) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            _("No pending transactions."));
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
        gtk_widget_show(dialog);
        return;
    }

    // Build the confirmation message listing all packages to be processed.
    std::string msg = std::string(_("Execute transaction?")) + "\n\n";
    if (!m_transactionInstall.empty()) {
        msg += _("Install: ");
        for (auto &p : m_transactionInstall) msg += p + " ";
        msg += "\n";
    }
    if (!m_transactionRemove.empty()) {
        msg += _("Remove: ");
        for (auto &p : m_transactionRemove) msg += p + " ";
        msg += "\n";
    }

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_CANCEL,
        "%s", msg.c_str());
    gtk_dialog_add_button(GTK_DIALOG(dialog), _("OK"), GTK_RESPONSE_ACCEPT);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }
    gtk_widget_destroy(dialog);

    // Build the command queue: install first, then remove.
    m_commandQueue.clear();
    if (!m_transactionInstall.empty()) {
        std::vector<std::string> installArgs = { XBPS_INSTALL_BIN, "-y" };
        for (auto &p : m_transactionInstall) installArgs.push_back(p);
        m_commandQueue.push_back({ installArgs, _("Installing packages...") });
    }
    if (!m_transactionRemove.empty()) {
        std::vector<std::string> removeArgs = { XBPS_REMOVE_BIN, "-R", "-y" };
        for (auto &p : m_transactionRemove) removeArgs.push_back(p);
        m_commandQueue.push_back({ removeArgs, _("Removing packages...") });
    }

    // Clear marks before running so the UI resets cleanly.
    clearTransaction();
    runCommandQueue();
}

// Clears all pending install/remove marks and resets the transaction list.
void MainWindow::clearTransaction() {
    m_transactionInstall.clear();
    m_transactionRemove.clear();
    m_installMarks.clear();
    m_removeMarks.clear();
    gtk_list_store_clear(m_transStore);
    populateTable();
}
