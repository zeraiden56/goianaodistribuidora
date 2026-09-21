#include "render.hpp"
#include "shop_layout.hpp"
#include "customers.hpp"

#include <SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace {

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

constexpr float PI       = 3.14159265358979323846f;
constexpr float HALF_PI  = PI * 0.5f;
constexpr float RAD2DEG  = 180.0f / PI;

constexpr int MAX_CHECKOUTS = 5;

// Virtual UI resolution used by the current renderer.
constexpr float UI_WIDTH  = 960.0f;
constexpr float UI_HEIGHT = 540.0f;

constexpr float BUBBLE_MIN_X = 8.0f;
constexpr float BUBBLE_MAX_X = 952.0f;
constexpr float BUBBLE_MIN_Y = 94.0f;
constexpr float BUBBLE_MAX_Y = 425.0f;

constexpr float BUBBLE_WIDTH       = 174.0f;
constexpr float BUBBLE_HEADER      = 30.0f;
constexpr float BUBBLE_LINE_HEIGHT = 15.0f;

constexpr float CUSTOMER_Z_START = -3.85f;
constexpr float CUSTOMER_Z_END   = -5.20f;

constexpr float CUSTOMER_SIDE_DISTANCE = 8.0f;

// -----------------------------------------------------------------------------
// Structures
// -----------------------------------------------------------------------------

struct CustomerPosition {
    float x = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
    float walk = 0.0f;
};

struct Bubble {
    float x = 0.0f;
    float y = 0.0f;
    float distance = 0.0f;
    bool visible = false;
};

struct UiRect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

std::array<Bubble, MAX_CHECKOUTS> bubbles{};

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

int normalizeStyle(int style)
{
    if (customerLookCount <= 0)
        return 0;

    style %= customerLookCount;

    if (style < 0)
        style += customerLookCount;

    return style;
}

int customerStyleForLane(const Game& g, int lane)
{
    if (lane == 0)
        return normalizeStyle(g.customerStyle);

    if (lane == 1)
        return normalizeStyle(g.second.customerStyle);

    return normalizeStyle(g.extraCheckout(lane).customerStyle);
}

float patienceForLane(const Game& g, int lane)
{
    if (lane == 0)
        return g.patience;

    if (lane == 1)
        return g.second.patience;

    return g.extraCheckout(lane).patience;
}

bool wholesaleForLane(const Game& g, int lane)
{
    if (lane == 0)
        return g.wholesale;

    if (lane == 1)
        return g.second.wholesale;

    return g.extraCheckout(lane).wholesale;
}

bool overlaps(const UiRect& a, const UiRect& b, float padding = 6.0f)
{
    return
        a.x < b.x + b.w + padding &&
        a.x + a.w + padding > b.x &&
        a.y < b.y + b.h + padding &&
        a.y + a.h + padding > b.y;
}

// -----------------------------------------------------------------------------
// Customer movement
// -----------------------------------------------------------------------------

CustomerPosition customerPosition(
    const Game& g,
    int lane,
    bool leaving = false)
{
    const auto& motion = g.customerMotion[lane];

    float t = leaving
        ? 1.0f - motion.departure
        : motion.approach;

    t = std::clamp(t, 0.0f, 1.0f);

    float side = (lane % 2 != 0)
        ? 1.0f
        : -1.0f;

    if (leaving)
        side = -side;

    const float checkout = checkoutX(lane);

    // First stage: walk toward / away from checkout in Z.
    const float forwardProgress =
        std::min(1.0f, t / 0.22f);

    const float z =
        CUSTOMER_Z_START -
        forwardProgress *
        (CUSTOMER_Z_START - CUSTOMER_Z_END);

    // Second stage: move horizontally away from checkout.
    const float lateralProgress =
        std::max(0.0f, (t - 0.22f) / 0.78f);

    const float x =
        checkout +
        side *
        lateralProgress *
        CUSTOMER_SIDE_DISTANCE;

    float yaw = 0.0f;

    if (t > 0.22f) {
        yaw = side > 0.0f
            ? -HALF_PI
            : HALF_PI;
    }

    if (leaving)
        yaw += PI;

    const float walking =
        (t > 0.0f && t < 1.0f)
        ? 1.0f
        : 0.0f;

    return {
        x,
        z,
        yaw,
        walking
    };
}

// -----------------------------------------------------------------------------
// Projection helper
// -----------------------------------------------------------------------------

bool projectCustomerToScreen(
    const CustomerPosition& p,
    const float* model,
    const float* projection,
    Bubble& result)
{
    // Position slightly above the customer's head.
    const float vertex[4] = {
        p.x,
        2.85f,
        p.z,
        1.0f
    };

    float view[4] = {};
    float clip[4] = {};

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            view[row] +=
                model[col * 4 + row] *
                vertex[col];
        }
    }

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            clip[row] +=
                projection[col * 4 + row] *
                view[col];
        }
    }

    // Behind camera.
    if (clip[3] <= 0.0f)
        return false;

    // Outside Z clipping range.
    if (clip[2] < -clip[3] ||
        clip[2] > clip[3]) {
        return false;
    }

    const float ndcX = clip[0] / clip[3];
    const float ndcY = clip[1] / clip[3];

    result.x =
        UI_WIDTH * 0.5f +
        ndcX * UI_WIDTH * 0.5f;

    result.y =
        UI_HEIGHT * 0.5f -
        ndcY * UI_HEIGHT * 0.5f;

    result.distance = -view[2];

    result.visible =
        std::abs(ndcX) < 0.98f &&
        std::abs(ndcY) < 0.96f;

    return result.visible;
}

} // namespace

// =============================================================================
// CHARACTER
// =============================================================================
//
// Shared articulated model.
// Character faces +Z.
//
// Limbs rotate around their actual joints.
// =============================================================================

void character(
    int style,
    float phase,
    float walk,
    float carry,
    float reach,
    bool staff)
{
    style = normalizeStyle(style);

    const auto& look = customerLooks()[style];

    auto skin  = look.skin;
    auto shirt = look.shirt;
    auto pants = look.trousers;
    auto hair  = look.hair;

    // -------------------------------------------------------------------------
    // Staff uniforms
    // -------------------------------------------------------------------------

    if (staff) {
        static const std::array<
            std::array<float, 3>,
            5
        > uniforms{{
            {{0.12f, 0.38f, 0.34f}},
            {{0.67f, 0.29f, 0.17f}},
            {{0.20f, 0.33f, 0.57f}},
            {{0.58f, 0.43f, 0.17f}},
            {{0.39f, 0.25f, 0.48f}}
        }};

        shirt = uniforms[
            static_cast<std::size_t>(style) %
            uniforms.size()
        ];
    }

    glPushMatrix();

    // Body proportions.
    glScalef(
        look.width,
        look.height,
        1.0f
    );

    // Very subtle vertical bob while walking.
    glTranslatef(
        0.0f,
        std::sin(phase * 2.0f) *
            0.015f *
            walk,
        0.0f
    );

    // =========================================================================
    // Torso
    // =========================================================================

    box({
        0.0f, 1.23f, 0.0f,
        0.56f, 0.64f, 0.32f,
        shirt[0], shirt[1], shirt[2]
    });

    // Waist.
    cylinder(
        0.0f,
        0.94f,
        0.0f,
        0.27f,
        0.17f,
        pants[0],
        pants[1],
        pants[2]
    );

    // Belt.
    box({
        0.0f, 0.97f, 0.17f,
        0.56f, 0.055f, 0.025f,
        0.12f, 0.09f, 0.06f
    });

    // Belt buckle.
    box({
        0.0f, 0.97f, 0.19f,
        0.09f, 0.07f, 0.035f,
        0.70f, 0.66f, 0.47f
    });

    // =========================================================================
    // Shirt details
    // =========================================================================

    if (staff) {
        // Apron.
        box({
            0.0f, 1.12f, 0.18f,
            0.42f, 0.65f, 0.035f,
            0.77f, 0.67f, 0.43f
        });

        // Apron pocket.
        box({
            0.0f, 1.22f, 0.203f,
            0.25f, 0.17f, 0.018f,
            0.40f, 0.34f, 0.23f
        });

        // Apron straps.
        for (float x : {-0.16f, 0.16f}) {
            box({
                x, 1.48f, 0.176f,
                0.035f, 0.18f, 0.03f,
                0.77f, 0.67f, 0.43f
            });
        }
    }
    else {
        // Shirt pocket.
        box({
            -0.16f, 1.34f, 0.168f,
            0.12f, 0.13f, 0.022f,
            shirt[0] * 0.8f,
            shirt[1] * 0.8f,
            shirt[2] * 0.8f
        });

        // Buttons.
        for (float y : {
            1.17f,
            1.32f,
            1.47f
        }) {
            box({
                0.0f, y, 0.17f,
                0.022f, 0.023f, 0.016f,
                0.82f, 0.78f, 0.62f
            });
        }
    }

    // =========================================================================
    // Legs and arms
    // =========================================================================

    for (float side : {-1.0f, 1.0f}) {
        const float swing =
            std::sin(phase) *
            side *
            walk;

        // ---------------------------------------------------------------------
        // Leg
        // ---------------------------------------------------------------------

        glPushMatrix();

        // Hip joint.
        glTranslatef(
            side * 0.16f,
            0.89f,
            0.0f
        );

        glRotatef(
            swing * 29.0f,
            1.0f,
            0.0f,
            0.0f
        );

        // Upper leg.
        box({
            0.0f, -0.20f, 0.0f,
            0.22f, 0.43f, 0.25f,
            pants[0],
            pants[1],
            pants[2]
        });

        // Knee joint.
        glTranslatef(
            0.0f,
            -0.40f,
            0.0f
        );

        glRotatef(
            std::max(0.0f, -swing) *
                24.0f,
            1.0f,
            0.0f,
            0.0f
        );

        // Lower leg.
        box({
            0.0f, -0.19f, 0.0f,
            0.20f, 0.39f, 0.23f,
            pants[0] * 0.9f,
            pants[1] * 0.9f,
            pants[2] * 0.9f
        });

        // Shoe.
        box({
            0.0f, -0.39f, 0.07f,
            0.23f, 0.14f, 0.39f,
            0.07f, 0.07f, 0.075f
        });

        // Sole.
        box({
            0.0f, -0.45f, 0.07f,
            0.24f, 0.025f, 0.40f,
            0.30f, 0.31f, 0.30f
        });

        glPopMatrix();

        // ---------------------------------------------------------------------
        // Arm
        // ---------------------------------------------------------------------

        glPushMatrix();

        // Shoulder.
        glTranslatef(
            side * 0.36f,
            1.46f,
            0.0f
        );

        glRotatef(
            -swing * 26.0f -
            carry * 46.0f -
            reach * 35.0f,
            1.0f,
            0.0f,
            0.0f
        );

        glRotatef(
            side *
            (carry * 12.0f + 4.0f),
            0.0f,
            0.0f,
            1.0f
        );

        // Sleeve / upper arm.
        box({
            0.0f, -0.12f, 0.0f,
            0.20f, 0.27f, 0.23f,
            shirt[0],
            shirt[1],
            shirt[2]
        });

        // Elbow.
        box({
            0.0f, -0.25f, 0.0f,
            0.16f, 0.15f, 0.18f,
            skin[0],
            skin[1],
            skin[2]
        });

        glTranslatef(
            0.0f,
            -0.30f,
            0.0f
        );

        glRotatef(
            -12.0f -
            carry * 42.0f -
            reach * 25.0f,
            1.0f,
            0.0f,
            0.0f
        );

        // Forearm.
        box({
            0.0f, -0.14f, 0.0f,
            0.145f, 0.29f, 0.17f,
            skin[0],
            skin[1],
            skin[2]
        });

        // Hand.
        box({
            0.0f, -0.32f, 0.01f,
            0.155f, 0.14f, 0.19f,
            skin[0],
            skin[1],
            skin[2]
        });

        glPopMatrix();
    }

    // =========================================================================
    // Neck and head
    // =========================================================================

    cylinder(
        0.0f,
        1.61f,
        0.0f,
        0.095f,
        0.18f,
        skin[0],
        skin[1],
        skin[2]
    );

    cylinder(
        0.0f,
        1.88f,
        0.0f,
        0.22f,
        0.40f,
        skin[0],
        skin[1],
        skin[2],
        0.205f
    );

    // Hair top.
    cylinder(
        0.0f,
        2.085f,
        -0.018f,
        0.225f,
        0.13f,
        hair[0],
        hair[1],
        hair[2],
        0.18f
    );

    // =========================================================================
    // Face
    // =========================================================================

    for (float side : {-1.0f, 1.0f}) {
        // Ear.
        box({
            side * 0.217f,
            1.87f,
            0.0f,
            0.055f,
            0.115f,
            0.08f,
            skin[0],
            skin[1],
            skin[2]
        });

        // Eye white.
        box({
            side * 0.085f,
            1.925f,
            0.198f,
            0.077f,
            0.049f,
            0.026f,
            0.94f,
            0.91f,
            0.85f
        });

        // Pupil.
        box({
            side * 0.085f,
            1.921f,
            0.217f,
            0.032f,
            0.041f,
            0.012f,
            0.07f,
            0.055f,
            0.04f
        });

        // Eyebrow.
        box({
            side * 0.085f,
            1.978f,
            0.195f,
            0.09f,
            0.021f,
            0.02f,
            hair[0],
            hair[1],
            hair[2]
        });
    }

    // Nose.
    box({
        0.0f,
        1.855f,
        0.221f,
        0.072f,
        0.095f,
        0.075f,
        skin[0] * 0.94f,
        skin[1] * 0.94f,
        skin[2] * 0.94f
    });

    // Mouth.
    box({
        0.0f,
        1.766f,
        0.201f,
        0.106f,
        0.02f,
        0.014f,
        0.35f,
        0.16f,
        0.13f
    });

    // =========================================================================
    // Hair variations
    // =========================================================================

    if (
        style == 0 ||
        style == 5 ||
        style == 7
    ) {
        // Back hair.
        box({
            0.0f,
            1.83f,
            -0.19f,
            0.40f,
            0.58f,
            0.13f,
            hair[0],
            hair[1],
            hair[2]
        });

        for (float x : {-0.21f, 0.21f}) {
            box({
                x,
                1.91f,
                -0.05f,
                0.09f,
                0.37f,
                0.25f,
                hair[0],
                hair[1],
                hair[2]
            });
        }
    }

    // =========================================================================
    // Hat variations
    // =========================================================================

    if (
        style == 1 ||
        style == 3
    ) {
        cylinder(
            0.0f,
            2.14f,
            0.0f,
            0.24f,
            0.12f,
            shirt[0],
            shirt[1],
            shirt[2]
        );

        box({
            0.0f,
            2.10f,
            0.23f,
            0.39f,
            0.045f,
            0.23f,
            shirt[0],
            shirt[1],
            shirt[2]
        });
    }

    // =========================================================================
    // Glasses
    // =========================================================================

    if (
        style == 2 ||
        style == 6
    ) {
        for (float x : {-0.09f, 0.09f}) {
            // Frame.
            box({
                x,
                1.93f,
                0.23f,
                0.16f,
                0.093f,
                0.023f,
                0.13f,
                0.15f,
                0.17f
            });

            // Lens.
            box({
                x,
                1.936f,
                0.246f,
                0.12f,
                0.061f,
                0.009f,
                0.35f,
                0.46f,
                0.47f
            });
        }

        // Nose bridge.
        box({
            0.0f,
            1.945f,
            0.24f,
            0.045f,
            0.023f,
            0.025f,
            0.15f,
            0.16f,
            0.16f
        });
    }

    // =========================================================================
    // Facial hair
    // =========================================================================

    if (
        style == 4 ||
        style == 6
    ) {
        box({
            0.0f,
            1.727f,
            0.12f,
            0.30f,
            0.11f,
            0.15f,
            hair[0],
            hair[1],
            hair[2]
        });
    }

    // Bun / hair knot.
    if (style == 5) {
        cylinder(
            0.0f,
            2.21f,
            -0.13f,
            0.15f,
            0.21f,
            hair[0],
            hair[1],
            hair[2],
            0.11f
        );
    }

    // Ponytail.
    if (style == 7) {
        box({
            0.22f,
            1.72f,
            -0.18f,
            0.12f,
            0.42f,
            0.14f,
            hair[0],
            hair[1],
            hair[2]
        });
    }

    glPopMatrix();
}

// =============================================================================
// CUSTOMER RENDERING
// =============================================================================

void renderCustomer(const Game& g)
{
    const int checkoutCount =
        std::min(
            g.checkoutCount(),
            MAX_CHECKOUTS
        );

    for (int lane = 0;
         lane < checkoutCount;
         ++lane) {

        // ---------------------------------------------------------------------
        // Render active customer and departing customer separately.
        // ---------------------------------------------------------------------

        for (bool leaving : {false, true}) {
            const auto& motion =
                g.customerMotion[lane];

            if (leaving) {
                if (motion.departure <= 0.0f)
                    continue;
            }
            else {
                if (!g.customerActive(lane))
                    continue;
            }

            const CustomerPosition p =
                customerPosition(
                    g,
                    lane,
                    leaving
                );

            const int style = leaving
                ? normalizeStyle(motion.departingStyle)
                : customerStyleForLane(g, lane);

            glPushMatrix();

            glTranslatef(
                p.x,
                0.09f,
                p.z
            );

            // Counteract shop world scaling.
            glScalef(
                1.0f / shopScaleX,
                1.0f,
                1.0f / shopScaleZ
            );

            glRotatef(
                p.yaw * RAD2DEG,
                0.0f,
                1.0f,
                0.0f
            );

            const bool carrying =
                leaving &&
                motion.purchased;

            character(
                style,
                g.time * 8.0f + lane,
                p.walk,
                carrying ? 0.65f : 0.0f,
                0.0f,
                false
            );

            // -----------------------------------------------------------------
            // Purchased product / box
            // -----------------------------------------------------------------

            if (carrying) {
                texturedBox(
                    {
                        0.0f,
                        1.04f,
                        0.46f,

                        0.48f,
                        0.43f,
                        0.30f,

                        0.74f,
                        0.58f,
                        0.34f
                    },
                    Surface::Wood
                );

                // Box handle / upper detail.
                box({
                    0.0f,
                    1.34f,
                    0.46f,

                    0.30f,
                    0.025f,
                    0.025f,

                    0.55f,
                    0.39f,
                    0.20f
                });
            }

            glPopMatrix();
        }
    }
}

// =============================================================================
// CUSTOMER ORDER WORLD -> SCREEN POSITIONS
// =============================================================================

void captureOrderPositions(const Game& g)
{
    float model[16]{};
    float projection[16]{};

    glGetFloatv(
        GL_MODELVIEW_MATRIX,
        model
    );

    glGetFloatv(
        GL_PROJECTION_MATRIX,
        projection
    );

    // Always clear previous bubbles first.
    for (Bubble& bubble : bubbles) {
        bubble.visible = false;
        bubble.distance = 0.0f;
    }

    const int checkoutCount =
        std::min(
            g.checkoutCount(),
            MAX_CHECKOUTS
        );

    for (int lane = 0;
         lane < checkoutCount;
         ++lane) {

        if (!g.customerActive(lane))
            continue;

        const CustomerPosition p =
            customerPosition(
                g,
                lane,
                false
            );

        Bubble projected{};

        if (!projectCustomerToScreen(
                p,
                model,
                projection,
                projected)) {
            continue;
        }

        bubbles[lane] = projected;
    }
}

// =============================================================================
// ORDER BUBBLES
// =============================================================================

void renderOrderBubbles(
    const Game& g,
    const Player& player)
{
    // Do not display customer orders if player is too far away
    // or currently sitting.
    if (
        player.z > 3.6f ||
        player.seated
    ) {
        return;
    }

    std::array<
        UiRect,
        MAX_CHECKOUTS
    > placed{};

    int placedCount = 0;

    const int checkoutCount =
        std::min(
            g.checkoutCount(),
            MAX_CHECKOUTS
        );

    for (int lane = 0;
         lane < checkoutCount;
         ++lane) {

        const Bubble& bubble =
            bubbles[lane];

        if (!bubble.visible)
            continue;

        if (bubble.distance > 19.0f)
            continue;

        // ---------------------------------------------------------------------
        // Number of product rows
        // ---------------------------------------------------------------------

        int lines = 0;

        for (int i = 0;
             i < g.availableProducts();
             ++i) {

            if (g.requested(lane, i) > 0)
                ++lines;
        }

        const float width =
            BUBBLE_WIDTH;

        const float height =
            BUBBLE_HEADER +
            lines * BUBBLE_LINE_HEIGHT;

        float x = std::clamp(
            bubble.x - width * 0.5f,
            BUBBLE_MIN_X,
            BUBBLE_MAX_X - width
        );

        float y = std::clamp(
            bubble.y - height,
            BUBBLE_MIN_Y,
            BUBBLE_MAX_Y - height
        );

        // ---------------------------------------------------------------------
        // Avoid bubble overlap.
        // Move the new bubble upward until free.
        // ---------------------------------------------------------------------

        bool validPosition = true;

        for (int attempt = 0;
             attempt < MAX_CHECKOUTS;
             ++attempt) {

            UiRect candidate{
                x,
                y,
                width,
                height
            };

            bool foundOverlap = false;
            float newY = y;

            for (int i = 0;
                 i < placedCount;
                 ++i) {

                if (!overlaps(
                        candidate,
                        placed[i])) {
                    continue;
                }

                newY = std::min(
                    newY,
                    placed[i].y -
                    height -
                    8.0f
                );

                foundOverlap = true;
            }

            if (!foundOverlap)
                break;

            y = newY;

            if (y < BUBBLE_MIN_Y) {
                validPosition = false;
                break;
            }
        }

        if (!validPosition)
            continue;

        if (placedCount >= MAX_CHECKOUTS)
            break;

        placed[placedCount++] = {
            x,
            y,
            width,
            height
        };

        // ---------------------------------------------------------------------
        // Main background
        // ---------------------------------------------------------------------

        rect(
            x,
            y,
            width,
            height,
            0.045f,
            0.09f,
            0.09f
        );

        // Accent strip.
        rect(
            x,
            y,
            3.0f,
            height,
            0.92f,
            0.67f,
            0.30f
        );

        // ---------------------------------------------------------------------
        // Customer name
        // ---------------------------------------------------------------------

        const int style =
            customerStyleForLane(
                g,
                lane
            );

        const auto& look =
            customerLooks()[style];

        const std::string title =
            std::string(look.name) +
            " - " +
            std::to_string(lane + 1);

        text(
            x + 10.0f,
            y + 8.0f,
            title,
            1.10f,
            0.97f,
            0.77f,
            0.41f
        );

        // ---------------------------------------------------------------------
        // Order product rows
        // ---------------------------------------------------------------------

        int row = 0;

        for (int i = 0;
             i < g.availableProducts();
             ++i) {

            const int requested =
                g.requested(lane, i);

            if (requested <= 0)
                continue;

            const int fulfilled =
                g.fulfilled(lane, i);

            const bool complete =
                g.remaining(lane, i) == 0;

            const std::string line =
                std::to_string(fulfilled) +
                "/" +
                std::to_string(requested) +
                " " +
                g.products[i].name;

            text(
                x + 10.0f,
                y + 23.0f +
                    row * BUBBLE_LINE_HEIGHT,
                line,
                1.05f,

                complete
                    ? 0.40f
                    : 0.93f,

                complete
                    ? 0.80f
                    : 0.93f,

                complete
                    ? 0.60f
                    : 0.86f
            );

            ++row;
        }

        // ---------------------------------------------------------------------
        // Patience bar
        // ---------------------------------------------------------------------

        const float patience =
            patienceForLane(
                g,
                lane
            );

        const bool wholesale =
            wholesaleForLane(
                g,
                lane
            );

        const float maxPatience =
            wholesale
                ? 180.0f
                : 65.0f;

        const float patienceRatio =
            std::clamp(
                patience / maxPatience,
                0.0f,
                1.0f
            );

        constexpr float PATIENCE_X_MARGIN = 8.0f;

        const float patienceWidth =
            width -
            PATIENCE_X_MARGIN * 2.0f;

        const float patienceY =
            y +
            height -
            5.0f;

        // Background.
        rect(
            x + PATIENCE_X_MARGIN,
            patienceY,
            patienceWidth,
            2.0f,
            0.16f,
            0.24f,
            0.23f
        );

        // Current patience.
        const bool critical =
            patience < 15.0f;

        rect(
            x + PATIENCE_X_MARGIN,
            patienceY,
            patienceWidth * patienceRatio,
            2.0f,

            critical
                ? 0.95f
                : 0.42f,

            critical
                ? 0.32f
                : 0.75f,

            0.43f
        );

        // ---------------------------------------------------------------------
        // Pointer triangle
        // ---------------------------------------------------------------------

        const float pointerX =
            std::clamp(
                bubble.x,
                x + 8.0f,
                x + width - 8.0f
            );

        glColor3f(
            0.045f,
            0.09f,
            0.09f
        );

        glBegin(GL_TRIANGLES);

        glVertex2f(
            pointerX - 5.0f,
            y + height
        );

        glVertex2f(
            pointerX + 5.0f,
            y + height
        );

        glVertex2f(
            pointerX,
            y + height + 7.0f
        );

        glEnd();
    }
}