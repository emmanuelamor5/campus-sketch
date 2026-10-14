#include "clipping.h"

namespace clipping {

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
        int c = ca ? ca : cb;
        Point p{};
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
        if (c == ca) { a = p; ca = outcode(w, a); }
        else { b = p; cb = outcode(w, b); }
    }
}

namespace {
enum class Edge { Left, Right, Bottom, Top };

bool inside(const Rect& w, const Point& p, Edge e) {
    switch (e) {
        case Edge::Left: return p.x >= w.xmin;
        case Edge::Right: return p.x <= w.xmax;
        case Edge::Bottom: return p.y >= w.ymin;
        case Edge::Top: return p.y <= w.ymax;
    }
    return false;
}

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

Polygon clipAgainst(const Rect& w, const Polygon& in, Edge e) {
    Polygon out;
    if (in.empty()) return out;
    for (size_t i = 0; i < in.size(); ++i) {
        const Point& cur = in[i];
        const Point& prev = in[(i + in.size() - 1) % in.size()];
        bool curIn = inside(w, cur, e);
        bool prevIn = inside(w, prev, e);
        if (curIn) {
            if (!prevIn) out.push_back(intersect(w, prev, cur, e));
            out.push_back(cur);
        } else if (prevIn) {
            out.push_back(intersect(w, prev, cur, e));
        }
    }
    return out;
}
}  // namespace

Polygon clipPolygon(const Rect& w, const Polygon& poly) {
    Polygon p = poly;
    p = clipAgainst(w, p, Edge::Left);
    p = clipAgainst(w, p, Edge::Right);
    p = clipAgainst(w, p, Edge::Bottom);
    p = clipAgainst(w, p, Edge::Top);
    return p;
}

}  // namespace clipping
