#include "scene.h"

#include <fstream>
#include <sstream>

namespace scene {

namespace {

// Strip an inline "# ..." comment and return the remaining content.
std::string stripComment(const std::string& line) {
    size_t hash = line.find('#');
    return hash == std::string::npos ? line : line.substr(0, hash);
}

// Read a flat list of doubles into a polygon of (x, y) pairs.
Polygon toPolygon(const std::vector<double>& nums) {
    Polygon poly;
    for (size_t i = 0; i + 1 < nums.size(); i += 2) {
        poly.push_back({nums[i], nums[i + 1]});
    }
    return poly;
}

}  // namespace

std::optional<Scene> load(const std::string& path, std::string* error) {
    std::ifstream f(path);
    if (!f) {
        if (error) *error = "cannot open scene file: " + path;
        return std::nullopt;
    }

    Scene scene;
    std::string raw;
    int lineNo = 0;

    auto fail = [&](const std::string& msg) -> std::optional<Scene> {
        if (error) *error = "line " + std::to_string(lineNo) + ": " + msg;
        return std::nullopt;
    };

    while (std::getline(f, raw)) {
        ++lineNo;
        std::istringstream in(stripComment(raw));
        std::string type;
        if (!(in >> type)) continue;  // blank or comment-only line

        if (type == "CANVAS") {
            if (!(in >> scene.canvasWidth >> scene.canvasHeight))
                return fail("CANVAS expects: width height");
        } else if (type == "VIEWPORT") {
            Rect r{};
            if (!(in >> r.xmin >> r.ymin >> r.xmax >> r.ymax))
                return fail("VIEWPORT expects: xmin ymin xmax ymax");
            scene.viewport = r;
        } else if (type == "BUILDING" || type == "LAWN" || type == "PARKING" ||
                   type == "PAD") {
            std::string name;
            if (!(in >> name)) return fail(type + " expects a name");
            std::vector<double> nums;
            double v;
            while (in >> v) nums.push_back(v);
            if (nums.size() < 6 || nums.size() % 2 != 0)
                return fail(type + " needs >= 3 (x y) coordinate pairs");
            Polygon poly = toPolygon(nums);
            if (type == "BUILDING") scene.buildings.push_back({name, poly});
            else if (type == "LAWN") scene.lawns.push_back({name, poly});
            else if (type == "PAD") scene.pads.push_back({name, poly});
            else scene.parking.push_back({name, poly});
        } else if (type == "TREE") {
            Tree t;
            if (!(in >> t.name >> t.position.x >> t.position.y >> t.radius))
                return fail("TREE expects: name x y radius");
            scene.trees.push_back(t);
        } else if (type == "CAR") {
            Car c;
            if (!(in >> c.name >> c.position.x >> c.position.y >> c.length >>
                  c.width >> c.angle))
                return fail("CAR expects: name x y length width angle");
            scene.cars.push_back(c);
        } else if (type == "ROAD") {
            Road r;
            if (!(in >> r.name >> r.p0.x >> r.p0.y >> r.p1.x >> r.p1.y >>
                  r.p2.x >> r.p2.y >> r.p3.x >> r.p3.y))
                return fail("ROAD expects: name x0 y0 x1 y1 x2 y2 x3 y3 [thickness]");
            double thickness;
            if (in >> thickness) r.thickness = thickness;
            scene.roads.push_back(r);
        } else {
            return fail("unknown record type: " + type);
        }
    }

    return scene;
}

}  // namespace scene
