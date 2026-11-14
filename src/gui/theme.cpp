#include "theme.h"
#include <FL/Fl.H>

namespace theme {

void apply() {
  Fl::scheme("gtk+");
}

}
