#pragma once

#include "geometry.h"

// Viewport clipping against an axis-aligned rectangular window.
namespace clipping {

// Cohen-Sutherland line clip. Returns false if the segment is entirely
// outside; otherwise trims `a`/`b` in place to the window edges.
bool clipLine(const Rect& win, Point& a, Point& b);

// Sutherland-Hodgman polygon clip against the window. Returns the (possibly
// empty) clipped polygon.
Polygon clipPolygon(const Rect& win, const Polygon& poly);

}  // namespace clipping
