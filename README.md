# Campus Renderer

A from-scratch 2D computer-graphics renderer in C++17. It reads a plain-text
**scene file** describing a campus (buildings, roads, trees, cars, parking,
lawns) recorded from a **SketchUp Web** top-down view, and rasterizes it into
its own pixel buffer using classic graphics algorithms — no external graphics
libraries, no imported 3D Warehouse meshes.

The SketchUp model is used only as a **position reference**. Each element is
redrawn in code as a simple 2D symbol, which keeps the C++ project light and
puts the focus on the algorithms (lines, fills, curves, clipping).

## Render output

| SketchUp top view (reference) | C++ render | Clipped zoom window |
| --- | --- | --- |
| ![SketchUp top](docs/sketchup_top.png) | ![campus](docs/campus.png) | ![zoom](docs/campus_zoom.png) |

> **Design decision:** cars and trees are intentionally simplified to 2D
> symbols (rotated rectangles and filled circles). This is a modeling choice,
> not a limitation — it is what lets every element be drawn with our own
> rasterization code.

## Algorithms implemented so far

| Element | Technique | Source |
| --- | --- | --- |
| Building / road / car outlines | Bresenham line (all octants) | `src/raster.cpp` |
| Building / pad / lawn / parking fill | Scanline polygon fill (even-odd) | `src/raster.cpp` |
| Tree canopy | Midpoint circle + offset shadow ellipse | `src/raster.cpp` |
| Car | Rotated-rectangle polygon transform | `src/raster.cpp` |
| Curved road | Cubic Bezier sampling, thickened stroke | `src/raster.cpp` |
| Zoom / line clip | Cohen–Sutherland | `src/clipping.cpp` |
| Zoom / polygon clip | Sutherland–Hodgman | `src/clipping.cpp` |
| Scene input | Plain-text scene loader | `src/scene.cpp` |

## Project layout

```
campus-renderer/
├── include/        # public headers (one per module)
│   ├── color.h     # RGB color + palette
│   ├── geometry.h  # Point, Polygon, Rect, rotation
│   ├── canvas.h    # pixel buffer + PPM writer
│   ├── raster.h    # line, circle, fill, bezier, car
│   ├── clipping.h  # Cohen-Sutherland + Sutherland-Hodgman
│   └── scene.h     # scene model + loader
├── src/            # implementations + main pipeline
├── assets/         # campus.scene (edit this to change the campus)
├── output/         # generated PPM images (gitignored)
├── docs/           # reference / README images
├── CMakeLists.txt
└── Makefile
```

Module ownership is clean per file, so a team can split work (e.g. one person
on `raster`, one on `clipping`, one on `scene`) with independent commits.

## Build & run

With **Make**:

```bash
make          # builds ./campus
make run      # renders assets/campus.scene -> output/*.ppm
```

With **CMake**:

```bash
cmake -S . -B build
cmake --build build
./build/campus assets/campus.scene output/campus.ppm output/campus_zoom.ppm
```

Open `output/campus.ppm` in GIMP, IrfanView, or VS Code (PPM viewer
extension). To export PNGs for the README:

```bash
python3 -c "from PIL import Image; \
  [Image.open(f'output/{n}.ppm').save(f'docs/{n}.png') for n in ['campus','campus_zoom']]"
```

## Rendering pipeline

1. **Parse** the scene file into typed records (`scene::load`).
2. **Build** shape objects (polygons, Bezier control points, rotated rects).
3. **Clip** to the viewport window (Cohen–Sutherland / Sutherland–Hodgman).
4. **Transform** the window onto the canvas for the zoom view.
5. **Rasterize** into the pixel buffer, layering back-to-front:
   ground → lawns → roads → parking → buildings → cars → tree canopies.
6. **Write** the buffer out as a PPM image.

## Scene file format

Coordinates are pixels read off a top-down SketchUp screenshot (origin
top-left, +y downward). Editing the campus is just editing this text file — no
recompile.

```
CANVAS   960 485
VIEWPORT 280 120 560 400                     # optional zoom/clip window
PAD      site_pad  454 34  166 198  200 303  276 425  525 425  525 290
LAWN     planter   222 200  266 295  274 297  277 290  230 202
PARKING  plaza     166 197  202 307  258 398  297 425  401 425
BUILDING e_bldg    380 139  348 154  ...      # E footprint (see campus.scene)
ROAD     access    200 210 260 280 320 360 390 420  10
TREE     t1        200 160 12
CAR      c1        250 340 28 12 -28
```

`assets/campus.scene` was digitized from `docs/sketchup_top.png` (half-scale
pixel coordinates). Lines beginning with `#`, and inline `# ...` comments, are
ignored.

## Getting data out of SketchUp Web

1. Hide cars and trees.
2. Take a **top-down, parallel-projection** screenshot of the full campus;
   save it as `docs/sketchup_top.png` and treat its pixel grid as your
   coordinate system.
3. Read building corner and road pixel positions off the screenshot and write
   them into `assets/campus.scene`.
4. Re-show cars/trees, read their pixel positions, and add `TREE` / `CAR`
   lines.
5. Run `make run` and compare side by side.

## Git workflow — pushing your progress

The repo is already initialized. Typical loop while building a module:

```bash
# 1. make a focused change (e.g. implement scanline fill)
git add src/raster.cpp include/raster.h
git commit -m "raster: add scanline polygon fill"

# 2. push to your branch
git push -u origin main
```

Suggested conventions for the team:

- **One module per commit.** Prefix messages by area: `raster:`, `clipping:`,
  `scene:`, `docs:`.
- **Each member owns a module** (lines/circles, polygons/fill, clipping,
  Bezier/scene loader) so commits don't collide.
- **Branch per feature** when work overlaps:
  ```bash
  git checkout -b feature/bezier-road
  # ... work, commit ...
  git push -u origin feature/bezier-road
  # open a Pull Request, review, merge into main
  ```
- **Update this README after each milestone**: add a row to “Algorithms
  implemented so far” and refresh the output image.
- **Don't commit generated images** (`output/*.ppm` is gitignored); commit
  curated PNGs under `docs/` for the README instead.

## Checkpoint demo checklist

- [x] Buildings drawn and filled from scene data
- [x] Trees via midpoint-circle code
- [x] At least one curved road via Bezier curve
- [x] A clipped zoom window showing part of the campus
- [x] SketchUp top view added next to the C++ render in this README
