#pragma once

#include <string>
#include <vector>

#include "color.h"

// A software framebuffer. Every drawing routine writes into this pixel buffer
// via setPixel(); nothing touches the screen or an external graphics library.
// The finished image is written out as a binary PPM (P6) file.
class Canvas {
public:
    Canvas(int width, int height, Color background = palette::white);

    int width() const { return w_; }
    int height() const { return h_; }

    // Bounds-checked write; out-of-range coordinates are ignored so drawing
    // code never has to clamp before plotting.
    void setPixel(int x, int y, Color c);
    Color getPixel(int x, int y) const;

    void clear(Color c);

    // Write the buffer to `path` as a PPM (P6). Returns false on I/O failure.
    bool savePPM(const std::string& path) const;

private:
    int w_;
    int h_;
    std::vector<Color> px_;
};
