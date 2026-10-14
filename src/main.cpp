#include <cstdio>
#include <string>

#include "canvas.h"
#include "clipping.h"
#include "raster.h"
#include "scene.h"

namespace {

// Maps world (scene) coordinates to screen pixels. For the full campus view
// this is the identity; for the zoom view it maps the viewport window onto the
// whole canvas.
struct Transform {
    double sx = 1, sy = 1, tx = 0, ty = 0;
    Point apply(const Point& p) const { return {p.x * sx + tx, p.y * sy + ty}; }
    Polygon apply(const Polygon& poly) const {
        Polygon out;
        out.reserve(poly.size());
        for (const auto& p : poly) out.push_back(apply(p));
        return out;
    }
};

Transform fitWindow(const Rect& win, int w, int h) {
    Transform t;
    t.sx = w / (win.xmax - win.xmin);
    t.sy = h / (win.ymax - win.ymin);
    t.tx = -win.xmin * t.sx;
    t.ty = -win.ymin * t.sy;
    return t;
}

// Fill + outline a polygon that has already been clipped and transformed.
void paintPolygon(Canvas& c, const Polygon& poly, Color fill, Color edge) {
    if (poly.size() < 3) return;
    raster::fillPolygon(c, poly, fill);
    raster::drawPolygon(c, poly, edge);
}

// Render the whole scene. When `win` is set, every element is clipped to that
// window (Cohen-Sutherland for road polylines, Sutherland-Hodgman for polygons)
// and mapped through `xf` so the window fills the canvas.
void renderScene(Canvas& canvas, const scene::Scene& s,
                 const std::optional<Rect>& win, const Transform& xf) {
    canvas.clear(palette::ground);

    // 1. Site pads (white ground under the building), then lawns / planters.
    for (const auto& pad : s.pads) {
        Polygon p = win ? clipping::clipPolygon(*win, pad.outline) : pad.outline;
        if (p.size() >= 3) paintPolygon(canvas, xf.apply(p), palette::pad, palette::padEdge);
    }
    for (const auto& l : s.lawns) {
        Polygon p = win ? clipping::clipPolygon(*win, l.outline) : l.outline;
        if (p.size() >= 3) paintPolygon(canvas, xf.apply(p), palette::lawn, palette::canopyShadow);
    }

    // 2. Roads: sample each cubic Bezier, clip segment-by-segment, draw thick.
    for (const auto& r : s.roads) {
        auto pts = raster::sampleBezier(r.p0, r.p1, r.p2, r.p3, 60);
        std::vector<Point> screenPts;
        for (size_t i = 0; i + 1 < pts.size(); ++i) {
            Point a = pts[i], b = pts[i + 1];
            if (win && !clipping::clipLine(*win, a, b)) continue;
            raster::drawThickPolyline(canvas, {xf.apply(a), xf.apply(b)},
                                      r.thickness * xf.sx, palette::road);
        }
        // Center dashes for a bit of road detail (unclipped-safe: reuse pts).
    }

    // 3. Parking lots.
    for (const auto& pk : s.parking) {
        Polygon p = win ? clipping::clipPolygon(*win, pk.outline) : pk.outline;
        if (p.size() >= 3) paintPolygon(canvas, xf.apply(p), palette::parking, palette::roadEdge);
    }

    // 4. Buildings (footprint polygon + scanline fill + outline).
    for (const auto& b : s.buildings) {
        Polygon p = win ? clipping::clipPolygon(*win, b.footprint) : b.footprint;
        if (p.size() >= 3) paintPolygon(canvas, xf.apply(p), palette::building, palette::buildingEdge);
    }

    // 5. Cars: rotated rectangle -> clip -> fill.
    for (const auto& car : s.cars) {
        Polygon rect = raster::rectangle(car.position, car.length, car.width, car.angle);
        Polygon p = win ? clipping::clipPolygon(*win, rect) : rect;
        if (p.size() >= 3) paintPolygon(canvas, xf.apply(p), palette::car, palette::carEdge);
    }

    // 6. Tree canopies last so they overlap buildings/roads realistically.
    for (const auto& t : s.trees) {
        if (win) {
            // Simple inclusion test on the canopy center.
            if (t.position.x < win->xmin || t.position.x > win->xmax ||
                t.position.y < win->ymin || t.position.y > win->ymax)
                continue;
        }
        Point c = xf.apply(t.position);
        int rr = std::max(1, static_cast<int>(t.radius * xf.sx));
        // Offset shadow ellipse, then the canopy disk and a darker rim.
        raster::fillEllipse(canvas, {c.x + rr * 0.35, c.y + rr * 0.35},
                            static_cast<int>(rr * 0.95), static_cast<int>(rr * 0.65),
                            palette::canopyShadow);
        raster::fillCircle(canvas, c, rr, palette::canopy);
        raster::drawCircle(canvas, c, rr, palette::canopyShadow);
    }

    // Outline the zoom window on the full view for clarity.
    if (!win && s.viewport) {
        const Rect& v = *s.viewport;
        Polygon box = {{v.xmin, v.ymin}, {v.xmax, v.ymin}, {v.xmax, v.ymax}, {v.xmin, v.ymax}};
        raster::drawPolygon(canvas, box, palette::viewport);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string scenePath = "assets/campus.scene";
    std::string outPath = "output/campus.ppm";
    std::string zoomPath = "output/campus_zoom.ppm";
    if (argc > 1) scenePath = argv[1];
    if (argc > 2) outPath = argv[2];
    if (argc > 3) zoomPath = argv[3];

    std::string err;
    auto loaded = scene::load(scenePath, &err);
    if (!loaded) {
        std::fprintf(stderr, "scene error: %s\n", err.c_str());
        return 1;
    }
    const scene::Scene& s = *loaded;

    // Full campus view.
    Canvas canvas(s.canvasWidth, s.canvasHeight);
    renderScene(canvas, s, std::nullopt, Transform{});
    if (!canvas.savePPM(outPath)) {
        std::fprintf(stderr, "failed to write %s\n", outPath.c_str());
        return 1;
    }
    std::printf("wrote %s (%dx%d)\n", outPath.c_str(), s.canvasWidth, s.canvasHeight);

    // Clipped zoom view, if a viewport window was declared.
    if (s.viewport) {
        Canvas zoom(s.canvasWidth, s.canvasHeight);
        Transform xf = fitWindow(*s.viewport, s.canvasWidth, s.canvasHeight);
        renderScene(zoom, s, s.viewport, xf);
        if (!zoom.savePPM(zoomPath)) {
            std::fprintf(stderr, "failed to write %s\n", zoomPath.c_str());
            return 1;
        }
        std::printf("wrote %s (clipped zoom)\n", zoomPath.c_str());
    }

    std::printf("done: %zu buildings, %zu trees, %zu cars, %zu roads\n",
                s.buildings.size(), s.trees.size(), s.cars.size(), s.roads.size());
    return 0;
}
