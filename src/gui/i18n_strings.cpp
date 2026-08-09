// ----------------------------------------------------------------------------
// i18n_strings.cpp — Translation extraction markers
//
// Some translatable strings are referenced in the code through constants
// defined in constants.h (e.g. _(IRUKA_APP_TITLE)). xgettext cannot see the
// string literals behind those identifiers, so the literals are repeated here
// inside _() purely so the POT extractor picks them up. This function is never
// called; it exists only to mark the strings for translation.
// ----------------------------------------------------------------------------

#include "i18n.h"

const char* iruka_i18n_markers() {
    _("Iruka-xbps Package Manager");
    _("Iruka-xbps");
    return nullptr;
}
