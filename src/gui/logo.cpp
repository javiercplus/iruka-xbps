#include "logo.h"
#include "logo_data.h"
#include <FL/Fl_PNG_Image.H>

Fl_Image *logo_image() {
    auto *png = new Fl_PNG_Image("iruka", iruka_logo_png, iruka_logo_png_len);
    if (png->fail()) {
        delete png;
        return nullptr;
    }

    Fl_Image *scaled = png->copy(80, 80);
    delete png;
    return scaled;
}
