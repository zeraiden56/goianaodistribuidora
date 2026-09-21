#include "player.hpp"
#include "shop_layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr float PI =
    3.14159265358979323846f;

constexpr float TWO_PI =
    PI * 2.0f;

constexpr float HALF_PI =
    PI * 0.5f;

constexpr float EPSILON =
    0.0001f;

// Player movement.
constexpr float WALK_SPEED   = 2.70f;
constexpr float SPRINT_SPEED = 4.80f;

// Maximum amount of frame time used for movement.
// Prevents huge teleports after a freeze / breakpoint.
constexpr float MAX_MOVEMENT_DT =
    0.10f;

// Movement is split into small pieces so thin obstacles
// cannot be skipped at low FPS.
constexpr float MAX_COLLISION_STEP =
    0.12f;

// Physical radius around the player's center.
constexpr float PLAYER_RADIUS =
    0.16f;

// Interaction.
constexpr float MIN_INTERACTION_ALIGNMENT =
    0.58f;

constexpr float INTERACTION_DISTANCE_WEIGHT =
    0.16f;

// Seat.
constexpr float SEAT_X =
    3.80f;

constexpr float SEAT_Z =
    6.20f;

constexpr float SEAT_EXIT_X =
    2.65f;

constexpr float SEAT_EXIT_Z =
    6.20f;

constexpr float SEAT_INTERACTION_X =
    3.20f;

constexpr float SEAT_INTERACTION_Z =
    6.20f;

constexpr float SEAT_REACH =
    1.65f;

// =============================================================================
// Station
// =============================================================================

struct Station {
    float x;
    float z;
    float reach;
    int id;
};

// =============================================================================
// Helpers
// =============================================================================

float safeScale(float scale)
{
    if (!std::isfinite(scale))
        return 1.0f;

    if (std::abs(scale) < EPSILON)
        return 1.0f;

    return std::abs(scale);
}

bool insideOpenRect(
    float x,
    float z,
    float minX,
    float maxX,
    float minZ,
    float maxZ)
{
    return
        x > minX &&
        x < maxX &&
        z > minZ &&
        z < maxZ;
}

bool stationAvailable(
    int id,
    bool expanded,
    bool secondCheckout,
    bool largeStore,
    bool thirdCheckout,
    int annexCheckouts)
{
    // Everything from ID 6 onward belongs to the expanded shop.
    if (id >= 6 && !expanded)
        return false;

    // Second checkout.
    if (id == 12 && !secondCheckout)
        return false;

    // Large-store-only stations.
    if (
        id == 13 ||
        id == 14 ||
        id >= 17
    ) {
        if (!largeStore)
            return false;
    }

    // Third checkout.
    if (id == 15 && !thirdCheckout)
        return false;

    // Annex checkout stations:
    //
    // 19 -> requires annexCheckouts >= 1
    // 20 -> requires annexCheckouts >= 2
    if (
        id >= 19 &&
        annexCheckouts < id - 18
    ) {
        return false;
    }

    return true;
}

// =============================================================================
// Player collision volume
//
// walkable() remains a point test because other parts of the project may use
// it. Movement uses this helper to give the player an actual radius.
// =============================================================================

bool walkableWithRadius(
    float x,
    float z,
    bool expanded,
    bool largeStore)
{
    if (!walkable(
            x,
            z,
            expanded,
            largeStore)) {
        return false;
    }

    const float scaleX =
        safeScale(shopScaleX);

    const float scaleZ =
        safeScale(shopScaleZ);

    // Convert world-space radius back into shop coordinates.
    const float rx =
        PLAYER_RADIUS / scaleX;

    const float rz =
        PLAYER_RADIUS / scaleZ;

    // Axis points.
    if (!walkable(
            x + rx,
            z,
            expanded,
            largeStore)) {
        return false;
    }

    if (!walkable(
            x - rx,
            z,
            expanded,
            largeStore)) {
        return false;
    }

    if (!walkable(
            x,
            z + rz,
            expanded,
            largeStore)) {
        return false;
    }

    if (!walkable(
            x,
            z - rz,
            expanded,
            largeStore)) {
        return false;
    }

    // Circular approximation for corners.
    constexpr float diagonal =
        0.70710678f;

    const float cx =
        rx * diagonal;

    const float cz =
        rz * diagonal;

    if (!walkable(
            x + cx,
            z + cz,
            expanded,
            largeStore)) {
        return false;
    }

    if (!walkable(
            x + cx,
            z - cz,
            expanded,
            largeStore)) {
        return false;
    }

    if (!walkable(
            x - cx,
            z + cz,
            expanded,
            largeStore)) {
        return false;
    }

    if (!walkable(
            x - cx,
            z - cz,
            expanded,
            largeStore)) {
        return false;
    }

    return true;
}

// =============================================================================
// Try moving one collision sub-step.
//
// First attempts the diagonal movement normally.
// If that fails, tries each axis independently, producing wall sliding.
// =============================================================================

void moveCollisionStep(
    Player& p,
    float dx,
    float dz,
    bool expanded,
    bool largeStore)
{
    const float targetX =
        p.x + dx;

    const float targetZ =
        p.z + dz;

    // -------------------------------------------------------------------------
    // Try complete movement first.
    // -------------------------------------------------------------------------

    if (walkableWithRadius(
            targetX,
            targetZ,
            expanded,
            largeStore)) {

        p.x = targetX;
        p.z = targetZ;

        return;
    }

    // -------------------------------------------------------------------------
    // Collision: slide along obstacle.
    //
    // Test the dominant movement axis first. This reduces the tiny directional
    // bias caused by always testing X before Z.
    // -------------------------------------------------------------------------

    const float worldDx =
        std::abs(
            dx * safeScale(shopScaleX)
        );

    const float worldDz =
        std::abs(
            dz * safeScale(shopScaleZ)
        );

    if (worldDx >= worldDz) {

        if (walkableWithRadius(
                p.x + dx,
                p.z,
                expanded,
                largeStore)) {

            p.x += dx;
        }

        if (walkableWithRadius(
                p.x,
                p.z + dz,
                expanded,
                largeStore)) {

            p.z += dz;
        }
    }
    else {

        if (walkableWithRadius(
                p.x,
                p.z + dz,
                expanded,
                largeStore)) {

            p.z += dz;
        }

        if (walkableWithRadius(
                p.x + dx,
                p.z,
                expanded,
                largeStore)) {

            p.x += dx;
        }
    }
}

} // namespace

// =============================================================================
// INTERACTION
//
// Finds the station that:
//   1. is currently available;
//   2. is close enough;
//   3. is reasonably close to the player's view direction.
//
// Distance now participates in target selection, preventing a farther object
// from stealing focus simply because it is a few degrees more centered.
// =============================================================================

int interaction(
    float px,
    float pz,
    float yaw,
    bool expanded,
    bool secondCheckout,
    bool largeStore,
    bool thirdCheckout,
    int annexCheckouts)
{
    if (
        !std::isfinite(px) ||
        !std::isfinite(pz) ||
        !std::isfinite(yaw)
    ) {
        return -1;
    }

    const std::array<Station, 21> stations{{
        {
            stockLocations[0].x,
            stockLocations[0].z,
            stockLocations[0].reach,
            0
        },

        {
            stockLocations[1].x,
            stockLocations[1].z,
            stockLocations[1].reach,
            1
        },

        {
            stockLocations[2].x,
            stockLocations[2].z,
            stockLocations[2].reach,
            2
        },

        {
            stockLocations[3].x,
            stockLocations[3].z,
            stockLocations[3].reach,
            3
        },

        { 3.60f,  1.80f, 1.90f,  4 },
        {-2.10f, -2.60f, 1.80f,  5 },

        { 3.20f,  6.20f, 1.65f,  6 },
        {-4.70f,  6.20f, 1.80f,  7 },

        {-3.60f,  8.70f, 1.50f,  8 },
        {-1.50f,  8.70f, 1.50f,  9 },
        { 0.60f,  8.70f, 1.50f, 10 },
        { 2.70f,  8.70f, 1.50f, 11 },

        { 2.10f, -2.60f, 1.80f, 12 },

        { 5.90f,  2.85f, 1.55f, 13 },
        { 8.00f,  2.85f, 1.55f, 14 },

        { 0.00f, -2.80f, 1.80f, 15 },

        { 2.50f,  7.35f, 1.80f, 16 },

        { 5.90f,  8.70f, 1.50f, 17 },
        { 8.00f,  8.70f, 1.50f, 18 },

        { 5.90f, -2.60f, 1.80f, 19 },
        { 8.00f, -2.60f, 1.80f, 20 }
    }};

    const float scaleX =
        safeScale(shopScaleX);

    const float scaleZ =
        safeScale(shopScaleZ);

    // Geometric average keeps reach sensible if X/Z scales differ.
    const float reachScale =
        std::sqrt(
            scaleX * scaleZ
        );

    // Camera forward direction.
    const float forwardX =
        std::sin(yaw);

    const float forwardZ =
        -std::cos(yaw);

    int target = -1;

    float bestScore =
        -2.0f;

    float bestDistance =
        1000000.0f;

    annexCheckouts =
        std::max(
            0,
            annexCheckouts
        );

    for (const Station& station : stations) {

        if (!stationAvailable(
                station.id,
                expanded,
                secondCheckout,
                largeStore,
                thirdCheckout,
                annexCheckouts)) {
            continue;
        }

        const float dx =
            (station.x - px) *
            scaleX;

        const float dz =
            (station.z - pz) *
            scaleZ;

        const float distance =
            std::hypot(
                dx,
                dz
            );

        if (distance < 0.01f)
            continue;

        const float reach =
            station.reach *
            reachScale;

        if (
            reach <= EPSILON ||
            distance > reach
        ) {
            continue;
        }

        // Dot product between camera forward vector and station direction.
        const float alignment =
            (
                dx * forwardX +
                dz * forwardZ
            ) /
            distance;

        if (
            alignment <
            MIN_INTERACTION_ALIGNMENT
        ) {
            continue;
        }

        const float distanceRatio =
            std::clamp(
                distance / reach,
                0.0f,
                1.0f
            );

        // Looking direction remains dominant, but nearby objects receive
        // a modest preference.
        const float score =
            alignment -
            distanceRatio *
            INTERACTION_DISTANCE_WEIGHT;

        if (
            score > bestScore + 0.0001f ||
            (
                std::abs(
                    score - bestScore
                ) <= 0.0001f &&
                distance < bestDistance
            )
        ) {
            bestScore =
                score;

            bestDistance =
                distance;

            target =
                station.id;
        }
    }

    return target;
}

// =============================================================================
// WALKABLE
//
// This intentionally remains a point-based query.
// Player collision radius is handled by walkableWithRadius().
// =============================================================================

bool walkable(
    float x,
    float z,
    bool expanded,
    bool largeStore)
{
    if (
        !std::isfinite(x) ||
        !std::isfinite(z)
    ) {
        return false;
    }

    // =========================================================================
    // Outer shop boundaries
    // =========================================================================

    const float maxX =
        largeStore
            ? 8.65f
            : 4.65f;

    if (
        x <= -4.65f ||
        x >= maxX ||
        z <= -1.78f
    ) {
        return false;
    }

    // =========================================================================
    // Large-store annex
    // =========================================================================

    if (
        largeStore &&
        x > 4.65f
    ) {
        if (z >= 9.65f)
            return false;

        // New refrigerators / partition.
        if (
            z > 2.50f &&
            z < 4.25f
        ) {
            return false;
        }

        // New depot racks.
        if (z > 8.45f)
            return false;

        return true;
    }

    // Partition between main store and annex.
    if (
        largeStore &&
        insideOpenRect(
            x,
            z,
            4.35f,
            100.0f,
            2.50f,
            4.35f
        )
    ) {
        return false;
    }

    // =========================================================================
    // Initial shop
    // =========================================================================

    if (!expanded) {

        if (z >= 2.50f)
            return false;

        // Computer desk.
        if (
            x > 2.48f &&
            z > 1.00f
        ) {
            return false;
        }

        // Freezer.
        if (
            x < -2.65f &&
            z > -0.60f &&
            z < 1.40f
        ) {
            return false;
        }

        return true;
    }

    // =========================================================================
    // Expanded shop
    // =========================================================================

    if (z >= 9.65f)
        return false;

    // -------------------------------------------------------------------------
    // Computer desk
    // -------------------------------------------------------------------------

    if (
        insideOpenRect(
            x,
            z,
            2.48f,
            4.60f,
            1.00f,
            2.60f
        )
    ) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Freezer
    // -------------------------------------------------------------------------

    if (
        x < -2.65f &&
        z > -0.60f &&
        z < 1.40f
    ) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Front shelving / partition
    // -------------------------------------------------------------------------

    if (
        x < 1.80f &&
        z > 2.50f &&
        z < 4.25f
    ) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Back-room wall.
    //
    // Open doorway between X = 1.9 and X = 3.85.
    // -------------------------------------------------------------------------

    if (
        z > 3.65f &&
        z < 4.35f &&
        (
            x < 1.90f ||
            x > 3.85f
        )
    ) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Sofa
    // -------------------------------------------------------------------------

    if (
        insideOpenRect(
            x,
            z,
            2.95f,
            4.85f,
            4.80f,
            7.65f
        )
    ) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Depot racks
    // -------------------------------------------------------------------------

    if (
        z > 8.45f &&
        x < 3.85f
    ) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Sofa computer table
    // -------------------------------------------------------------------------

    if (
        insideOpenRect(
            x,
            z,
            1.85f,
            2.95f,
            7.05f,
            7.85f
        )
    ) {
        return false;
    }

    return true;
}

// =============================================================================
// MOVEMENT SPEED
// =============================================================================

float movementSpeed(
    bool sprint,
    float intoxication)
{
    if (!std::isfinite(intoxication))
        intoxication = 0.0f;

    intoxication =
        std::clamp(
            intoxication,
            0.0f,
            100.0f
        );

    const float baseSpeed =
        sprint
            ? SPRINT_SPEED
            : WALK_SPEED;

    const float modifier =
        1.0f -
        intoxication *
        0.003f;

    return
        baseSpeed *
        modifier;
}

// =============================================================================
// MOVE PLAYER
// =============================================================================

float movePlayer(
    Player& p,
    float forward,
    float strafe,
    bool sprint,
    float intoxication,
    float dt,
    bool expanded,
    bool largeStore)
{
    if (p.seated)
        return 0.0f;

    if (
        !std::isfinite(p.x) ||
        !std::isfinite(p.z) ||
        !std::isfinite(p.yaw)
    ) {
        return 0.0f;
    }

    if (
        !std::isfinite(forward) ||
        !std::isfinite(strafe) ||
        !std::isfinite(dt)
    ) {
        return 0.0f;
    }

    const float inputMagnitude =
        std::hypot(
            forward,
            strafe
        );

    if (inputMagnitude < EPSILON)
        return 0.0f;

    // Prevent diagonal input from moving faster.
    const float normalization =
        std::max(
            1.0f,
            inputMagnitude
        );

    forward /=
        normalization;

    strafe /=
        normalization;

    dt =
        std::clamp(
            dt,
            0.0f,
            MAX_MOVEMENT_DT
        );

    if (dt <= 0.0f)
        return 0.0f;

    const float speed =
        movementSpeed(
            sprint,
            intoxication
        );

    const float distanceForFrame =
        speed * dt;

    // Camera-relative movement in world coordinates.
    const float worldDX =
        (
            std::sin(p.yaw) *
                forward +

            std::cos(p.yaw) *
                strafe
        ) *
        distanceForFrame;

    const float worldDZ =
        (
            -std::cos(p.yaw) *
                forward +

            std::sin(p.yaw) *
                strafe
        ) *
        distanceForFrame;

    const float scaleX =
        safeScale(shopScaleX);

    const float scaleZ =
        safeScale(shopScaleZ);

    // Shop coordinates.
    const float totalDX =
        worldDX /
        scaleX;

    const float totalDZ =
        worldDZ /
        scaleZ;

    const float intendedWorldDistance =
        std::hypot(
            worldDX,
            worldDZ
        );

    // -------------------------------------------------------------------------
    // Collision substeps
    //
    // At a low frame rate, instead of asking:
    //
    //     can I move directly from A to B?
    //
    // we test:
    //
    //     A -> a -> b -> c -> B
    //
    // preventing passage through narrow objects.
    // -------------------------------------------------------------------------

    int steps =
        static_cast<int>(
            std::ceil(
                intendedWorldDistance /
                MAX_COLLISION_STEP
            )
        );

    steps =
        std::clamp(
            steps,
            1,
            32
        );

    const float stepDX =
        totalDX /
        static_cast<float>(
            steps
        );

    const float stepDZ =
        totalDZ /
        static_cast<float>(
            steps
        );

    const float startX = p.x;
    const float startZ = p.z;

    for (int i = 0;
         i < steps;
         ++i) {

        moveCollisionStep(
            p,
            stepDX,
            stepDZ,
            expanded,
            largeStore
        );
    }

    // -------------------------------------------------------------------------
    // Actual travelled distance
    // -------------------------------------------------------------------------

    const float travelledX =
        (p.x - startX) *
        scaleX;

    const float travelledZ =
        (p.z - startZ) *
        scaleZ;

    const float distance =
        std::hypot(
            travelledX,
            travelledZ
        );

    // -------------------------------------------------------------------------
    // Walk animation
    // -------------------------------------------------------------------------

    if (!std::isfinite(p.walkCycle))
        p.walkCycle = 0.0f;

    p.walkCycle =
        std::fmod(
            p.walkCycle +
            distance * 4.5f,
            TWO_PI
        );

    if (p.walkCycle < 0.0f)
        p.walkCycle += TWO_PI;

    return distance;
}

// =============================================================================
// VISION BLUR
// =============================================================================

float visionBlur(float intoxication)
{
    if (!std::isfinite(intoxication))
        return 0.0f;

    const float normalized =
        std::clamp(
            intoxication,
            0.0f,
            100.0f
        ) /
        100.0f;

    return
        11.0f *
        std::pow(
            normalized,
            1.3f
        );
}

// =============================================================================
// SEAT
// =============================================================================

bool toggleSeat(
    Player& p,
    bool expanded)
{
    if (!expanded)
        return false;

    // =========================================================================
    // Stand up
    // =========================================================================

    if (p.seated) {

        p.seated = false;

        p.x =
            SEAT_EXIT_X;

        p.z =
            SEAT_EXIT_Z;

        // Original code left the sitting pitch (-5) active after standing.
        p.pitch = 0.0f;

        p.walkCycle = 0.0f;

        return true;
    }

    if (
        !std::isfinite(p.x) ||
        !std::isfinite(p.z)
    ) {
        return false;
    }

    // =========================================================================
    // Check distance from sofa
    // =========================================================================

    const float scaleX =
        safeScale(shopScaleX);

    const float scaleZ =
        safeScale(shopScaleZ);

    const float dx =
        (p.x - SEAT_INTERACTION_X) *
        scaleX;

    const float dz =
        (p.z - SEAT_INTERACTION_Z) *
        scaleZ;

    const float distance =
        std::hypot(
            dx,
            dz
        );

    const float reachScale =
        std::sqrt(
            scaleX *
            scaleZ
        );

    if (
        distance >
        SEAT_REACH *
        reachScale
    ) {
        return false;
    }

    // =========================================================================
    // Sit
    // =========================================================================

    p.seated = true;

    p.x =
        SEAT_X;

    p.z =
        SEAT_Z;

    p.yaw =
        -HALF_PI;

    p.pitch =
        -5.0f;

    p.walkCycle =
        0.0f;

    return true;
}