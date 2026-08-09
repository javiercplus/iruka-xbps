#include "repository_editor.h"
#include "constants.h"
#include "i18n.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace {

const char *REPO_DIR = "/etc/xbps.d";

struct SaveResult {
  RepositoryEditor *editor;
  std::string output;
  bool success;
};

struct DeleteResult {
  RepositoryEditor *editor;
  int page;
  std::string output;
  bool success;
};

std::string fileNameFromText(const std::string &text) {
  std::string name;
  for (char c : text) {
    if (isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-')
      name += c;
  }
  if (name.empty()) name = "new-repo";
  if (name.size() < 5 || name.compare(name.size() - 5, 5, ".conf") != 0)
    name += ".conf";
  return name;
}

} // namespace

RepositoryEditor::RepositoryEditor(GtkWidget *parent)
  : m_parent(parent), m_window(nullptr), m_notebook(nullptr) {
  buildUi();
}

RepositoryEditor::~RepositoryEditor() {}

void RepositoryEditor::show() {
  gtk_widget_show_all(m_window);
  gtk_window_present(GTK_WINDOW(m_window));
}

void RepositoryEditor::close() {
  if (m_window) gtk_widget_destroy(m_window);
}

void RepositoryEditor::buildUi() {
  m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(m_window), _("Repositories"));
  gtk_window_set_transient_for(GTK_WINDOW(m_window), GTK_WINDOW(m_parent));
  gtk_window_set_default_size(GTK_WINDOW(m_window), 640, 480);
  g_signal_connect(m_window, "delete-event", G_CALLBACK(cb_delete), this);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(vbox, 8);
  gtk_widget_set_margin_end(vbox, 8);
  gtk_widget_set_margin_top(vbox, 8);
  gtk_widget_set_margin_bottom(vbox, 8);
  gtk_container_add(GTK_CONTAINER(m_window), vbox);

  GtkWidget *btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  GtkWidget *addBtn = gtk_button_new_with_label("+");
  gtk_widget_set_tooltip_text(addBtn, _("Add repository"));
  g_signal_connect(addBtn, "clicked", G_CALLBACK(cb_add), this);
  gtk_box_pack_start(GTK_BOX(btnBox), addBtn, FALSE, FALSE, 0);

  GtkWidget *delBtn = gtk_button_new_with_label("-");
  gtk_widget_set_tooltip_text(delBtn, _("Delete selected repository"));
  g_signal_connect(delBtn, "clicked", G_CALLBACK(cb_del), this);
  gtk_box_pack_start(GTK_BOX(btnBox), delBtn, FALSE, FALSE, 0);

  GtkWidget *saveBtn = gtk_button_new_with_label(_("Save"));
  gtk_widget_set_tooltip_text(saveBtn, _("Save repositories (requires root)"));
  g_signal_connect(saveBtn, "clicked", G_CALLBACK(cb_save), this);
  gtk_box_pack_end(GTK_BOX(btnBox), saveBtn, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), btnBox, FALSE, FALSE, 0);

  m_notebook = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(m_notebook), TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), m_notebook, TRUE, TRUE, 0);

  loadFiles();
}

void RepositoryEditor::loadFiles() {
  std::vector<std::string> files;
  DIR *dir = opendir(REPO_DIR);
  if (dir) {
    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
      std::string name = ent->d_name;
      if (name.size() >= 5 &&
          name.compare(name.size() - 5, 5, ".conf") == 0) {
        files.push_back(name);
      }
    }
    closedir(dir);
  }

  std::sort(files.begin(), files.end());

  for (auto &name : files) {
    std::ifstream in(std::string(REPO_DIR) + "/" + name);
    std::stringstream ss;
    ss << in.rdbuf();
    addTab(name, ss.str());
  }
}

void RepositoryEditor::addTab(const std::string &fileName,
                              const std::string &content) {
  GtkWidget *scroll = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GtkWidget *textView = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textView), GTK_WRAP_NONE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);
  gtk_container_add(GTK_CONTAINER(scroll), textView);

  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
  gtk_text_buffer_set_text(buf, content.c_str(), -1);

  gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), scroll,
    gtk_label_new(fileName.c_str()));

  m_files.push_back({fileName, textView});
}

void RepositoryEditor::addRepository() {
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
    _("Add repository"), GTK_WINDOW(m_window),
    static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
    _("Cancel"), GTK_RESPONSE_CANCEL,
    _("Add"), GTK_RESPONSE_ACCEPT, nullptr);
  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

  GtkWidget *nameEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(nameEntry), "new-repo.conf");
  GtkWidget *urlEntry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry),
    "https://repo.example.org/current");

  gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("File name:")), 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), nameEntry, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), gtk_label_new(_("Repository:")), 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), urlEntry, 1, 1, 1, 1);

  gtk_container_add(GTK_CONTAINER(content), grid);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
    gtk_widget_destroy(dialog);
    return;
  }

  std::string name = gtk_entry_get_text(GTK_ENTRY(nameEntry));
  std::string url = gtk_entry_get_text(GTK_ENTRY(urlEntry));
  gtk_widget_destroy(dialog);

  std::string fileName = fileNameFromText(name);

  for (auto &f : m_files) {
    if (f.fileName == fileName) {
      GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(m_window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
        _("A file named '%s' already exists."), fileName.c_str());
      g_signal_connect(msg, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
      gtk_widget_show(msg);
      return;
    }
  }

  if (url.empty()) {
    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(m_window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
      _("The repository URL cannot be empty."));
    g_signal_connect(msg, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
    gtk_widget_show(msg);
    return;
  }

  addTab(fileName, "repository=" + url + "\n");
  gtk_notebook_set_current_page(GTK_NOTEBOOK(m_notebook),
    gtk_notebook_get_n_pages(GTK_NOTEBOOK(m_notebook)) - 1);
}

void RepositoryEditor::deleteRepository() {
  int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(m_notebook));
  if (page < 0 || page >= static_cast<int>(m_files.size())) return;

  std::string fileName = m_files[page].fileName;

  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
    GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_CANCEL,
    _("Delete repository '%s'?\n\nThis will run: rm -f /etc/xbps.d/%s"),
    fileName.c_str(), fileName.c_str());
  gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), GTK_RESPONSE_ACCEPT);
  if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
    gtk_widget_destroy(dialog);
    return;
  }
  gtk_widget_destroy(dialog);

  std::string full = std::string(PKEXEC_BIN) + " rm -f " +
                     std::string(REPO_DIR) + "/" + fileName + " 2>&1";

  auto *result = new DeleteResult();
  result->editor = this;
  result->page = page;

  std::thread([result, full](){
    std::string output;
    FILE *pipe = popen(full.c_str(), "r");
    int status = 0;
    if (pipe) {
      char buf[4096];
      while (fgets(buf, sizeof(buf), pipe)) output += buf;
      status = pclose(pipe);
    }
    result->output = output;
    result->success = (status == 0);
    g_idle_add(delete_done_cb, result);
  }).detach();
}

void RepositoryEditor::applyDeleteResult(int page, const std::string &output,
                                         bool success) {
  if (success) {
    if (page >= 0 && page < static_cast<int>(m_files.size())) {
      gtk_notebook_remove_page(GTK_NOTEBOOK(m_notebook), page);
      m_files.erase(m_files.begin() + page);
    }
    return;
  }

  std::string msg = _("Failed to delete repository.");
  if (!output.empty()) msg += "\n\n" + output;
  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg.c_str());
  g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
  gtk_widget_show(dialog);
}

void RepositoryEditor::saveAll() {
  if (m_files.empty()) {
    GtkWidget *msg = gtk_message_dialog_new(GTK_WINDOW(m_window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
      _("No repositories to save."));
    g_signal_connect(msg, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
    gtk_widget_show(msg);
    return;
  }

  std::string command;
  std::string pidStr = std::to_string(getpid());
  int idx = 0;

  for (auto &f : m_files) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(f.textView));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buf, &start);
    gtk_text_buffer_get_end_iter(buf, &end);
    char *text = gtk_text_buffer_get_text(buf, &start, &end, TRUE);
    std::string content = text ? text : "";
    g_free(text);

    std::string tmp = "/tmp/iruka-repo-" + pidStr + "-" + std::to_string(idx++);
    std::ofstream out(tmp, std::ios::out | std::ios::trunc);
    out << content;
    out.close();

    command += "cp " + tmp + " " + std::string(REPO_DIR) + "/" + f.fileName +
               " && chmod 644 " + std::string(REPO_DIR) + "/" + f.fileName +
               " && rm -f " + tmp + " && ";
  }
  if (command.size() >= 4) command.erase(command.size() - 4);

  std::string full = std::string(PKEXEC_BIN) + " sh -c '" + command + "' 2>&1";

  auto *result = new SaveResult();
  result->editor = this;

  std::thread([result, full](){
    std::string output;
    FILE *pipe = popen(full.c_str(), "r");
    int status = 0;
    if (pipe) {
      char buf[4096];
      while (fgets(buf, sizeof(buf), pipe)) output += buf;
      status = pclose(pipe);
    }
    result->output = output;
    result->success = (status == 0);
    g_idle_add(save_done_cb, result);
  }).detach();
}

void RepositoryEditor::applySaveResult(const std::string &output, bool success) {
  GtkWidget *dialog;
  if (success) {
    dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
      _("Repositories saved successfully."));
  } else {
    std::string msg = _("Failed to save repositories.");
    if (!output.empty()) msg += "\n\n" + output;
    dialog = gtk_message_dialog_new(GTK_WINDOW(m_window),
      GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", msg.c_str());
  }
  g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
  gtk_widget_show(dialog);
}

void RepositoryEditor::cb_add(GtkWidget*, gpointer d) {
  static_cast<RepositoryEditor*>(d)->addRepository();
}

void RepositoryEditor::cb_del(GtkWidget*, gpointer d) {
  static_cast<RepositoryEditor*>(d)->deleteRepository();
}

void RepositoryEditor::cb_save(GtkWidget*, gpointer d) {
  static_cast<RepositoryEditor*>(d)->saveAll();
}

gboolean RepositoryEditor::cb_delete(GtkWidget*, GdkEvent*, gpointer d) {
  auto *editor = static_cast<RepositoryEditor*>(d);
  gtk_widget_hide(editor->m_window);
  return TRUE;
}

gboolean RepositoryEditor::save_done_cb(gpointer data) {
  auto *r = static_cast<SaveResult*>(data);
  r->editor->applySaveResult(r->output, r->success);
  delete r;
  return G_SOURCE_REMOVE;
}

gboolean RepositoryEditor::delete_done_cb(gpointer data) {
  auto *r = static_cast<DeleteResult*>(data);
  r->editor->applyDeleteResult(r->page, r->output, r->success);
  delete r;
  return G_SOURCE_REMOVE;
}
