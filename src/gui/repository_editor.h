#ifndef IRUKA_REPOSITORY_EDITOR_H
#define IRUKA_REPOSITORY_EDITOR_H

#include <string>
#include <vector>

class Fl_Widget;
class Fl_Window;
class Fl_Tabs;
class Fl_Group;
class Fl_Text_Editor;
class Fl_Text_Buffer;

class RepositoryEditor {
public:
  explicit RepositoryEditor(Fl_Window *parent);
  ~RepositoryEditor();

  void show();
  void close();

private:
  struct RepoFile {
    std::string fileName;
    Fl_Group *page;
    Fl_Text_Editor *editor;
    Fl_Text_Buffer *buffer;
  };

  void buildUi();
  void loadFiles();
  void addTab(const std::string &fileName, const std::string &content);
  void addRepository();
  void deleteRepository();
  void saveAll();
  void applySaveResult(const std::string &output, bool success);
  void applyDeleteResult(int page, const std::string &output, bool success);

  static void cb_add(Fl_Widget *w, void *d);
  static void cb_del(Fl_Widget *w, void *d);
  static void cb_save(Fl_Widget *w, void *d);
  static void cb_close(Fl_Widget *w, void *d);
  static void save_done_awake(void *data);
  static void delete_done_awake(void *data);

  Fl_Window *m_parent;
  Fl_Window *m_window;
  Fl_Tabs *m_tabs;
  std::vector<RepoFile> m_files;
};

#endif
