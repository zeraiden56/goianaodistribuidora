#pragma once

#include <array>
#include <cstddef>
#include <string>

// =============================================================================
// SHOP GLOBAL LAYOUT
// =============================================================================

// Saved positions continue using logical shop coordinates.
// Rendering converts them to the larger physical world through these scales.
inline constexpr float shopScaleX = 1.30f;
inline constexpr float shopScaleZ = 1.50f;

inline constexpr int maxCheckoutCount = 5;
inline constexpr int layoutProductCount = 6;
inline constexpr int interactionStationCount = 21;

// =============================================================================
// BASIC TYPES
// =============================================================================

struct StockLocation {
    float x;
    float z;

    // Preferred position used by workers when accessing this stock.
    float workerX;
    float workerZ;

    // Interaction radius in logical shop units.
    float reach;
};

struct TerminalLocation {
    float x;
    float y;
    float z;

    float scale;

    // Associated interaction station.
    int station;
};

struct LayoutRect {
    float minX;
    float maxX;

    float minZ;
    float maxZ;
};

// =============================================================================
// STATION IDS
//
// Keep IDs compatible with existing save/game/input logic.
//
// Naming them here removes most of the mysterious raw integers from the game.
// =============================================================================

namespace StationId {

inline constexpr int FrontStock0 = 0;
inline constexpr int FrontStock1 = 1;
inline constexpr int FrontStock2 = 2;
inline constexpr int FrontStock3 = 3;

inline constexpr int FrontComputer = 4;

inline constexpr int Checkout0 = 5;

inline constexpr int Sofa = 6;
inline constexpr int Television = 7;

inline constexpr int DepotStock0 = 8;
inline constexpr int DepotStock1 = 9;
inline constexpr int DepotStock2 = 10;
inline constexpr int DepotStock3 = 11;

inline constexpr int Checkout1 = 12;

inline constexpr int FrontStock4 = 13;
inline constexpr int FrontStock5 = 14;

inline constexpr int Checkout2 = 15;

inline constexpr int BackComputer = 16;

inline constexpr int DepotStock4 = 17;
inline constexpr int DepotStock5 = 18;

inline constexpr int Checkout3 = 19;
inline constexpr int Checkout4 = 20;

} // namespace StationId

// =============================================================================
// STATION ACCESS REQUIREMENTS
// =============================================================================

enum class StationGate {
    Always,

    Expanded,
    SecondCheckout,

    LargeStore,
    ThirdCheckout,

    AnnexCheckout1,
    AnnexCheckout2
};

// =============================================================================
// INTERACTION STATION
//
// product:
//     -1 = station is not product storage.
//
// checkout:
//     -1 = station is not a checkout.
//
// This lets player.cpp stop maintaining a second copy of the shop layout.
// =============================================================================

struct InteractionStation {
    float x;
    float z;
    float reach;

    int id;

    int product;
    int checkout;

    StationGate gate;
};

// =============================================================================
// CHECKOUT POSITIONS
// =============================================================================

inline constexpr std::array<float, maxCheckoutCount> checkoutPositions{{
    -2.10f,
     2.10f,
     0.00f,
     5.90f,
     8.00f
}};

inline constexpr std::array<int, maxCheckoutCount> checkoutStations{{
    StationId::Checkout0,
    StationId::Checkout1,
    StationId::Checkout2,
    StationId::Checkout3,
    StationId::Checkout4
}};

// =============================================================================
// PRODUCT STOCK LOCATIONS
//
// Index directly corresponds to product ID.
//
// 0..3 = original store
// 4..5 = large-store annex
// =============================================================================

inline constexpr std::array<
    StockLocation,
    layoutProductCount
> stockLocations{{
    // x      z       worker x   worker z   reach
    {-3.45f,  2.85f, -3.45f,     2.20f,    1.55f},
    { 0.65f, -2.25f,  0.65f,    -1.45f,    1.35f},
    {-0.65f,  2.85f, -0.65f,     2.20f,    1.55f},
    {-3.70f,  0.40f, -2.20f,     0.40f,    1.80f},

    { 5.90f,  2.85f,  5.90f,     2.20f,    1.55f},
    { 8.00f,  2.85f,  8.00f,     2.20f,    1.55f}
}};

// =============================================================================
// TERMINALS
//
// Screens face -Z, toward the service area.
// Used both for physical rendering and camera zoom.
// =============================================================================

inline constexpr std::array<TerminalLocation, 2> terminals{{
    {
        3.60f,
        1.62f,
        1.78f,
        1.00f,
        StationId::FrontComputer
    },

    {
        2.50f,
        1.31f,
        7.425f,
        0.78f,
        StationId::BackComputer
    }
}};

// =============================================================================
// INTERACTION STATIONS
//
// This replaces the hard-coded Station[] array currently inside player.cpp.
// =============================================================================

inline constexpr std::array<
    InteractionStation,
    interactionStationCount
> interactionStations{{
    // -------------------------------------------------------------------------
    // Original products
    // -------------------------------------------------------------------------

    {
        stockLocations[0].x,
        stockLocations[0].z,
        stockLocations[0].reach,

        StationId::FrontStock0,
        0,
        -1,
        StationGate::Always
    },

    {
        stockLocations[1].x,
        stockLocations[1].z,
        stockLocations[1].reach,

        StationId::FrontStock1,
        1,
        -1,
        StationGate::Always
    },

    {
        stockLocations[2].x,
        stockLocations[2].z,
        stockLocations[2].reach,

        StationId::FrontStock2,
        2,
        -1,
        StationGate::Always
    },

    {
        stockLocations[3].x,
        stockLocations[3].z,
        stockLocations[3].reach,

        StationId::FrontStock3,
        3,
        -1,
        StationGate::Always
    },

    // -------------------------------------------------------------------------
    // Front computer
    // -------------------------------------------------------------------------

    {
        3.60f,
        1.80f,
        1.90f,

        StationId::FrontComputer,
        -1,
        -1,
        StationGate::Always
    },

    // -------------------------------------------------------------------------
    // Checkout 1
    // -------------------------------------------------------------------------

    {
        checkoutPositions[0],
        -2.60f,
        1.80f,

        StationId::Checkout0,
        -1,
        0,
        StationGate::Always
    },

    // -------------------------------------------------------------------------
    // Rest area
    // -------------------------------------------------------------------------

    {
        3.20f,
        6.20f,
        1.65f,

        StationId::Sofa,
        -1,
        -1,
        StationGate::Expanded
    },

    {
        -4.70f,
        6.20f,
        1.80f,

        StationId::Television,
        -1,
        -1,
        StationGate::Expanded
    },

    // -------------------------------------------------------------------------
    // Original depot shelves
    // -------------------------------------------------------------------------

    {
        -3.60f,
        8.70f,
        1.50f,

        StationId::DepotStock0,
        0,
        -1,
        StationGate::Expanded
    },

    {
        -1.50f,
        8.70f,
        1.50f,

        StationId::DepotStock1,
        1,
        -1,
        StationGate::Expanded
    },

    {
        0.60f,
        8.70f,
        1.50f,

        StationId::DepotStock2,
        2,
        -1,
        StationGate::Expanded
    },

    {
        2.70f,
        8.70f,
        1.50f,

        StationId::DepotStock3,
        3,
        -1,
        StationGate::Expanded
    },

    // -------------------------------------------------------------------------
    // Checkout 2
    // -------------------------------------------------------------------------

    {
        checkoutPositions[1],
        -2.60f,
        1.80f,

        StationId::Checkout1,
        -1,
        1,
        StationGate::SecondCheckout
    },

    // -------------------------------------------------------------------------
    // New refrigerators
    // -------------------------------------------------------------------------

    {
        stockLocations[4].x,
        stockLocations[4].z,
        stockLocations[4].reach,

        StationId::FrontStock4,
        4,
        -1,
        StationGate::LargeStore
    },

    {
        stockLocations[5].x,
        stockLocations[5].z,
        stockLocations[5].reach,

        StationId::FrontStock5,
        5,
        -1,
        StationGate::LargeStore
    },

    // -------------------------------------------------------------------------
    // Checkout 3
    // -------------------------------------------------------------------------

    {
        checkoutPositions[2],
        -2.80f,
        1.80f,

        StationId::Checkout2,
        -1,
        2,
        StationGate::ThirdCheckout
    },

    // -------------------------------------------------------------------------
    // Back-room computer
    // -------------------------------------------------------------------------

    {
        2.50f,
        7.35f,
        1.80f,

        StationId::BackComputer,
        -1,
        -1,
        StationGate::Expanded
    },

    // -------------------------------------------------------------------------
    // Annex depot shelves
    // -------------------------------------------------------------------------

    {
        5.90f,
        8.70f,
        1.50f,

        StationId::DepotStock4,
        4,
        -1,
        StationGate::LargeStore
    },

    {
        8.00f,
        8.70f,
        1.50f,

        StationId::DepotStock5,
        5,
        -1,
        StationGate::LargeStore
    },

    // -------------------------------------------------------------------------
    // Annex checkout 1
    // -------------------------------------------------------------------------

    {
        checkoutPositions[3],
        -2.60f,
        1.80f,

        StationId::Checkout3,
        -1,
        3,
        StationGate::AnnexCheckout1
    },

    // -------------------------------------------------------------------------
    // Annex checkout 2
    // -------------------------------------------------------------------------

    {
        checkoutPositions[4],
        -2.60f,
        1.80f,

        StationId::Checkout4,
        -1,
        4,
        StationGate::AnnexCheckout2
    }
}};

// =============================================================================
// IMPORTANT FURNITURE POSITIONS
//
// The renderer, collision system and player interactions can now share these.
// =============================================================================

namespace ShopPosition {

inline constexpr float SofaX = 3.20f;
inline constexpr float SofaZ = 6.20f;

inline constexpr float SofaSeatX = 3.80f;
inline constexpr float SofaSeatZ = 6.20f;

inline constexpr float SofaExitX = 2.65f;
inline constexpr float SofaExitZ = 6.20f;

inline constexpr float TelevisionX = -4.70f;
inline constexpr float TelevisionZ = 6.20f;

inline constexpr float FrontComputerX = 3.60f;
inline constexpr float FrontComputerZ = 1.80f;

inline constexpr float BackComputerX = 2.50f;
inline constexpr float BackComputerZ = 7.35f;

} // namespace ShopPosition

// =============================================================================
// WALKABLE MAP LIMITS
// =============================================================================

namespace ShopBounds {

inline constexpr float MinX = -4.65f;

inline constexpr float BaseMaxX = 4.65f;
inline constexpr float LargeMaxX = 8.65f;

inline constexpr float FrontMinZ = -1.78f;
inline constexpr float BackMaxZ = 9.65f;

// Main furniture / obstruction rectangles.
inline constexpr LayoutRect ComputerDesk{
    2.48f,
    4.60f,
    1.00f,
    2.60f
};

inline constexpr LayoutRect Freezer{
    -4.65f,
    -2.65f,
    -0.60f,
    1.40f
};

inline constexpr LayoutRect FrontPartition{
    -4.65f,
    1.80f,
    2.50f,
    4.25f
};

inline constexpr LayoutRect Sofa{
    2.95f,
    4.85f,
    4.80f,
    7.65f
};

inline constexpr LayoutRect SofaTable{
    1.85f,
    2.95f,
    7.05f,
    7.85f
};

inline constexpr LayoutRect DepotRacks{
    -4.65f,
    3.85f,
    8.45f,
    9.65f
};

inline constexpr LayoutRect AnnexFrontPartition{
    4.65f,
    8.65f,
    2.50f,
    4.25f
};

} // namespace ShopBounds

// =============================================================================
// RECT HELPERS
// =============================================================================

inline constexpr bool insideLayoutRect(
    float x,
    float z,
    const LayoutRect& rect) noexcept
{
    return
        x > rect.minX &&
        x < rect.maxX &&
        z > rect.minZ &&
        z < rect.maxZ;
}

// =============================================================================
// STATION LOOKUP
// =============================================================================

inline constexpr const InteractionStation*
stationInfo(int station) noexcept
{
    for (const auto& entry : interactionStations) {
        if (entry.id == station)
            return &entry;
    }

    return nullptr;
}

// =============================================================================
// STATION AVAILABILITY
// =============================================================================

inline constexpr bool stationUnlocked(
    const InteractionStation& station,
    bool expanded,
    bool secondCheckout,
    bool largeStore,
    bool thirdCheckout,
    int annexCheckouts) noexcept
{
    switch (station.gate) {

        case StationGate::Always:
            return true;

        case StationGate::Expanded:
            return expanded;

        case StationGate::SecondCheckout:
            return secondCheckout;

        case StationGate::LargeStore:
            return largeStore;

        case StationGate::ThirdCheckout:
            return thirdCheckout;

        case StationGate::AnnexCheckout1:
            return
                largeStore &&
                annexCheckouts >= 1;

        case StationGate::AnnexCheckout2:
            return
                largeStore &&
                annexCheckouts >= 2;
    }

    return false;
}

// =============================================================================
// PRODUCT FOR STATION
// =============================================================================

inline constexpr int stationProduct(
    int station) noexcept
{
    const auto* info =
        stationInfo(station);

    return info
        ? info->product
        : -1;
}

// =============================================================================
// CHECKOUT FOR STATION
// =============================================================================

inline constexpr int stationCheckout(
    int station) noexcept
{
    const auto* info =
        stationInfo(station);

    return info
        ? info->checkout
        : -1;
}

// =============================================================================
// CHECKOUT X
// =============================================================================

inline constexpr float checkoutX(
    int lane) noexcept
{
    if (
        lane < 0 ||
        lane >=
            static_cast<int>(
                checkoutPositions.size()
            )
    ) {
        return 0.0f;
    }

    return checkoutPositions[
        static_cast<std::size_t>(lane)
    ];
}

// =============================================================================
// CHECKOUT STATION
// =============================================================================

inline constexpr int checkoutStation(
    int lane) noexcept
{
    if (
        lane < 0 ||
        lane >=
            static_cast<int>(
                checkoutStations.size()
            )
    ) {
        return -1;
    }

    return checkoutStations[
        static_cast<std::size_t>(lane)
    ];
}

// =============================================================================
// CARRIED ITEM LABEL
// =============================================================================

inline std::string carriedLabel(
    int units,
    bool packed)
{
    units =
        units < 0
            ? 0
            : units;

    if (!packed)
        return
            std::to_string(units) +
            " UN.";

    constexpr int unitsPerCrate = 12;

    const int crates =
        units / unitsPerCrate;

    const int loose =
        units % unitsPerCrate;

    if (crates <= 0)
        return
            std::to_string(loose) +
            " UN.";

    std::string result =
        std::to_string(crates) +
        " CX";

    if (loose > 0) {
        result +=
            " + " +
            std::to_string(loose) +
            " UN.";
    }

    return result;
}