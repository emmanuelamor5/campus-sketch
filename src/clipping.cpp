#include "clipping.h"

namespace clipping {


// Cohen-Sutherland line clipping
//
// Each endpoint gets a 4-bit "outcode" saying which side(s) of the window it
// lies beyond. The nine regions around the window look like this (+y is down
// in scene space, so BOTTOM = y < ymin is the top row on screen):
//
//            LEFT|BOTTOM   BOTTOM   RIGHT|BOTTOM
//            LEFT          INSIDE   RIGHT
//            LEFT|TOP      TOP      RIGHT|TOP
//
// Two bitwise tests then settle most segments without any arithmetic:
//   (ca | cb) == 0  -> both endpoints inside: trivially accept.
//   (ca & cb) != 0  -> both endpoints beyond the same edge: trivially reject.
// Otherwise pick an endpoint that is outside, slide it along the segment onto
// the edge its outcode names, recompute its outcode and repeat. Each pass
// clears at least one bit, so the loop runs at most four times.


namespace {
constexpr int INSIDE = 0, LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8;

int outcode(const Rect& w, const Point& p) {
    int code = INSIDE;
    if (p.x < w.xmin) code |= LEFT;
    else if (p.x > w.xmax) code |= RIGHT;
    if (p.y < w.ymin) code |= BOTTOM;
    else if (p.y > w.ymax) code |= TOP;
    return code;
}
}  // namespace

bool clipLine(const Rect& w, Point& a, Point& b) {
    int ca = outcode(w, a), cb = outcode(w, b);
    while (true) {
        if (!(ca | cb)) return true;        // both inside
        if (ca & cb) return false;          // both share an outside zone
        // Move whichever endpoint is outside (prefer `a`).
        int c = ca ? ca : cb;
        Point p{};
        // Intersect the segment with the edge named by the outcode, using
        // the line's parametric form. The divisors are non-zero here: e.g. if
        // c has TOP set, one endpoint is above ymax and the other is not, so
        // b.y != a.y.
        if (c & TOP) {
            p.x = a.x + (b.x - a.x) * (w.ymax - a.y) / (b.y - a.y);
            p.y = w.ymax;
        } else if (c & BOTTOM) {
            p.x = a.x + (b.x - a.x) * (w.ymin - a.y) / (b.y - a.y);
            p.y = w.ymin;
        } else if (c & RIGHT) {
            p.y = a.y + (b.y - a.y) * (w.xmax - a.x) / (b.x - a.x);
            p.x = w.xmax;
        } else {  // LEFT
            p.y = a.y + (b.y - a.y) * (w.xmin - a.x) / (b.x - a.x);
            p.x = w.xmin;
        }
        // Replace the outside endpoint with the edge point and re-classify.
        if (c == ca) { a = p; ca = outcode(w, a); }
        else { b = p; cb = outcode(w, b); }
    }
}


// Sutherland-Hodgman polygon clipping
//
// The window is the intersection of four half-planes (x >= xmin, x <= xmax,
// y >= ymin, y <= ymax). The polygon is clipped against one half-plane at a
// time, and each stage's output is the next stage's input. Within a stage,
// each edge prev -> cur emits vertices by these four cases:
//
//   prev in,  cur in   -> emit cur
//   prev in,  cur out  -> emit the crossing point (leaving the window)
//   prev out, cur in   -> emit the crossing point, then cur (entering)
//   prev out, cur out  -> emit nothing
//
// This works for any simple polygon against a convex window, including the
// concave E-building footprint. A concave polygon that the window splits into
// separate pieces comes back as one polygon whose pieces are joined by
// zero-width edges running along the window border. That is harmless for the
// even-odd scanline fill.


namespace {
enum class Edge { Left, Right, Bottom, Top };

// Is p on the kept side of edge e? Points exactly on the edge count as inside.
bool inside(const Rect& w, const Point& p, Edge e) {
    switch (e) {
        case Edge::Left: return p.x >= w.xmin;
        case Edge::Right: return p.x <= w.xmax;
        case Edge::Bottom: return p.y >= w.ymin;
        case Edge::Top: return p.y <= w.ymax;
    }
    return false;
}

// Point where segment a->b crosses edge e. Only called when a and b lie on
// opposite sides, so the divisor is never zero.
Point intersect(const Rect& w, const Point& a, const Point& b, Edge e) {
    double t = 0.0;
    switch (e) {
        case Edge::Left: t = (w.xmin - a.x) / (b.x - a.x); break;
        case Edge::Right: t = (w.xmax - a.x) / (b.x - a.x); break;
        case Edge::Bottom: t = (w.ymin - a.y) / (b.y - a.y); break;
        case Edge::Top: t = (w.ymax - a.y) / (b.y - a.y); break;
    }
    return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
}

// One Sutherland-Hodgman stage: clip `in` against a single window edge.
Polygon clipAgainst(const Rect& w, const Polygon& in, Edge e) {
    Polygon out;
    if (in.empty()) return out;
    for (size_t i = 0; i < in.size(); ++i) {
        const Point& cur = in[i];
        const Point& prev = in[(i + in.size() - 1) % in.size()];  // wraps to close the polygon
        bool curIn = inside(w, cur, e);
        bool prevIn = inside(w, prev, e);
        if (curIn) {
            if (!prevIn) out.push_back(intersect(w, prev, cur, e));  // entering
            out.push_back(cur);
        } else if (prevIn) {
            out.push_back(intersect(w, prev, cur, e));  // leaving
        }
    }
    return out;
}
}  // namespace

Polygon clipPolygon(const Rect& w, const Polygon& poly) {
    // Pipeline the four edge stages. An empty result means the polygon was
    // entirely outside; callers skip anything with fewer than 3 vertices.
    Polygon p = poly;
    p = clipAgainst(w, p, Edge::Left);
    p = clipAgainst(w, p, Edge::Right);
    p = clipAgainst(w, p, Edge::Bottom);
    p = clipAgainst(w, p, Edge::Top);
    return p;
}

}  // namespace clipping
