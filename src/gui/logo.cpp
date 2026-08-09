// ----------------------------------------------------------------------------
// logo.cpp — Embedded application logo
//
// The About dialog (and the window icon) need a logo even when the icon theme
// is unavailable. The PNG bytes are compiled straight into the binary via
// logo_data.h, which is generated with:
//
//   xxd -i -n iruka_logo_png resources/icons/iruka.png > src/gui/logo_data.h
//
// A GdkPixbufLoader decodes those bytes at runtime, so no filesystem access
// or icon-theme lookup is required.
// ----------------------------------------------------------------------------

#include "logo.h"
#include "logo_data.h"

// Decodes the embedded PNG into a GdkPixbuf. Returns a new reference.
GdkPixbuf *logo_pixbuf() {
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    if (!loader) return nullptr;

    GError *error = nullptr;
    bool ok = gdk_pixbuf_loader_write(loader, iruka_logo_png, iruka_logo_png_len, &error);
    if (ok) ok = gdk_pixbuf_loader_close(loader, &error);

    GdkPixbuf *pixbuf = nullptr;
    if (ok) {
        // gdk_pixbuf_loader_get_pixbuf() returns a borrowed reference; take our own.
        pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
        if (pixbuf) g_object_ref(pixbuf);
    } else {
        g_warning("Failed to decode embedded logo: %s",
                  error ? error->message : "unknown error");
        if (error) g_error_free(error);
    }

    g_object_unref(loader);
    return pixbuf;
}
