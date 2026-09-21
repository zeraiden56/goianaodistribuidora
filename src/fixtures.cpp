#include "fixtures.hpp"

#include "render.hpp"
#include "shop_layout.hpp"

#include <SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr float FRIDGE_DOOR_MAX_ANGLE = 68.0f;

constexpr float FRIDGE_Z       = 3.30f;
constexpr float FRIDGE_BACK_Z  = 3.80f;
constexpr float FRIDGE_DOOR_Z  = 2.78f;

constexpr float PI = 3.14159265358979323846f;

// =============================================================================
// Small helpers
// =============================================================================

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

int fridgeTimerIndex(int product)
{
    switch (product) {
        case 0: return 0;
        case 2: return 1;
        case 4: return 2;
        case 5: return 3;
        default: return 0;
    }
}

float doorAngle(float remaining)
{
    if (remaining <= 0.0f)
        return 0.0f;

    // Opening.
    if (remaining > 1.3f) {
        const float t =
            clamp01(
                (1.6f - remaining) / 0.3f
            );

        return t * FRIDGE_DOOR_MAX_ANGLE;
    }

    // Fully open.
    if (remaining > 0.4f)
        return FRIDGE_DOOR_MAX_ANGLE;

    // Closing.
    return
        clamp01(remaining / 0.4f) *
        FRIDGE_DOOR_MAX_ANGLE;
}

void doorTransform(
    const Game& g,
    int product)
{
    const int timer =
        fridgeTimerIndex(product);

    glTranslatef(
        stockLocations[product].x - 0.96f,
        0.0f,
        FRIDGE_DOOR_Z
    );

    glRotatef(
        doorAngle(g.fridgeTime[timer]),
        0.0f,
        1.0f,
        0.0f
    );
}

// =============================================================================
// Procedural material helpers
// =============================================================================

// Draws a very thin darker strip. Useful for seams between panels.
void seam(
    float x,
    float y,
    float z,
    float w,
    float h,
    float d)
{
    box({
        x, y, z,
        w, h, d,

        0.08f,
        0.11f,
        0.12f
    });
}

// Small highlight strip used on metal edges.
void metalHighlight(
    float x,
    float y,
    float z,
    float w,
    float h,
    float d)
{
    box({
        x, y, z,
        w, h, d,

        0.47f,
        0.53f,
        0.54f
    });
}

// Dark inset panel.
void darkInset(
    float x,
    float y,
    float z,
    float w,
    float h,
    float d)
{
    box({
        x, y, z,
        w, h, d,

        0.075f,
        0.105f,
        0.11f
    });
}

// =============================================================================
// Refrigerator body
// =============================================================================

void renderFridgeBody(
    const Game& g,
    int product)
{
    const float x =
        stockLocations[product].x;

    // -------------------------------------------------------------------------
    // Rear/interior wall
    // -------------------------------------------------------------------------

    box({
        x,
        1.32f,
        FRIDGE_BACK_Z,

        2.08f,
        2.64f,
        0.12f,

        0.16f,
        0.22f,
        0.24f
    });

    // Slightly darker inner cavity.
    box({
        x,
        1.30f,
        3.735f,

        1.86f,
        2.42f,
        0.035f,

        0.095f,
        0.14f,
        0.15f
    });

    // Rear vertical panel variation.
    for (float panelX : {
        -0.60f,
         0.00f,
         0.60f
    }) {
        seam(
            x + panelX,
            1.30f,
            3.71f,

            0.015f,
            2.28f,
            0.012f
        );
    }

    // -------------------------------------------------------------------------
    // Side columns
    // -------------------------------------------------------------------------

    for (float side : {-0.99f, 0.99f}) {
        // Main metal rail.
        box({
            x + side,
            1.32f,
            FRIDGE_Z,

            0.10f,
            2.64f,
            1.00f,

            0.53f,
            0.59f,
            0.59f
        });

        // Dark inner edge.
        box({
            x + side * 0.95f,
            1.32f,
            3.23f,

            0.045f,
            2.48f,
            0.82f,

            0.18f,
            0.23f,
            0.23f
        });

        // Metallic highlight on front edge.
        metalHighlight(
            x + side * 1.025f,
            1.32f,
            2.84f,

            0.018f,
            2.54f,
            0.055f
        );
    }

    // -------------------------------------------------------------------------
    // Bottom / top casing
    // -------------------------------------------------------------------------

    for (float y : {
        0.09f,
        2.62f
    }) {
        box({
            x,
            y,
            FRIDGE_Z,

            2.08f,
            0.15f,
            1.00f,

            0.48f,
            0.54f,
            0.54f
        });
    }

    // Bright front edge.
    metalHighlight(
        x,
        2.66f,
        2.84f,

        2.02f,
        0.035f,
        0.035f
    );

    // -------------------------------------------------------------------------
    // Bottom kick plate
    // -------------------------------------------------------------------------

    box({
        x,
        0.075f,
        2.86f,

        1.96f,
        0.14f,
        0.13f,

        0.10f,
        0.13f,
        0.14f
    });

    // Horizontal ventilation slots.
    for (int i = 0; i < 9; ++i) {
        const float ventX =
            x - 0.72f +
            i * 0.18f;

        box({
            ventX,
            0.075f,
            2.785f,

            0.095f,
            0.025f,
            0.008f,

            0.025f,
            0.035f,
            0.038f
        });
    }

    // -------------------------------------------------------------------------
    // Shelves
    // -------------------------------------------------------------------------

    constexpr std::array<float, 4> shelfY{
        0.25f,
        0.91f,
        1.57f,
        2.23f
    };

    for (float y : shelfY) {
        // Shelf body.
        box({
            x,
            y,
            FRIDGE_Z,

            1.85f,
            0.045f,
            0.87f,

            0.59f,
            0.68f,
            0.67f
        });

        // Front chrome lip.
        box({
            x,
            y + 0.015f,
            2.87f,

            1.82f,
            0.055f,
            0.035f,

            0.72f,
            0.78f,
            0.76f
        });

        // Dark shadow just under shelf.
        box({
            x,
            y - 0.035f,
            3.20f,

            1.78f,
            0.018f,
            0.60f,

            0.055f,
            0.075f,
            0.078f
        });
    }

    // -------------------------------------------------------------------------
    // Upper lighting panel
    // -------------------------------------------------------------------------

    box({
        x,
        2.47f,
        3.13f,

        1.72f,
        0.04f,
        0.17f,

        0.80f,
        0.94f,
        0.86f
    });

    // Light glow edge.
    box({
        x,
        2.445f,
        3.10f,

        1.60f,
        0.018f,
        0.025f,

        0.60f,
        0.82f,
        0.72f
    });

    // -------------------------------------------------------------------------
    // Header / product signs
    // -------------------------------------------------------------------------

    // Dark board behind name.
    box({
        x,
        2.965f,
        2.785f,

        1.92f,
        0.37f,
        0.055f,

        0.07f,
        0.10f,
        0.105f
    });

    // Thin metal border.
    box({
        x,
        3.145f,
        2.775f,

        1.98f,
        0.025f,
        0.075f,

        0.48f,
        0.52f,
        0.52f
    });

    sign(
        x + 0.89f,
        2.98f,
        2.74f,

        g.products[product].name,
        1.8f,
        true
    );

    sign(
        x + 0.42f,
        2.73f,
        2.74f,

        g.stockLabel(product),
        0.85f,
        true
    );
}

// =============================================================================
// Products inside refrigerator
// =============================================================================

void renderFridgeProducts(
    const Game& g,
    int product)
{
    const float x =
        stockLocations[product].x;

    const int stock =
        std::min(
            24,
            g.products[product].stock
        );

    for (int i = 0; i < stock; ++i) {
        const int column = i % 6;
        const int row    = i / 6;

        glPushMatrix();

        glTranslatef(
            x - 0.76f +
                column * 0.30f,

            0.47f +
                row * 0.66f,

            3.17f
        );

        glRotatef(
            180.0f,
            0.0f,
            1.0f,
            0.0f
        );

        item(
            product,
            0.0f,
            0.0f,
            0.0f,
            0.74f
        );

        glPopMatrix();
    }
}

// =============================================================================
// Refrigerator door
// =============================================================================

void renderFridgeDoorFrame(
    const Game& g,
    int product)
{
    glPushMatrix();

    doorTransform(
        g,
        product
    );

    // -------------------------------------------------------------------------
    // Outer frame
    // -------------------------------------------------------------------------

    for (float x : {
        0.0f,
        1.92f
    }) {
        box({
            x,
            1.29f,
            0.0f,

            0.055f,
            2.50f,
            0.07f,

            0.23f,
            0.29f,
            0.31f
        });

        // Metallic inner highlight.
        box({
            x +
                (x < 1.0f
                    ? 0.025f
                    : -0.025f),

            1.29f,
            -0.041f,

            0.012f,
            2.43f,
            0.018f,

            0.54f,
            0.59f,
            0.59f
        });
    }

    for (float y : {
        0.08f,
        2.51f
    }) {
        box({
            0.96f,
            y,
            0.0f,

            1.92f,
            0.055f,
            0.07f,

            0.23f,
            0.29f,
            0.31f
        });
    }

    // -------------------------------------------------------------------------
    // Rubber seal
    // -------------------------------------------------------------------------

    // Vertical seals.
    for (float x : {
        0.075f,
        1.845f
    }) {
        box({
            x,
            1.29f,
            -0.046f,

            0.025f,
            2.35f,
            0.025f,

            0.035f,
            0.045f,
            0.045f
        });
    }

    // Horizontal seals.
    for (float y : {
        0.15f,
        2.43f
    }) {
        box({
            0.96f,
            y,
            -0.046f,

            1.72f,
            0.025f,
            0.025f,

            0.035f,
            0.045f,
            0.045f
        });
    }

    // -------------------------------------------------------------------------
    // Handle
    // -------------------------------------------------------------------------

    // Handle shadow.
    box({
        1.765f,
        1.28f,
        -0.115f,

        0.075f,
        0.66f,
        0.085f,

        0.10f,
        0.12f,
        0.12f
    });

    // Main handle.
    box({
        1.76f,
        1.28f,
        -0.145f,

        0.055f,
        0.63f,
        0.075f,

        0.72f,
        0.77f,
        0.76f
    });

    // Handle highlight.
    box({
        1.755f,
        1.28f,
        -0.188f,

        0.018f,
        0.57f,
        0.018f,

        0.92f,
        0.94f,
        0.91f
    });

    // -------------------------------------------------------------------------
    // Hinges
    // -------------------------------------------------------------------------

    for (float y : {
        0.28f,
        2.30f
    }) {
        box({
            0.015f,
            y,
            -0.065f,

            0.095f,
            0.16f,
            0.09f,

            0.12f,
            0.15f,
            0.16f
        });
    }

    glPopMatrix();
}

// =============================================================================
// Glass helpers
// =============================================================================

void transparentQuad(
    float x1,
    float y1,
    float x2,
    float y2,
    float z,

    float r,
    float g,
    float b,
    float a)
{
    glColor4f(
        r,
        g,
        b,
        a
    );

    glBegin(GL_QUADS);

    glVertex3f(x1, y1, z);
    glVertex3f(x2, y1, z);
    glVertex3f(x2, y2, z);
    glVertex3f(x1, y2, z);

    glEnd();
}

void renderSingleFridgeGlass(
    const Game& g,
    int product)
{
    glPushMatrix();

    doorTransform(
        g,
        product
    );

    // -------------------------------------------------------------------------
    // Base glass tint
    // -------------------------------------------------------------------------

    transparentQuad(
        0.035f,
        0.11f,

        1.885f,
        2.48f,

        -0.004f,

        0.36f,
        0.64f,
        0.70f,
        0.13f
    );

    // -------------------------------------------------------------------------
    // Slight darker edges, creating depth in glass
    // -------------------------------------------------------------------------

    transparentQuad(
        0.045f,
        0.12f,

        0.095f,
        2.47f,

        -0.008f,

        0.08f,
        0.18f,
        0.20f,
        0.14f
    );

    transparentQuad(
        1.825f,
        0.12f,

        1.875f,
        2.47f,

        -0.008f,

        0.08f,
        0.18f,
        0.20f,
        0.10f
    );

    // -------------------------------------------------------------------------
    // Large diagonal reflection
    // -------------------------------------------------------------------------

    glColor4f(
        0.88f,
        0.97f,
        1.00f,
        0.17f
    );

    glBegin(GL_QUADS);

    glVertex3f(
        0.14f,
        0.30f,
        -0.012f
    );

    glVertex3f(
        0.25f,
        0.30f,
        -0.012f
    );

    glVertex3f(
        0.79f,
        2.33f,
        -0.012f
    );

    glVertex3f(
        0.64f,
        2.33f,
        -0.012f
    );

    glEnd();

    // Secondary reflection.
    glColor4f(
        0.90f,
        0.98f,
        1.00f,
        0.08f
    );

    glBegin(GL_QUADS);

    glVertex3f(
        1.18f,
        0.24f,
        -0.013f
    );

    glVertex3f(
        1.24f,
        0.24f,
        -0.013f
    );

    glVertex3f(
        1.66f,
        1.75f,
        -0.013f
    );

    glVertex3f(
        1.58f,
        1.75f,
        -0.013f
    );

    glEnd();

    // -------------------------------------------------------------------------
    // Horizontal subtle reflections
    // -------------------------------------------------------------------------

    for (float y : {
        0.52f,
        1.18f,
        1.84f
    }) {
        transparentQuad(
            0.12f,
            y,

            1.80f,
            y + 0.012f,

            -0.015f,

            0.75f,
            0.91f,
            0.94f,
            0.06f
        );
    }

    // -------------------------------------------------------------------------
    // Tiny condensation / grime marks
    //
    // Kept deterministic so they don't "sparkle" every frame.
    // -------------------------------------------------------------------------

    constexpr std::array<
        std::array<float, 3>,
        12
    > marks{{
        {{0.28f, 0.72f, 0.012f}},
        {{0.48f, 1.74f, 0.009f}},
        {{0.73f, 2.04f, 0.013f}},
        {{0.91f, 0.48f, 0.008f}},
        {{1.12f, 1.32f, 0.011f}},
        {{1.38f, 1.92f, 0.009f}},
        {{1.61f, 0.83f, 0.012f}},
        {{0.37f, 1.21f, 0.007f}},
        {{0.84f, 1.52f, 0.008f}},
        {{1.48f, 2.19f, 0.007f}},
        {{1.68f, 1.43f, 0.009f}},
        {{1.22f, 0.58f, 0.008f}}
    }};

    for (const auto& mark : marks) {
        const float x = mark[0];
        const float y = mark[1];
        const float s = mark[2];

        transparentQuad(
            x - s,
            y - s,

            x + s,
            y + s,

            -0.017f,

            0.92f,
            0.97f,
            0.97f,
            0.10f
        );
    }

    glPopMatrix();
}

// =============================================================================
// Counter material
// =============================================================================

void renderWoodPanel(
    float x,
    float y,
    float z,
    float w,
    float h,
    float d)
{
    texturedBox(
        {
            x,
            y,
            z,

            w,
            h,
            d,

            0.47f,
            0.34f,
            0.22f
        },
        Surface::Wood
    );

    // Dark underside / trim gives it a thicker appearance.
    box({
        x,
        y - h * 0.48f,
        z - d * 0.48f,

        w * 0.97f,
        0.025f,
        0.025f,

        0.16f,
        0.10f,
        0.065f
    });
}

} // namespace

// =============================================================================
// REFRIGERATORS
// =============================================================================

void renderRefrigerators(const Game& g)
{
    constexpr std::array<int, 4> fridgeProducts{
        0,
        2,
        4,
        5
    };

    for (int product : fridgeProducts) {
        if (product >= g.availableProducts())
            continue;

        renderFridgeBody(
            g,
            product
        );

        renderFridgeProducts(
            g,
            product
        );

        renderFridgeDoorFrame(
            g,
            product
        );
    }
}

// =============================================================================
// REFRIGERATOR GLASS
// =============================================================================

void renderFridgeGlass(const Game& g)
{
    constexpr std::array<int, 4> fridgeProducts{
        0,
        2,
        4,
        5
    };

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    // Glass shouldn't fill the depth buffer.
    glDepthMask(GL_FALSE);

    for (int product : fridgeProducts) {
        if (product >= g.availableProducts())
            continue;

        renderSingleFridgeGlass(
            g,
            product
        );
    }

    glDepthMask(GL_TRUE);

    glDisable(GL_BLEND);

    // Important if another renderer assumes opaque white afterwards.
    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        1.0f
    );
}

// =============================================================================
// CIGARETTE COUNTER
// =============================================================================

void renderCigaretteCounter(const Game& g)
{
    // -------------------------------------------------------------------------
    // Main wooden cabinets
    // -------------------------------------------------------------------------

    renderWoodPanel(
        -2.70f,
        0.55f,
        -2.60f,

        4.40f,
        1.10f,
        0.90f
    );

    renderWoodPanel(
        3.30f,
        0.55f,
        -2.60f,

        3.20f,
        1.10f,
        0.90f
    );

    // -------------------------------------------------------------------------
    // Open center cubby
    // -------------------------------------------------------------------------

    // Deep shadow in back.
    darkInset(
        0.65f,
        0.53f,
        -2.98f,

        2.10f,
        1.05f,
        0.08f
    );

    // Subtle inner panel.
    box({
        0.65f,
        0.53f,
        -2.93f,

        1.96f,
        0.91f,
        0.018f,

        0.105f,
        0.17f,
        0.15f
    });

    // -------------------------------------------------------------------------
    // Shelves
    // -------------------------------------------------------------------------

    for (float y : {
        0.08f,
        0.55f
    }) {
        texturedBox(
            {
                0.65f,
                y,
                -2.60f,

                2.10f,
                0.07f,
                0.88f,

                0.43f,
                0.31f,
                0.20f
            },
            Surface::Wood
        );

        // Bright front lip.
        box({
            0.65f,
            y + 0.012f,
            -2.145f,

            2.06f,
            0.045f,
            0.022f,

            0.59f,
            0.43f,
            0.27f
        });

        // Shadow below shelf.
        box({
            0.65f,
            y - 0.055f,
            -2.40f,

            1.98f,
            0.025f,
            0.43f,

            0.055f,
            0.075f,
            0.065f
        });
    }

    // -------------------------------------------------------------------------
    // Vertical cubby trim
    // -------------------------------------------------------------------------

    for (float x : {
        -0.42f,
        1.72f
    }) {
        box({
            x,
            0.52f,
            -2.135f,

            0.055f,
            1.03f,
            0.055f,

            0.34f,
            0.23f,
            0.15f
        });

        // Highlight.
        box({
            x,
            0.52f,
            -2.168f,

            0.018f,
            0.98f,
            0.014f,

            0.57f,
            0.42f,
            0.27f
        });
    }

    // -------------------------------------------------------------------------
    // Cigarette signage
    // -------------------------------------------------------------------------

    // Header plate.
    box({
        0.03f,
        1.04f,
        -2.125f,

        1.60f,
        0.23f,
        0.025f,

        0.065f,
        0.085f,
        0.075f
    });

    // Gold-ish border around sign.
    box({
        0.03f,
        1.16f,
        -2.14f,

        1.64f,
        0.018f,
        0.038f,

        0.58f,
        0.44f,
        0.23f
    });

    box({
        0.03f,
        0.92f,
        -2.14f,

        1.64f,
        0.018f,
        0.038f,

        0.58f,
        0.44f,
        0.23f
    });

    sign(
        -0.22f,
        1.03f,
        -2.11f,

        "CIGARROS",
        1.72f
    );

    // -------------------------------------------------------------------------
    // Stock
    // -------------------------------------------------------------------------

    // Prevent access to products[1] when cigarettes aren't available yet.
    if (g.availableProducts() <= 1)
        return;

    sign(
        0.26f,
        0.80f,
        -2.10f,

        g.stockLabel(1),
        0.79f
    );

    const int cigaretteStock =
        std::min(
            16,
            g.products[1].stock
        );

    for (int i = 0;
         i < cigaretteStock;
         ++i) {

        const int column =
            i % 8;

        const int row =
            i / 8;

        item(
            1,

            -0.19f +
                column * 0.24f,

            0.29f +
                row * 0.43f,

            -2.34f,

            0.72f
        );
    }

    // -------------------------------------------------------------------------
    // Small wear / scuffing
    //
    // These marks stop the counter looking like a perfectly clean CG block.
    // -------------------------------------------------------------------------

    constexpr std::array<
        std::array<float, 3>,
        7
    > scratches{{
        {{-3.75f, 0.48f, -2.12f}},
        {{-2.32f, 0.29f, -2.12f}},
        {{-1.11f, 0.78f, -2.12f}},
        {{ 2.37f, 0.35f, -2.12f}},
        {{ 3.08f, 0.73f, -2.12f}},
        {{ 3.86f, 0.23f, -2.12f}},
        {{ 4.43f, 0.65f, -2.12f}}
    }};

    for (const auto& scratch : scratches) {
        box({
            scratch[0],
            scratch[1],
            scratch[2],

            0.17f,
            0.012f,
            0.012f,

            0.64f,
            0.48f,
            0.31f
        });
    }
}