#include "repository_editor.h"
#include "constants.h"
#include "i18n.h"
#include "privileged_daemon.h"
#include "window_utils.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace {

class RepoTabs : public Fl_Tabs {
public:
  RepoTabs(int X, int Y, int W, int H) : Fl_Tabs(X, Y, W, H) {}
  void resize(int X, int Y, int W, int H) override {
    Fl_Tabs::resize(X, Y, W, H);
    for (int i = 0; i < children(); ++i) {
      Fl_Widget *page = child(i);
      page->resize(X, Y + 24, W, H - 24);
    }
  }
};

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

class AddRepoDialog {
public:
  AddRepoDialog() {
    win = new Fl_Window(440, 190, _("Add repository"));

    win->begin();

    Fl_Box *nameLabel = new Fl_Box(20, 30, 110, 28, _("File name:"));
    nameLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    nameIn = new Fl_Input(140, 30, 280, 28);
    nameIn->value("new-repo.conf");

    Fl_Box *urlLabel = new Fl_Box(20, 70, 110, 28, _("Repository:"));
    urlLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    urlIn = new Fl_Input(140, 70, 280, 28);
    urlIn->value("https://repo.example.org/current");
    urlIn->tooltip("https://repo.example.org/current");

    Fl_Button *cancel = new Fl_Button(240, 136, 80, 28, _("Cancel"));
    cancel->callback(cb_cancel, this);
    Fl_Button *add = new Fl_Button(330, 136, 90, 28, _("Add"));
    add->callback(cb_ok, this);

    win->end();
  }

  ~AddRepoDialog() { delete win; }

  bool run() {
    win->set_modal();
    win->show();
    while (win->shown()) Fl::wait();
    return accepted;
  }

  std::string name() const { return nameIn->value() ? nameIn->value() : ""; }
  std::string url() const { return urlIn->value() ? urlIn->value() : ""; }

private:
  static void cb_ok(Fl_Widget *, void *d) {
    auto *self = static_cast<AddRepoDialog *>(d);
    self->accepted = true;
    self->win->hide();
  }
  static void cb_cancel(Fl_Widget *, void *d) {
    auto *self = static_cast<AddRepoDialog *>(d);
    self->win->hide();
  }

  Fl_Window *win = nullptr;
  Fl_Input *nameIn = nullptr;
  Fl_Input *urlIn = nullptr;
  bool accepted = false;
};

}

RepositoryEditor::RepositoryEditor(Fl_Window *parent)
  : m_parent(parent), m_window(nullptr), m_tabs(nullptr) {
  buildUi();
}

RepositoryEditor::~RepositoryEditor() {
  delete m_window;
  m_window = nullptr;
  for (auto &f : m_files) delete f.buffer;
  m_files.clear();
}

void RepositoryEditor::show() {
  gui_utils::showCentered(m_window, m_parent);
}

void RepositoryEditor::close() {
  if (m_window) m_window->hide();
}

void RepositoryEditor::buildUi() {
  m_window = new Fl_Window(640, 480, _("Repositories"));
  m_window->callback(cb_close, this);

  m_window->begin();

  Fl_Button *addBtn = new Fl_Button(8, 8, 32, 28, "+");
  addBtn->tooltip(_("Add repository"));
  addBtn->callback(cb_add, this);

  Fl_Button *delBtn = new Fl_Button(46, 8, 32, 28, "-");
  delBtn->tooltip(_("Delete selected repository"));
  delBtn->callback(cb_del, this);

  Fl_Button *saveBtn = new Fl_Button(544, 8, 88, 28, _("Save"));
  saveBtn->tooltip(_("Save repositories (requires root)"));
  saveBtn->callback(cb_save, this);

  m_tabs = new RepoTabs(8, 44, 624, 428);
  m_tabs->end();

  m_window->end();
  m_window->resizable(m_tabs);

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
  m_tabs->begin();

  Fl_Group *page = new Fl_Group(0, 0, 10, 10);
  page->copy_label(fileName.c_str());
  page->begin();

  Fl_Text_Editor *editor = new Fl_Text_Editor(0, 0, 10, 10);
  Fl_Text_Buffer *buffer = new Fl_Text_Buffer();
  editor->buffer(buffer);
  editor->textfont(FL_COURIER);
  editor->wrap_mode(Fl_Text_Display::WRAP_NONE, 0);
  buffer->text(content.c_str());
  page->resizable(editor);
  page->end();

  m_tabs->end();

  page->resize(m_tabs->x(), m_tabs->y() + 24, m_tabs->w(), m_tabs->h() - 24);

  m_files.push_back({fileName, page, editor, buffer});
  m_tabs->value(page);
  m_window->redraw();
}

void RepositoryEditor::addRepository() {
  AddRepoDialog dialog;
  if (!dialog.run()) return;

  std::string fileName = fileNameFromText(dialog.name());
  std::string url = dialog.url();

  for (auto &f : m_files) {
    if (f.fileName == fileName) {
      fl_alert(_("A file named '%s' already exists."), fileName.c_str());
      return;
    }
  }

  if (url.empty()) {
    fl_alert("%s", _("The repository URL cannot be empty."));
    return;
  }

  addTab(fileName, "repository=" + url + "\n");
}

void RepositoryEditor::deleteRepository() {
  Fl_Widget *cur = m_tabs->value();
  int page = -1;
  for (size_t i = 0; i < m_files.size(); ++i) {
    if (m_files[i].page == cur) { page = static_cast<int>(i); break; }
  }
  if (page < 0 || page >= static_cast<int>(m_files.size())) return;

  std::string fileName = m_files[page].fileName;

  int choice = fl_choice(_("Delete repository '%s'?\n\n"
                           "This will run: rm -f /etc/xbps.d/%s"),
                         _("Cancel"), _("Delete"), nullptr,
                         fileName.c_str(), fileName.c_str());
  if (choice != 1) return;

  if (!gui_utils::requestPkexecPrivileges()) return;

  std::string cmd = "rm -f " + std::string(REPO_DIR) + "/" + fileName;

  auto *result = new DeleteResult();
  result->editor = this;
  result->page = page;

  std::thread([result, cmd]() {
    std::string output;
    int code = PrivilegedDaemon::instance().run(cmd,
        [&output](const std::string &t) { output += t; });
    result->output = output;
    result->success = (code == 0);
    Fl::awake(delete_done_awake, result);
  }).detach();
}

void RepositoryEditor::applyDeleteResult(int page, const std::string &output,
                                         bool success) {
  if (success) {
    if (page >= 0 && page < static_cast<int>(m_files.size())) {
      RepoFile f = m_files[page];
      m_files.erase(m_files.begin() + page);
      m_tabs->remove(f.page);
      delete f.page;
      delete f.buffer;
      m_window->redraw();
    }
    return;
  }

  std::string msg = _("Failed to delete repository.");
  if (!output.empty()) msg += "\n\n" + output;
  fl_alert("%s", msg.c_str());
}

void RepositoryEditor::saveAll() {
  if (m_files.empty()) {
    fl_message("%s", _("No repositories to save."));
    return;
  }

  if (!gui_utils::requestPkexecPrivileges()) return;

  std::string command;
  std::string pidStr = std::to_string(getpid());
  int idx = 0;

  for (auto &f : m_files) {
    std::string content = f.buffer->text() ? f.buffer->text() : "";

    std::string tmp = "/tmp/iruka-repo-" + pidStr + "-" + std::to_string(idx++);
    std::ofstream out(tmp, std::ios::out | std::ios::trunc);
    out << content;
    out.close();

    command += "cp " + tmp + " " + std::string(REPO_DIR) + "/" + f.fileName +
               " && chmod 644 " + std::string(REPO_DIR) + "/" + f.fileName +
               " && rm -f " + tmp + " && ";
  }
  if (command.size() >= 4) command.erase(command.size() - 4);

  auto *result = new SaveResult();
  result->editor = this;

  std::thread([result, command]() {
    std::string output;
    int code = PrivilegedDaemon::instance().run(command,
        [&output](const std::string &t) { output += t; });
    result->output = output;
    result->success = (code == 0);
    Fl::awake(save_done_awake, result);
  }).detach();
}

void RepositoryEditor::applySaveResult(const std::string &output, bool success) {
  if (success) {
    fl_message("%s", _("Repositories saved successfully."));
  } else {
    std::string msg = _("Failed to save repositories.");
    if (!output.empty()) msg += "\n\n" + output;
    fl_alert("%s", msg.c_str());
  }
}

void RepositoryEditor::cb_add(Fl_Widget *, void *d) {
  static_cast<RepositoryEditor *>(d)->addRepository();
}

void RepositoryEditor::cb_del(Fl_Widget *, void *d) {
  static_cast<RepositoryEditor *>(d)->deleteRepository();
}

void RepositoryEditor::cb_save(Fl_Widget *, void *d) {
  static_cast<RepositoryEditor *>(d)->saveAll();
}

void RepositoryEditor::cb_close(Fl_Widget *, void *d) {
  static_cast<RepositoryEditor *>(d)->close();
}

void RepositoryEditor::save_done_awake(void *data) {
  auto *r = static_cast<SaveResult *>(data);
  r->editor->applySaveResult(r->output, r->success);
  delete r;
}

void RepositoryEditor::delete_done_awake(void *data) {
  auto *r = static_cast<DeleteResult *>(data);
  r->editor->applyDeleteResult(r->page, r->output, r->success);
  delete r;
}
