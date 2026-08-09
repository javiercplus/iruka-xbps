#ifndef IRUKA_REPOSITORY_EDITOR_H
#define IRUKA_REPOSITORY_EDITOR_H

#include <gtk/gtk.h>
#include <string>
#include <vector>

class RepositoryEditor {
public:
  explicit RepositoryEditor(GtkWidget *parent);
  ~RepositoryEditor();

  void show();
  // Destroys the editor window (used when the main UI is rebuilt on a language
  // change). The object is still valid but should be deleted by the owner.
  void close();

private:
  struct RepoFile {
    std::string fileName;
    GtkWidget *textView;
  };

  void buildUi();
  void loadFiles();
  void addTab(const std::string &fileName, const std::string &content);
  void addRepository();
  void deleteRepository();
  void saveAll();
  void applySaveResult(const std::string &output, bool success);
  void applyDeleteResult(int page, const std::string &output, bool success);

  static void cb_add(GtkWidget *w, gpointer d);
  static void cb_del(GtkWidget *w, gpointer d);
  static void cb_save(GtkWidget *w, gpointer d);
  static gboolean cb_delete(GtkWidget *w, GdkEvent *event, gpointer d);
  static gboolean save_done_cb(gpointer data);
  static gboolean delete_done_cb(gpointer data);

  GtkWidget *m_parent;
  GtkWidget *m_window;
  GtkWidget *m_notebook;
  std::vector<RepoFile> m_files;
};

#endif
