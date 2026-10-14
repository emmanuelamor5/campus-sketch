#pragma once

#include <optional>
#include <string>
#include <vector>

#include "geometry.h"

// In-memory representation of a campus scene parsed from a plain-text .scene
// file. Each record mirrors the way an element was recorded off a top-down
// SketchUp screenshot (pixel coordinates), and maps to one drawing technique.
namespace scene {

struct Building {
    std::string name;
    Polygon footprint;  // >= 3 vertices
};

struct Pad {
    std::string name;
    Polygon outline;  // white site pad under the building
};

struct Lawn {
    std::string name;
    Polygon outline;
};

struct ParkingLot {
    std::string name;
    Polygon outline;
};

struct Tree {
    std::string name;
    Point position;
    int radius = 10;
};

struct Car {
    std::string name;
    Point position;
    double length = 30;
    double width = 14;
    double angle = 0;  // degrees
};

struct Road {
    std::string name;
    Point p0, p1, p2, p3;  // cubic Bezier control points
    double thickness = 10;
};

struct Scene {
    int canvasWidth = 800;
    int canvasHeight = 600;
    std::optional<Rect> viewport;  // zoom/clip window, if declared

    std::vector<Pad> pads;
    std::vector<Lawn> lawns;
    std::vector<Road> roads;
    std::vector<ParkingLot> parking;
    std::vector<Building> buildings;
    std::vector<Car> cars;
    std::vector<Tree> trees;
};

// Parse a .scene file. On a syntax error, writes a message to `error` (if
// non-null) and returns std::nullopt.
std::optional<Scene> load(const std::string& path, std::string* error = nullptr);

}  // namespace scene
