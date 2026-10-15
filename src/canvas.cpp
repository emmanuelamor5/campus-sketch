#include "canvas.h"

#include <algorithm>
#include <fstream>

Canvas::Canvas(int width, int height, Color background)
    : w_(width), h_(height), px_(static_cast<size_t>(width) * height, background) {}

void Canvas::setPixel(int x, int y, Color c) {
    if (x < 0 || x >= w_ || y < 0 || y >= h_) {
        return;
    }

    px_[static_cast<size_t>(y) * w_ + x] = c;
}

Color Canvas::getPixel(int x, int y) const {
    if (x < 0 || x >= w_ || y < 0 || y >= h_) {
        return palette::black;
    }

    return px_[static_cast<size_t>(y) * w_ + x];
}

void Canvas::clear(Color c) {
    std::fill(px_.begin(), px_.end(), c);
}

bool Canvas::savePPM(const std::string& path) const {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }

    output << "P6\n" << w_ << " " << h_ << "\n255\n";
    output.write(reinterpret_cast<const char*>(px_.data()),
            static_cast<std::streamsize>(px_.size()) * 3);
    return static_cast<bool>(output);
}
