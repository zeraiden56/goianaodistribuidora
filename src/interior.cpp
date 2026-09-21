#include "interior.hpp"

#include "render.hpp"
#include "computer.hpp"
#include "shop_layout.hpp"

#include <SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr float RAD2DEG = 180.0f / PI;

// =============================================================================
// Generic interior helpers
// =============================================================================

void wallTrim(
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
        0.25f, 0.28f, 0.24f
    });

    box({
        x,
        y + h * 0.15f,
        z - d * 0.08f,
        w * 0.96f,
        h * 0.12f,
        d * 0.30f,
        0.48f, 0.50f, 0.43f
    });
}

void ceilingLamp(
    float x,
    float z,
    float width = 1.9f)
{
    // Fixture.
    box({
        x,
        3.55f,
        z,
        width,
        0.06f,
        0.30f,
        0.91f,
        0.92f,
        0.78f
    });

    // Bright central strip.
    box({
        x,
        3.515f,
        z,
        width * 0.86f,
        0.018f,
        0.22f,
        1.0f,
        0.97f,
        0.82f
    });
}

// =============================================================================
// Storage shelf
// =============================================================================

void storageShelf(
    const Game& g,
    int product,
    float x)
{
    if (
        product < 0 ||
        product >= g.availableProducts()
    ) {
        return;
    }

    // -------------------------------------------------------------------------
    // Rear shadow / backing panel
    // -------------------------------------------------------------------------

    box({
        x,
        1.15f,
        9.53f,
        1.92f,
        2.28f,
        0.055f,
        0.075f,
        0.095f,
        0.09f
    });

    // -------------------------------------------------------------------------
    // Wooden shelves
    // -------------------------------------------------------------------------

    for (float y : {
        0.20f,
        1.20f
    }) {
        texturedBox(
            {
                x,
                y,
                9.15f,

                1.85f,
                0.09f,
                0.85f,

                0.44f,
                0.32f,
                0.21f
            },
            Surface::Wood
        );

        // Front lip.
        box({
            x,
            y + 0.03f,
            8.70f,

            1.82f,
            0.075f,
            0.04f,

            0.22f,
            0.17f,
            0.12f
        });
    }

    // -------------------------------------------------------------------------
    // Uprights
    // -------------------------------------------------------------------------

    for (float dx : {
        -0.90f,
        0.90f
    }) {
        box({
            x + dx,
            1.15f,
            9.15f,

            0.08f,
            2.30f,
            0.85f,

            0.18f,
            0.22f,
            0.20f
        });

        // Metallic-ish highlight.
        box({
            x + dx - 0.022f,
            1.15f,
            8.705f,

            0.018f,
            2.20f,
            0.018f,

            0.46f,
            0.49f,
            0.43f
        });
    }

    // -------------------------------------------------------------------------
    // Labels
    // -------------------------------------------------------------------------

    sign(
        x + 0.80f,
        2.80f,
        8.61f,
        g.products[product].name,
        1.60f,
        true
    );

    sign(
        x + 0.50f,
        2.44f,
        8.60f,
        g.stockLabel(product),
        1.00f,
        true
    );

    // -------------------------------------------------------------------------
    // Product cartons
    //
    // The original visualized roughly one carton for every six units.
    // Keep that behaviour, but cap the visible geometry.
    // -------------------------------------------------------------------------

    const int visibleBoxes =
        std::min(
            8,
            std::max(
                0,
                (g.products[product].stock + 5) / 6
            )
        );

    for (int i = 0;
         i < visibleBoxes;
         ++i) {

        const float xx =
            x -
            0.63f +
            (i % 4) *
            0.42f;

        const float yy =
            0.53f +
            (i / 4);

        // Main cardboard body.
        texturedBox(
            {
                xx,
                yy,
                9.15f,

                0.37f,
                0.55f,
                0.60f,

                product == 3
                    ? 0.44f
                    : 0.57f,

                product == 3
                    ? 0.59f
                    : 0.44f,

                product == 3
                    ? 0.62f
                    : 0.29f
            },
            Surface::Wood
        );

        // Tape / label.
        box({
            xx,
            yy,
            8.84f,

            0.23f,
            0.13f,
            0.02f,

            0.85f,
            0.75f,
            0.50f
        });

        // Small dark underside adds depth.
        box({
            xx,
            yy - 0.285f,
            9.15f,

            0.34f,
            0.025f,
            0.54f,

            0.15f,
            0.12f,
            0.09f
        });
    }
}

// =============================================================================
// Sofa
// =============================================================================

void sofa()
{
    // Base.
    texturedBox(
        {
            3.88f,
            0.45f,
            6.20f,

            1.35f,
            0.50f,
            2.30f,

            0.24f,
            0.36f,
            0.37f
        },
        Surface::Plaster
    );

    // Backrest.
    texturedBox(
        {
            4.43f,
            0.96f,
            6.20f,

            0.29f,
            0.90f,
            2.45f,

            0.19f,
            0.31f,
            0.33f
        },
        Surface::Plaster
    );

    // Armrests.
    for (float z : {
        5.02f,
        7.38f
    }) {
        texturedBox(
            {
                3.87f,
                0.75f,
                z,

                1.45f,
                0.40f,
                0.22f,

                0.20f,
                0.32f,
                0.34f
            },
            Surface::Plaster
        );
    }

    // Two distinct cushions.
    for (float z : {
        5.65f,
        6.75f
    }) {
        texturedBox(
            {
                3.73f,
                0.78f,
                z,

                1.04f,
                0.18f,
                1.02f,

                0.29f,
                0.43f,
                0.43f
            },
            Surface::Plaster
        );

        // Cushion seam.
        box({
            3.17f,
            0.78f,
            z,

            0.018f,
            0.12f,
            0.92f,

            0.12f,
            0.20f,
            0.21f
        });
    }

    // Feet.
    for (float z : {
        5.18f,
        7.22f
    }) {
        box({
            3.62f,
            0.13f,
            z,

            0.10f,
            0.22f,
            0.10f,

            0.10f,
            0.075f,
            0.055f
        });

        box({
            4.22f,
            0.13f,
            z,

            0.10f,
            0.22f,
            0.10f,

            0.10f,
            0.075f,
            0.055f
        });
    }

    glPushMatrix();

    glTranslatef(
        3.18f,
        0.61f,
        5.60f
    );

    glRotatef(
        -90.0f,
        0.0f,
        1.0f,
        0.0f
    );

    sign(
        0.0f,
        0.0f,
        0.0f,
        "SOFA",
        1.10f
    );

    glPopMatrix();
}

// =============================================================================
// Sofa terminal table
// =============================================================================

void sofaComputerTable(
    const Game& g)
{
    // Main top.
    texturedBox(
        {
            2.50f,
            0.86f,
            7.40f,

            1.00f,
            0.09f,
            0.65f,

            0.57f,
            0.42f,
            0.27f
        },
        Surface::Wood
    );

    // Slight front edge.
    texturedBox(
        {
            2.50f,
            0.81f,
            7.07f,

            1.00f,
            0.08f,
            0.05f,

            0.40f,
            0.27f,
            0.16f
        },
        Surface::Wood
    );

    // Legs.
    for (float x : {
        2.08f,
        2.92f
    }) {
        for (float z : {
            7.15f,
            7.65f
        }) {
            box({
                x,
                0.41f,
                z,

                0.055f,
                0.82f,
                0.05f,

                0.17f,
                0.22f,
                0.21f
            });
        }
    }

    // Small cross beam gives the table more structure.
    box({
        2.50f,
        0.24f,
        7.40f,

        0.76f,
        0.055f,
        0.055f,

        0.15f,
        0.19f,
        0.18f
    });

    renderTerminal(
        g,
        1
    );

    sign(
        2.93f,
        1.82f,
        7.385f,
        "COMPUTADOR",
        0.95f,
        true
    );
}

// =============================================================================
// Television
// =============================================================================

void television(const Game& g)
{
    // -------------------------------------------------------------------------
    // Back housing
    // -------------------------------------------------------------------------

    box({
        -4.80f,
        1.85f,
        6.20f,

        0.18f,
        1.80f,
        3.06f,

        0.035f,
        0.045f,
        0.052f
    });

    // Front bezel.
    box({
        -4.695f,
        1.85f,
        6.20f,

        0.035f,
        1.66f,
        2.90f,

        0.07f,
        0.08f,
        0.085f
    });

    // Screen.
    glPushMatrix();

    glTranslatef(
        -4.674f,
        2.66f,
        7.62f
    );

    glRotatef(
        90.0f,
        0.0f,
        1.0f,
        0.0f
    );

    glScalef(
        0.0045f,
        -0.0045f,
        0.0045f
    );

    glDepthFunc(
        GL_LEQUAL
    );

    rect(
        0,
        0,
        630,
        360,
        0.018f,
        0.028f,
        0.03f
    );

    glTranslatef(
        0,
        0,
        0.2f
    );

    if (g.tvOn) {

        // -------------------------------------------------------------
        // Football pitch
        // -------------------------------------------------------------

        rect(
            0,
            0,
            630,
            360,
            0.065f,
            0.28f,
            0.15f
        );

        // Alternating grass strips.
        for (int i = 0;
             i < 7;
             ++i) {

            rect(
                i * 90.0f,
                32.0f,
                45.0f,
                280.0f,

                0.075f,
                0.33f,
                0.18f
            );
        }

        // Field markings.
        rect(
            30,
            30,
            570,
            3,
            0.78f,
            0.84f,
            0.70f
        );

        rect(
            30,
            309,
            570,
            3,
            0.78f,
            0.84f,
            0.70f
        );

        rect(
            30,
            30,
            3,
            282,
            0.78f,
            0.84f,
            0.70f
        );

        rect(
            597,
            30,
            3,
            282,
            0.78f,
            0.84f,
            0.70f
        );

        rect(
            314,
            30,
            3,
            282,
            0.78f,
            0.84f,
            0.70f
        );

        // Center circle approximation.
        for (int i = 0;
             i < 12;
             ++i) {

            const float angle =
                i *
                PI *
                2.0f /
                12.0f;

            rect(
                312.0f +
                    std::cos(angle) *
                    40.0f,

                170.0f +
                    std::sin(angle) *
                    40.0f,

                4,
                4,

                0.70f,
                0.77f,
                0.64f
            );
        }

        const float ballX =
            315.0f +
            std::sin(
                g.time * 0.70f
            ) *
            230.0f;

        const float ballY =
            165.0f +
            std::sin(
                g.time * 1.20f
            ) *
            85.0f;

        // Players.
        for (int i = 0;
             i < 6;
             ++i) {

            const float x =
                75.0f +
                i * 82.0f +
                std::sin(
                    g.time + i
                ) *
                16.0f;

            const float y =
                105.0f +
                (i % 2) *
                115.0f;

            const bool red =
                (i % 2) != 0;

            // Shadow.
            rect(
                x - 2,
                y + 21,
                18,
                5,
                0.04f,
                0.09f,
                0.06f
            );

            rect(
                x,
                y,
                14,
                24,

                red ? 0.82f : 0.14f,
                0.24f,
                red ? 0.14f : 0.82f
            );
        }

        // Ball shadow.
        rect(
            ballX + 2,
            ballY + 6,
            10,
            4,
            0.04f,
            0.08f,
            0.05f
        );

        rect(
            ballX,
            ballY,
            9,
            9,
            0.94f,
            0.92f,
            0.78f
        );

        // Broadcast bars.
        rect(
            8,
            5,
            614,
            21,
            0.025f,
            0.055f,
            0.04f
        );

        rect(
            120,
            326,
            390,
            27,
            0.025f,
            0.055f,
            0.04f
        );

        text(
            18,
            11,
            "TV DISTRIBUIDORA - FUTEBOL DA FIRMA",
            1.70f
        );

        text(
            145,
            333,
            "AZUIS 1 X 1 VERMELHOS",
            1.70f
        );
    }
    else {
        // Slightly illuminated black instead of completely dead screen.
        rect(
            6,
            6,
            618,
            348,
            0.018f,
            0.027f,
            0.027f
        );

        text(
            130,
            160,
            "TV DESLIGADA",
            3.0f,
            0.35f,
            0.42f,
            0.40f
        );
    }

    glDepthFunc(
        GL_LESS
    );

    glPopMatrix();

    // -------------------------------------------------------------------------
    // LED indicator
    // -------------------------------------------------------------------------

    box({
        -4.69f,
        1.00f,
        4.85f,

        0.025f,
        0.025f,
        0.025f,

        g.tvOn ? 0.10f : 0.65f,
        g.tvOn ? 0.80f : 0.05f,
        0.04f
    });

    // Cable below TV.
    box({
        -4.69f,
        0.74f,
        6.20f,

        0.018f,
        0.48f,
        0.018f,

        0.025f,
        0.028f,
        0.028f
    });
}

// =============================================================================
// Back-room floor
// =============================================================================

void backRoomFloor()
{
    for (int x = -5;
         x < 5;
         ++x) {

        for (int z = 4;
             z < 10;
             ++z) {

            texturedBox(
                {
                    x + 0.5f,
                    0.0f,
                    z + 0.5f,

                    0.98f,
                    0.04f,
                    0.98f,

                    0.52f,
                    0.51f,
                    0.46f
                },
                Surface::Tile
            );
        }
    }
}

// =============================================================================
// Annex floor
// =============================================================================

void annexFloor()
{
    for (int x = 5;
         x < 9;
         ++x) {

        for (int z = -4;
             z < 10;
             ++z) {

            texturedBox(
                {
                    x + 0.5f,
                    0.0f,
                    z + 0.5f,

                    0.98f,
                    0.04f,
                    0.98f,

                    0.52f,
                    0.55f,
                    0.47f
                },
                Surface::Tile
            );
        }
    }
}

} // namespace

// =============================================================================
// BACK ROOM
// =============================================================================

void renderBackRoom(const Game& g)
{
    // =========================================================================
    // Locked expansion
    // =========================================================================

    if (!g.expanded) {

        texturedBox(
            {
                0.0f,
                1.80f,
                4.0f,

                10.0f,
                3.60f,
                0.20f,

                0.73f,
                0.69f,
                0.51f
            },
            Surface::Plaster
        );

        // Baseboard.
        wallTrim(
            0.0f,
            0.16f,
            3.87f,

            9.8f,
            0.19f,
            0.08f
        );

        sign(
            4.10f,
            2.80f,
            3.88f,
            "AMPLIACAO",
            1.90f,
            true
        );

        sign(
            4.10f,
            2.45f,
            3.88f,
            "B NO COMPUTADOR",
            1.90f,
            true
        );

        return;
    }

    // =========================================================================
    // Front partition
    // =========================================================================

    texturedBox(
        {
            -1.70f,
            1.80f,
            4.0f,

            6.60f,
            3.60f,
            0.20f,

            0.73f,
            0.69f,
            0.51f
        },
        Surface::Plaster
    );

    texturedBox(
        {
            4.60f,
            1.80f,
            4.0f,

            0.80f,
            3.60f,
            0.20f,

            0.73f,
            0.69f,
            0.51f
        },
        Surface::Plaster
    );

    // Beam over doorway.
    texturedBox(
        {
            2.90f,
            3.15f,
            4.0f,

            2.60f,
            0.90f,
            0.20f,

            0.73f,
            0.69f,
            0.51f
        },
        Surface::Plaster
    );

    // Door frame.
    for (float x : {
        1.91f,
        3.87f
    }) {
        box({
            x,
            1.80f,
            3.86f,

            0.08f,
            3.05f,
            0.09f,

            0.34f,
            0.30f,
            0.22f
        });
    }

    box({
        2.89f,
        3.29f,
        3.86f,

        2.04f,
        0.09f,
        0.09f,

        0.34f,
        0.30f,
        0.22f
    });

    sign(
        3.95f,
        3.40f,
        3.87f,
        "DEPOSITO E DESCANSO",
        2.20f,
        true
    );

    // =========================================================================
    // Floor
    // =========================================================================

    backRoomFloor();

    // =========================================================================
    // Side walls
    // =========================================================================

    for (float x : {
        -5.0f,
        5.0f
    }) {
        if (
            x > 0.0f &&
            g.largeStore
        ) {
            continue;
        }

        texturedBox(
            {
                x,
                1.80f,
                7.0f,

                0.20f,
                3.60f,
                6.0f,

                0.50f,
                0.57f,
                0.49f
            },
            Surface::Plaster
        );
    }

    // =========================================================================
    // Rear wall
    // =========================================================================

    texturedBox(
        {
            0.0f,
            1.80f,
            10.0f,

            10.0f,
            3.60f,
            0.20f,

            0.53f,
            0.60f,
            0.50f
        },
        Surface::Plaster
    );

    wallTrim(
        0.0f,
        0.18f,
        9.86f,

        9.7f,
        0.18f,
        0.08f
    );

    // Ceiling.
    box({
        0.0f,
        3.70f,
        7.0f,

        10.0f,
        0.18f,
        6.0f,

        0.30f,
        0.32f,
        0.27f
    });

    // =========================================================================
    // Lighting
    // =========================================================================

    ceilingLamp(
        0.0f,
        6.30f,
        2.0f
    );

    ceilingLamp(
        -3.0f,
        8.10f,
        1.7f
    );

    // =========================================================================
    // Storage shelves
    // =========================================================================

    const int storageProducts =
        std::min(
            4,
            g.availableProducts()
        );

    for (int product = 0;
         product < storageProducts;
         ++product) {

        const float x =
            -3.60f +
            product * 2.10f;

        storageShelf(
            g,
            product,
            x
        );
    }

    // =========================================================================
    // Rest area
    // =========================================================================

    sofa();

    sofaComputerTable(g);

    television(g);
}

// =============================================================================
// STAFF
// =============================================================================

void renderHelper(const Game& g)
{
    constexpr std::array<int, 5> looks{
        1,
        0,
        6,
        5,
        3
    };

    const int count =
        std::min(
            g.checkoutCount(),
            static_cast<int>(
                looks.size()
            )
        );

    for (int lane = 0;
         lane < count;
         ++lane) {

        const auto& helper =
            g.staff(lane);

        if (!helper.hired)
            continue;

        glPushMatrix();

        glTranslatef(
            helper.x,
            0.0f,
            helper.z
        );

        glScalef(
            1.0f / shopScaleX,
            1.0f,
            1.0f / shopScaleZ
        );

        glRotatef(
            helper.yaw *
                RAD2DEG,
            0.0f,
            1.0f,
            0.0f
        );

        float reach = 0.0f;

        if (helper.gesture > 0.0f) {

            const float progress =
                std::clamp(
                    helper.gesture /
                    0.55f,
                    0.0f,
                    1.0f
                );

            reach =
                std::sin(
                    progress *
                    PI
                );
        }

        character(
            looks[
                static_cast<std::size_t>(
                    lane
                )
            ],

            helper.walkCycle,

            helper.route.empty()
                ? 0.0f
                : 1.0f,

            helper.held >= 0
                ? 1.0f
                : 0.0f,

            reach,

            true
        );

        // ---------------------------------------------------------------------
        // Carried goods
        // ---------------------------------------------------------------------

        if (
            helper.held >= 0 &&
            helper.held <
                g.availableProducts()
        ) {

            glTranslatef(
                0.0f,
                0.0f,
                reach * 0.16f
            );

            if (helper.packed) {

                const int crates =
                    std::min(
                        3,
                        std::max(
                            1,
                            (helper.count + 11) /
                            12
                        )
                    );

                for (int i = 0;
                     i < crates;
                     ++i) {

                    crate(
                        helper.held,

                        0.0f,

                        1.03f +
                            i * 0.23f,

                        0.45f,

                        0.63f
                    );
                }
            }
            else {

                const int visibleItems =
                    std::min(
                        helper.count,
                        9
                    );

                if (helper.count > 1) {
                    texturedBox(
                        {
                            0.0f,
                            1.04f,
                            0.46f,

                            0.46f,
                            0.29f,
                            0.32f,

                            0.74f,
                            0.63f,
                            0.40f
                        },
                        Surface::Wood
                    );
                }

                for (int i = 0;
                     i < visibleItems;
                     ++i) {

                    item(
                        helper.held,

                        -0.12f +
                            (i % 3) *
                            0.12f,

                        1.22f +
                            (i / 3) *
                            0.15f,

                        0.46f,

                        0.50f
                    );
                }
            }
        }

        glPopMatrix();
    }
}

// =============================================================================
// LARGE STORE ANNEX
// =============================================================================

void renderAnnex(const Game& g)
{
    if (!g.largeStore)
        return;

    // =========================================================================
    // Floor
    // =========================================================================

    annexFloor();

    // =========================================================================
    // Outer walls
    // =========================================================================

    texturedBox(
        {
            9.0f,
            1.80f,
            3.0f,

            0.20f,
            3.60f,
            14.0f,

            0.61f,
            0.67f,
            0.52f
        },
        Surface::Plaster
    );

    texturedBox(
        {
            7.0f,
            1.80f,
            10.0f,

            4.0f,
            3.60f,
            0.20f,

            0.53f,
            0.60f,
            0.50f
        },
        Surface::Plaster
    );

    // Ceiling.
    box({
        7.0f,
        3.70f,
        3.0f,

        4.0f,
        0.18f,
        14.0f,

        0.29f,
        0.32f,
        0.28f
    });

    // =========================================================================
    // Checkout / wholesale counter
    // =========================================================================

    texturedBox(
        {
            7.0f,
            0.55f,
            -2.60f,

            4.0f,
            1.10f,
            0.90f,

            0.48f,
            0.35f,
            0.23f
        },
        Surface::Wood
    );

    texturedBox(
        {
            7.0f,
            1.15f,
            -2.60f,

            4.0f,
            0.15f,
            1.10f,

            0.66f,
            0.53f,
            0.34f
        },
        Surface::Wood
    );

    // Counter front accent.
    box({
        7.0f,
        0.56f,
        -3.07f,

        3.78f,
        0.72f,
        0.04f,

        0.12f,
        0.22f,
        0.20f
    });

    sign(
        5.50f,
        3.30f,
        -2.84f,
        "ATACADO - GOIANÃO",
        3.0f
    );

    // =========================================================================
    // Partition
    // =========================================================================

    texturedBox(
        {
            6.70f,
            1.80f,
            4.0f,

            4.60f,
            3.60f,
            0.15f,

            0.53f,
            0.60f,
            0.50f
        },
        Surface::Plaster
    );

    // =========================================================================
    // Lighting
    // =========================================================================

    ceilingLamp(
        6.80f,
        0.0f,
        1.90f
    );

    ceilingLamp(
        6.80f,
        6.40f,
        1.90f
    );

    ceilingLamp(
        6.80f,
        8.40f,
        1.70f
    );

    // =========================================================================
    // Rear product storage
    // =========================================================================

    for (int product = 4;
         product < 6;
         ++product) {

        if (
            product >=
            g.availableProducts()
        ) {
            continue;
        }

        const float x =
            product == 4
                ? 5.90f
                : 8.00f;

        storageShelf(
            g,
            product,
            x
        );
    }

    // =========================================================================
    // Wall trim
    // =========================================================================

    wallTrim(
        8.87f,
        0.18f,
        3.0f,

        0.06f,
        0.18f,
        13.6f
    );

    wallTrim(
        7.0f,
        0.18f,
        9.86f,

        3.7f,
        0.18f,
        0.08f
    );
}