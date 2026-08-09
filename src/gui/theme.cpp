#include "theme.h"
#include <gtk/gtk.h>

namespace theme {

// Application-level CSS overrides. Forces flat, border-radius-free styling
// and applies a tab underline style for the notebook widget.
static const char *kCss =
  "* {\n"
  "  border-radius: 0;\n"
  "}\n"
  "button, entry, combobox, menubar > menuitem, menu item, notebook tab {\n"
  "  border-radius: 0;\n"
  "}\n"
  "notebook > header {\n"
  "  background-color: @theme_bg_color;\n"
  "  border: none;\n"
  "  border-bottom: 1px solid @borders;\n"
  "  padding: 0;\n"
  "  box-shadow: none;\n"
  "}\n"
  "notebook > header > tabs > tab {\n"
  "  padding: 7px 14px;\n"
  "  border: none;\n"
  "  border-bottom: 2px solid transparent;\n"
  "  background: transparent;\n"
  "  box-shadow: none;\n"
  "}\n"
  "notebook > header > tabs > tab label {\n"
  "  color: alpha(@theme_fg_color, 0.55);\n"
  "}\n"
  "notebook > header > tabs > tab:checked {\n"
  "  background: transparent;\n"
  "  border-bottom: 2px solid @theme_selected_bg_color;\n"
  "}\n"
  "notebook > header > tabs > tab:checked label {\n"
  "  color: @theme_fg_color;\n"
  "}\n"
  "notebook > header > tabs > arrow {\n"
  "  border: none;\n"
  "  background: transparent;\n"
  "}\n"
  "notebook scrolledwindow {\n"
  "  border: none;\n"
  "  background: @theme_base_color;\n"
  "}\n";

// Loads and applies the application CSS to the default GDK screen.
void apply() {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider, kCss, -1, nullptr);
  gtk_style_context_add_provider_for_screen(
    gdk_screen_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);
}

} // namespace theme
