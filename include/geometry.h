#pragma once

#include <cmath>
#include <vector>

// A point in image space. We keep coordinates as doubles while transforming
// (rotation, viewport mapping, Bezier sampling) and only round to integer
// pixels at the final rasterization step.
struct Point {
    double x = 0.0;
    double y = 0.0;
};

inline Point operator+(const Point& a, const Point& b) { return {a.x + b.x, a.y + b.y}; }
inline Point operator-(const Point& a, const Point& b) { return {a.x - b.x, a.y - b.y}; }
inline Point operator*(const Point& a, double s) { return {a.x * s, a.y * s}; }

// An ordered list of vertices. Used for building footprints, parking lots and
// lawns, and as the output of polygon clipping.
using Polygon = std::vector<Point>;

// Axis-aligned rectangle, used as the viewport / clipping window.
struct Rect {
    double xmin, ymin, xmax, ymax;
};

// Rotate `p` around pivot `c` by `deg` degrees (clockwise in image space,
// where +y points down). Used to orient cars to their parking angle.
inline Point rotateAround(const Point& p, const Point& c, double deg) {
    double rad = deg * 3.14159265358979323846 / 180.0;
    double s = std::sin(rad), co = std::cos(rad);
    Point d = p - c;
    return {c.x + d.x * co - d.y * s, c.y + d.x * s + d.y * co};
}
