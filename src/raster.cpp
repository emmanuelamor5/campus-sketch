#include "raster.h"

#include <algorithm>
#include <cmath>

namespace raster {

static int iround(double v) { return static_cast<int>(std::lround(v)); }

// Bresenham's line algorithm (integer-only, all octants).
//
// Steps one pixel at a time from (x0,y0) to (x1,y1). `err` tracks how far
// the pixel path has drifted from the ideal line, scaled by 2 to stay in
// integers. Each step looks at e2 = 2*err:
//   e2 >= dy -> the line has advanced enough in x: step x
//   e2 <= dx -> the line has advanced enough in y: step y
// Both can fire in one iteration (a diagonal step). Using signed steps
// sx/sy and a negative dy covers every octant with one loop, with no
// separate steep/shallow cases.
void drawLine(Canvas& c, Point a, Point b, Color col) {
    int x0 = iround(a.x), y0 = iround(a.y);
    int x1 = iround(b.x), y1 = iround(b.y);
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;  // error term for the first step
    while (true) {
        c.setPixel(x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }  // step in x
        if (e2 <= dx) { err += dx; y0 += sy; }  // step in y
    }
}

void drawPolygon(Canvas& c, const Polygon& poly, Color col) {
    if (poly.size() < 2) return;
    for (size_t i = 0; i < poly.size(); ++i) {
        drawLine(c, poly[i], poly[(i + 1) % poly.size()], col);
    }
}

// Scanline polygon fill, even-odd rule.
//
// For every pixel row inside the polygon's y-range we cast a horizontal ray,
// collect the x where it crosses each edge, sort them, and fill between
// crossings in pairs: [x0,x1], [x2,x3], ... Walking left to right, each
// crossing toggles inside/outside, so no convexity assumption is needed.
//
// The E-building shows why this matters: a row through the arms crosses the
// outline 4 or 6 times (spine left edge, then the left and right side of each
// arm it hits). Sorting gives spans like [spine..arm tip] [next arm..tip], and
// the gaps between pairs are exactly the concave notches between the arms,
// which stay unfilled. The same holds for the step on the spine and the notch
// beside the top tab.
//
// Vertex handling: the half-open test (one endpoint <= sy, the other > sy)
// counts a vertex shared by two edges once when the outline passes through
// it, and zero or two times at a local min/max, so the crossing count per row
// is always even. Horizontal edges never satisfy the test and are skipped;
// the edges around them supply the span ends.
void fillPolygon(Canvas& c, const Polygon& poly, Color col) {
    if (poly.size() < 3) return;
    double ymin = poly[0].y, ymax = poly[0].y;
    for (const auto& p : poly) {
        ymin = std::min(ymin, p.y);
        ymax = std::max(ymax, p.y);
    }
    // Only visit rows that are both inside the polygon and on the canvas.
    int yStart = std::max(0, iround(std::ceil(ymin)));
    int yEnd = std::min(c.height() - 1, iround(std::floor(ymax)));

    std::vector<double> xs;  // reused across rows to avoid reallocating
    for (int y = yStart; y <= yEnd; ++y) {
        xs.clear();
        double sy = y + 0.5;  // sample at pixel center to avoid vertex glitches
        for (size_t i = 0; i < poly.size(); ++i) {
            const Point& a = poly[i];
            const Point& b = poly[(i + 1) % poly.size()];
            // Does the horizontal scanline cross edge a-b? (half-open in y)
            if ((a.y <= sy && b.y > sy) || (b.y <= sy && a.y > sy)) {
                // Linear interpolation along the edge gives the crossing x.
                double t = (sy - a.y) / (b.y - a.y);
                xs.push_back(a.x + t * (b.x - a.x));
            }
        }
        std::sort(xs.begin(), xs.end());
        // Fill inside spans only: pairs (0,1), (2,3), ... Each span covers the
        // integer x in [ceil(left), floor(right)], clamped to the canvas.
        for (size_t i = 0; i + 1 < xs.size(); i += 2) {
            int xl = std::max(0, iround(std::ceil(xs[i])));
            int xr = std::min(c.width() - 1, iround(std::floor(xs[i + 1])));
            for (int x = xl; x <= xr; ++x) c.setPixel(x, y, col);
        }
    }
}

// Midpoint circle algorithm (Bresenham's circle). Used for tree canopy rims.
//
// Walks one octant only, from (0, r) going right until x > y, and mirrors
// each point into the other seven octants with plot8. At every step it
// chooses between moving east (x+1, y) or south-east (x+1, y-1) by testing
// whether the midpoint between those two pixels is inside the circle:
//   d < 0  -> midpoint inside, stay on this row:  d += 2x + 3
//   d >= 0 -> midpoint outside, drop a row:        d += 2(x - y) + 5
// d starts at 1 - r (the integer form of 5/4 - r), so everything stays in
// integer arithmetic.
void drawCircle(Canvas& c, Point center, int radius, Color col) {
    int cx = iround(center.x), cy = iround(center.y);
    int x = 0, y = radius, d = 1 - radius;  // d = decision variable
    auto plot8 = [&](int px, int py) {
        c.setPixel(cx + px, cy + py, col);
        c.setPixel(cx - px, cy + py, col);
        c.setPixel(cx + px, cy - py, col);
        c.setPixel(cx - px, cy - py, col);
        c.setPixel(cx + py, cy + px, col);
        c.setPixel(cx - py, cy + px, col);
        c.setPixel(cx + py, cy - px, col);
        c.setPixel(cx - py, cy - px, col);
    };
    while (x <= y) {
        plot8(x, y);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

// Filled disk from the same midpoint circle walk (tree canopies).
//
// Instead of plotting the 8 symmetric points, each octant point (x, y) is
// turned into horizontal spans between its mirrored left and right points.
// span(y, x) fills the wide rows near the middle and span(x, y) the short
// rows near the top and bottom, so every row of the disk is filled with no
// gaps and no sqrt.
void fillCircle(Canvas& c, Point center, int radius, Color col) {
    int cx = iround(center.x), cy = iround(center.y);
    int x = 0, y = radius, d = 1 - radius;
    // Fill the row from cx - px to cx + px at cy + py and at cy - py.
    auto span = [&](int px, int py) {
        for (int i = cx - px; i <= cx + px; ++i) {
            c.setPixel(i, cy + py, col);
            c.setPixel(i, cy - py, col);
        }
    };
    while (x <= y) {
        span(y, x);  // wide rows
        span(x, y);  // tall rows
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            --y;
        }
        ++x;
    }
}

void fillEllipse(Canvas& c, Point center, int rx, int ry, Color col) {
    int cx = iround(center.x), cy = iround(center.y);
    if (rx <= 0 || ry <= 0) return;
    for (int y = -ry; y <= ry; ++y) {
        double t = static_cast<double>(y) / ry;
        double w = rx * std::sqrt(std::max(0.0, 1.0 - t * t));
        int wi = iround(w);
        for (int x = -wi; x <= wi; ++x) c.setPixel(cx + x, cy + y, col);
    }
}

// Cubic Bezier in Bernstein form:
//   B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3,  t in [0, 1]
// P0/P3 are the endpoints (the curve passes through them); P1/P2 are control
// points that only pull the curve toward them (the "handles"). The four
// weights b0..b3 always sum to 1, so every sample is a convex blend of the
// control points and stays inside their hull.
Point bezierPoint(const Point& p0, const Point& p1, const Point& p2,
                  const Point& p3, double t) {
    double u = 1.0 - t;
    double b0 = u * u * u;
    double b1 = 3 * u * u * t;
    double b2 = 3 * u * t * t;
    double b3 = t * t * t;
    return {b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x,
            b0 * p0.y + b1 * p1.y + b2 * p2.y + b3 * p3.y};
}

// Flatten the curve into a polyline of `segments + 1` points by evaluating
// B(t) at uniform steps t = i / segments (i = 0..segments). Uniform t is not
// uniform arc length, but at 60 samples the chords are short enough that the
// result looks smooth and Cohen-Sutherland can clip each chord independently.
std::vector<Point> sampleBezier(const Point& p0, const Point& p1,
                                const Point& p2, const Point& p3,
                                int segments) {
    std::vector<Point> out;
    segments = std::max(1, segments);
    out.reserve(segments + 1);
    for (int i = 0; i <= segments; ++i) {
        out.push_back(bezierPoint(p0, p1, p2, p3, static_cast<double>(i) / segments));
    }
    return out;
}

// Thick stroke built from a 1-px Bresenham centerline plus round "joints":
//   1. drawLine() joins each pair of consecutive samples.
//   2. A filled circle of radius thickness/2 is stamped on every sample.
// The circles widen the line to `thickness` pixels and also fill the wedge
// gaps that appear on the outside of a bend. main.cpp passes ONE short chord
// (2 points) per call after clipping it, so each call stamps circles at both
// ends; neighbouring chords overlap there and the stroke stays continuous.
void drawThickPolyline(Canvas& c, const std::vector<Point>& pts,
                       double thickness, Color col) {
    if (pts.empty()) return;
    int r = std::max(0, iround(thickness / 2.0));
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        drawLine(c, pts[i], pts[i + 1], col);
    }
    if (r > 0) {
        for (const auto& p : pts) fillCircle(c, p, r, col);
    }
}

// Car symbol: rotated rectangle (2D rotation transform).
//
// Build the four corners of an axis-aligned length x width box around
// `center`, then rotate each corner about the center by angleDeg with
// x' = cx + dx*cos - dy*sin,  y' = cy + dx*sin + dy*cos.
// With +y pointing down, a positive angle turns the car clockwise on screen.
// The result is a normal polygon, so it can be clipped, filled by the
// scanline fill and outlined with Bresenham lines.
Polygon rectangle(Point center, double length, double width, double angleDeg) {
    double hl = length / 2.0, hw = width / 2.0;
    Polygon corners = {
        {center.x - hl, center.y - hw},
        {center.x + hl, center.y - hw},
        {center.x + hl, center.y + hw},
        {center.x - hl, center.y + hw},
    };
    for (auto& p : corners) p = rotateAround(p, center, angleDeg);
    return corners;
}

}  // namespace raster
