#ifndef IRUKA_I18N_H
#define IRUKA_I18N_H

// ----------------------------------------------------------------------------
// i18n.h — Internationalization helpers (gettext)
//
// Wrap every user-visible string literal in _() so it gets translated at
// runtime via the message catalog. Locale binding is set up in main.cpp.
// ----------------------------------------------------------------------------

#include <libintl.h>

// Location of the installed message catalogs. Overridable at build time via
// -DLOCALEDIR (set by meson.build) or at runtime via the IRUKA_LOCALEDIR
// environment variable (useful when running from the build directory).
#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

// Translates a string through the current text domain.
#define _(String) gettext(String)

// Marks a string for extraction only (no translation at runtime); useful for
// literals that feed into translatable contexts later.
#define N_(String) String

#endif // IRUKA_I18N_H
