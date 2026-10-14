#pragma once

#include <cstdint>

// 24-bit RGB color used throughout the renderer and stored directly in the
// pixel buffer that gets written out as a PPM (P6) file.
struct Color {
    unsigned char r = 0, g = 0, b = 0;
};

namespace palette {
inline constexpr Color white{255, 255, 255};
inline constexpr Color black{0, 0, 0};
inline constexpr Color ground{45, 70, 35};       // SketchUp dark-green surrounds
inline constexpr Color pad{245, 245, 245};       // white site pad under buildings
inline constexpr Color padEdge{40, 40, 40};      // pad outline
inline constexpr Color lawn{90, 170, 70};        // grass / planter fill
inline constexpr Color road{90, 92, 96};         // asphalt
inline constexpr Color roadEdge{60, 62, 66};     // darker road outline
inline constexpr Color parking{170, 170, 170};   // plaza / parking fill
inline constexpr Color building{210, 175, 140};  // building footprint fill
inline constexpr Color buildingEdge{40, 40, 40}; // building outline
inline constexpr Color canopy{74, 136, 70};      // tree canopy
inline constexpr Color canopyShadow{40, 84, 44}; // offset shadow under canopy
inline constexpr Color car{206, 74, 74};         // car body
inline constexpr Color carEdge{120, 36, 36};     // car outline
inline constexpr Color viewport{40, 90, 180};    // zoom window outline
}  // namespace palette
