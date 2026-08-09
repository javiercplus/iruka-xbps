#ifndef IRUKA_LOGO_H
#define IRUKA_LOGO_H

#include <gtk/gtk.h>

// Returns a new reference to the application logo embedded in the binary
// (see logo_data.h, generated with `xxd -i resources/icons/iruka.png`).
// Returns nullptr if the embedded image cannot be decoded; the caller owns
// the returned reference and must release it with g_object_unref().
GdkPixbuf *logo_pixbuf();

#endif // IRUKA_LOGO_H
