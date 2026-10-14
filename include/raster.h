#pragma once

#include "canvas.h"
#include "geometry.h"

// Low-level raster primitives. Every function here is implemented from scratch
// using classic computer-graphics algorithms (no library fills or curves).
namespace raster {

// Bresenham's line algorithm, correct for all octants.
void drawLine(Canvas& c, Point a, Point b, Color col);

// Draw the closed outline of a polygon with Bresenham segments.
void drawPolygon(Canvas& c, const Polygon& poly, Color col);

// Scanline polygon fill (even-odd rule). Fills the interior only; pair it with
// drawPolygon() for a crisp outline.
void fillPolygon(Canvas& c, const Polygon& poly, Color col);

// Midpoint (Bresenham) circle. Used for tree canopies.
void drawCircle(Canvas& c, Point center, int radius, Color col);
void fillCircle(Canvas& c, Point center, int radius, Color col);

// Midpoint ellipse. Used for the soft shadow offset under a tree canopy.
void fillEllipse(Canvas& c, Point center, int rx, int ry, Color col);

// Evaluate a cubic Bezier at parameter t in [0, 1].
Point bezierPoint(const Point& p0, const Point& p1, const Point& p2,
                  const Point& p3, double t);

// Sample a cubic Bezier into a polyline of `segments` straight pieces.
std::vector<Point> sampleBezier(const Point& p0, const Point& p1,
                                const Point& p2, const Point& p3,
                                int segments = 40);

// Draw a polyline, optionally thickened by stamping a small filled disk at
// each sample. `thickness` is the full stroke width in pixels.
void drawThickPolyline(Canvas& c, const std::vector<Point>& pts,
                       double thickness, Color col);

// Build the 4 corners of a rectangle centered at `center`, sized length x
// width, rotated by `angleDeg`. Used to place cars at their parking angle.
Polygon rectangle(Point center, double length, double width, double angleDeg);

}  // namespace raster
