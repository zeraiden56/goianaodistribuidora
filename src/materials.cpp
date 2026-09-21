#include "render.hpp"

#include <SDL_opengl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr int TEXTURE_SIZE = 64;
constexpr int SURFACE_COUNT = 5;

constexpr float PI =
    3.14159265358979323846f;

constexpr float TWO_PI =
    PI * 2.0f;

// One OpenGL texture per procedural material.
std::array<GLuint, SURFACE_COUNT> textures{};

// =============================================================================
// Surface mapping
//
// Don't depend on the numerical values of the Surface enum.
// =============================================================================

int surfaceSlot(Surface surface)
{
    switch (surface) {
        case Surface::Plaster:
            return 0;

        case Surface::Tile:
            return 1;

        case Surface::Brick:
            return 2;

        case Surface::Wood:
            return 3;

        case Surface::Asphalt:
            return 4;

        default:
            return 0;
    }
}

// =============================================================================
// Random / noise helpers
// =============================================================================

std::uint32_t hash32(std::uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;

    return x;
}

float hashNoise(
    int x,
    int y,
    std::uint32_t seed)
{
    const std::uint32_t h =
        hash32(
            static_cast<std::uint32_t>(x) *
                0x9e3779b9u ^
            static_cast<std::uint32_t>(y) *
                0x85ebca6bu ^
            seed
        );

    return
        static_cast<float>(
            h & 0xffffu
        ) /
        65535.0f;
}

float smoothCurve(float t)
{
    return
        t *
        t *
        (3.0f - 2.0f * t);
}

// Periodic value noise.
// Periodicity makes repeating GL textures much less obvious at their borders.
float periodicNoise(
    float x,
    float y,
    int period,
    std::uint32_t seed)
{
    const int ix =
        static_cast<int>(
            std::floor(x)
        );

    const int iy =
        static_cast<int>(
            std::floor(y)
        );

    float fx =
        x - std::floor(x);

    float fy =
        y - std::floor(y);

    fx = smoothCurve(fx);
    fy = smoothCurve(fy);

    auto wrap =
        [period](int value)
        {
            value %= period;

            if (value < 0)
                value += period;

            return value;
        };

    const int x0 = wrap(ix);
    const int y0 = wrap(iy);

    const int x1 = wrap(ix + 1);
    const int y1 = wrap(iy + 1);

    const float a =
        hashNoise(x0, y0, seed);

    const float b =
        hashNoise(x1, y0, seed);

    const float c =
        hashNoise(x0, y1, seed);

    const float d =
        hashNoise(x1, y1, seed);

    const float top =
        a + (b - a) * fx;

    const float bottom =
        c + (d - c) * fx;

    return
        top +
        (bottom - top) * fy;
}

float fractalNoise(
    float x,
    float y,
    std::uint32_t seed)
{
    float value = 0.0f;
    float weight = 0.0f;

    float amplitude = 1.0f;

    for (int octave = 0;
         octave < 3;
         ++octave) {

        const int period =
            4 << octave;

        value +=
            periodicNoise(
                x * period,
                y * period,
                period,
                seed +
                    static_cast<std::uint32_t>(
                        octave * 197
                    )
            ) *
            amplitude;

        weight += amplitude;

        amplitude *= 0.5f;
    }

    if (weight <= 0.0f)
        return 0.0f;

    return value / weight;
}

// =============================================================================
// Texture pixel
// =============================================================================

struct Pixel {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
};

Pixel multiply(
    Pixel value,
    float amount)
{
    value.r *= amount;
    value.g *= amount;
    value.b *= amount;

    return value;
}

Pixel mixPixel(
    const Pixel& a,
    const Pixel& b,
    float t)
{
    t =
        std::clamp(
            t,
            0.0f,
            1.0f
        );

    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t
    };
}

void storePixel(
    std::array<
        unsigned char,
        TEXTURE_SIZE *
        TEXTURE_SIZE *
        3
    >& data,

    int x,
    int y,
    Pixel pixel)
{
    pixel.r =
        std::clamp(
            pixel.r,
            0.0f,
            1.0f
        );

    pixel.g =
        std::clamp(
            pixel.g,
            0.0f,
            1.0f
        );

    pixel.b =
        std::clamp(
            pixel.b,
            0.0f,
            1.0f
        );

    const std::size_t index =
        static_cast<std::size_t>(
            (y * TEXTURE_SIZE + x) *
            3
        );

    data[index + 0] =
        static_cast<unsigned char>(
            pixel.r * 255.0f
        );

    data[index + 1] =
        static_cast<unsigned char>(
            pixel.g * 255.0f
        );

    data[index + 2] =
        static_cast<unsigned char>(
            pixel.b * 255.0f
        );
}

// =============================================================================
// PLASTER
// =============================================================================

Pixel plasterPixel(
    int x,
    int y,
    std::uint32_t seed)
{
    const float u =
        static_cast<float>(x) /
        TEXTURE_SIZE;

    const float v =
        static_cast<float>(y) /
        TEXTURE_SIZE;

    const float largeNoise =
        fractalNoise(
            u,
            v,
            seed
        );

    const float tinyNoise =
        hashNoise(
            x,
            y,
            seed + 983u
        );

    float value =
        0.84f +
        (largeNoise - 0.5f) *
            0.12f +
        (tinyNoise - 0.5f) *
            0.035f;

    // Occasional tiny pores.
    if (tinyNoise < 0.025f)
        value -= 0.10f;

    Pixel result{
        value * 1.00f,
        value * 0.995f,
        value * 0.965f
    };

    return result;
}

// =============================================================================
// TILE
// =============================================================================

Pixel tilePixel(
    int x,
    int y,
    std::uint32_t seed)
{
    constexpr int tileSize = 16;
    constexpr int grout = 2;

    const int localX =
        x % tileSize;

    const int localY =
        y % tileSize;

    // Grout.
    if (
        localX < grout ||
        localY < grout
    ) {
        const float noise =
            hashNoise(
                x,
                y,
                seed
            );

        const float value =
            0.45f +
            noise * 0.045f;

        return {
            value * 0.93f,
            value * 0.95f,
            value
        };
    }

    const int tileX =
        x / tileSize;

    const int tileY =
        y / tileSize;

    const float tileVariation =
        hashNoise(
            tileX,
            tileY,
            seed + 100u
        );

    const float micro =
        hashNoise(
            x,
            y,
            seed + 712u
        );

    float value =
        0.88f +
        (tileVariation - 0.5f) *
            0.045f +
        (micro - 0.5f) *
            0.018f;

    // Slight glazed highlight near one tile edge.
    const float normalizedX =
        static_cast<float>(
            localX
        ) /
        tileSize;

    const float normalizedY =
        static_cast<float>(
            localY
        ) /
        tileSize;

    const float shine =
        std::max(
            0.0f,
            1.0f -
            std::abs(
                normalizedX -
                normalizedY
            ) *
            7.0f
        );

    value +=
        shine *
        0.025f;

    return {
        value * 0.94f,
        value * 0.975f,
        value
    };
}

// =============================================================================
// BRICK
// =============================================================================

Pixel brickPixel(
    int x,
    int y,
    std::uint32_t seed)
{
    constexpr int brickWidth = 32;
    constexpr int brickHeight = 16;

    constexpr int mortar = 2;

    const int row =
        y / brickHeight;

    const int shiftedX =
        (
            x +
            ((row & 1)
                ? brickWidth / 2
                : 0)
        ) %
        TEXTURE_SIZE;

    const int localX =
        shiftedX %
        brickWidth;

    const int localY =
        y %
        brickHeight;

    if (
        localX < mortar ||
        localY < mortar
    ) {
        const float n =
            hashNoise(
                x,
                y,
                seed + 17u
            );

        const float value =
            0.44f +
            n * 0.055f;

        return {
            value,
            value * 0.98f,
            value * 0.91f
        };
    }

    const int brickX =
        shiftedX /
        brickWidth;

    const float brickVariation =
        hashNoise(
            brickX,
            row,
            seed + 300u
        );

    const float grain =
        hashNoise(
            x,
            y,
            seed + 913u
        );

    float value =
        0.77f +
        (brickVariation - 0.5f) *
            0.13f +
        (grain - 0.5f) *
            0.055f;

    // Slight darkening near brick edges.
    const int distanceX =
        std::min(
            localX,
            brickWidth - localX
        );

    const int distanceY =
        std::min(
            localY,
            brickHeight - localY
        );

    if (
        distanceX < 4 ||
        distanceY < 4
    ) {
        value *= 0.94f;
    }

    return {
        value,
        value * 0.91f,
        value * 0.85f
    };
}

// =============================================================================
// WOOD
// =============================================================================

Pixel woodPixel(
    int x,
    int y,
    std::uint32_t seed)
{
    const float u =
        static_cast<float>(x) /
        TEXTURE_SIZE;

    const float v =
        static_cast<float>(y) /
        TEXTURE_SIZE;

    const float distortion =
        fractalNoise(
            u,
            v,
            seed
        );

    const float fineNoise =
        hashNoise(
            x,
            y,
            seed + 135u
        );

    // Long organic grain.
    const float grainPhase =
        v * 48.0f +
        std::sin(
            u * TWO_PI * 1.7f
        ) *
        2.7f +
        distortion *
        5.0f;

    const float grain =
        0.5f +
        0.5f *
        std::sin(
            grainPhase
        );

    const float fineGrain =
        0.5f +
        0.5f *
        std::sin(
            grainPhase * 3.15f
        );

    float value =
        0.66f +
        grain * 0.15f +
        fineGrain * 0.035f +
        (fineNoise - 0.5f) *
            0.035f;

    // Knot.
    const float dx =
        u - 0.67f;

    const float dy =
        v - 0.41f;

    const float knotDistance =
        std::sqrt(
            dx * dx * 3.0f +
            dy * dy * 7.0f
        );

    if (knotDistance < 0.14f) {
        const float ring =
            0.5f +
            0.5f *
            std::sin(
                knotDistance *
                150.0f
            );

        value =
            value * 0.78f +
            ring * 0.09f;
    }

    // Neutral-warm procedural texture.
    // Box color still multiplies this.
    return {
        value,
        value * 0.92f,
        value * 0.79f
    };
}

// =============================================================================
// ASPHALT
// =============================================================================

Pixel asphaltPixel(
    int x,
    int y,
    std::uint32_t seed)
{
    const float u =
        static_cast<float>(x) /
        TEXTURE_SIZE;

    const float v =
        static_cast<float>(y) /
        TEXTURE_SIZE;

    const float macro =
        fractalNoise(
            u,
            v,
            seed
        );

    const float grain =
        hashNoise(
            x,
            y,
            seed + 616u
        );

    float value =
        0.53f +
        (macro - 0.5f) *
            0.13f +
        (grain - 0.5f) *
            0.12f;

    // Aggregate stones.
    if (grain > 0.94f)
        value += 0.14f;

    if (grain < 0.035f)
        value -= 0.09f;

    return {
        value * 0.93f,
        value * 0.95f,
        value
    };
}

// =============================================================================
// Texture generation
// =============================================================================

Pixel generatePixel(
    Surface surface,
    int x,
    int y,
    std::uint32_t seed)
{
    switch (surface) {

        case Surface::Plaster:
            return plasterPixel(
                x,
                y,
                seed
            );

        case Surface::Tile:
            return tilePixel(
                x,
                y,
                seed
            );

        case Surface::Brick:
            return brickPixel(
                x,
                y,
                seed
            );

        case Surface::Wood:
            return woodPixel(
                x,
                y,
                seed
            );

        case Surface::Asphalt:
            return asphaltPixel(
                x,
                y,
                seed
            );

        default:
            return {
                1.0f,
                1.0f,
                1.0f
            };
    }
}

// =============================================================================
// Material
// =============================================================================

GLuint material(Surface surface)
{
    const int slot =
        surfaceSlot(surface);

    GLuint& texture =
        textures[
            static_cast<std::size_t>(
                slot
            )
        ];

    if (texture != 0)
        return texture;

    std::array<
        unsigned char,
        TEXTURE_SIZE *
        TEXTURE_SIZE *
        3
    > data{};

    const std::uint32_t seed =
        713u +
        static_cast<std::uint32_t>(
            slot
        ) *
        7919u;

    for (int y = 0;
         y < TEXTURE_SIZE;
         ++y) {

        for (int x = 0;
             x < TEXTURE_SIZE;
             ++x) {

            const Pixel pixel =
                generatePixel(
                    surface,
                    x,
                    y,
                    seed
                );

            storePixel(
                data,
                x,
                y,
                pixel
            );
        }
    }

    glGenTextures(
        1,
        &texture
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture
    );

    // Smooth enough for 3D surfaces but still preserves the procedural detail.
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    // We use RGB = 3 bytes per pixel.
    // Explicit alignment avoids surprises if texture dimensions change.
    GLint oldAlignment = 4;

    glGetIntegerv(
        GL_UNPACK_ALIGNMENT,
        &oldAlignment
    );

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );

    glTexImage2D(
        GL_TEXTURE_2D,

        0,
        GL_RGB,

        TEXTURE_SIZE,
        TEXTURE_SIZE,

        0,

        GL_RGB,
        GL_UNSIGNED_BYTE,

        data.data()
    );

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        oldAlignment
    );

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    return texture;
}

// =============================================================================
// Material UV density
//
// Values indicate how often the procedural texture repeats per world unit.
// =============================================================================

float textureRepeat(Surface surface)
{
    switch (surface) {

        case Surface::Plaster:
            return 0.75f;

        case Surface::Tile:
            return 0.85f;

        case Surface::Brick:
            return 0.70f;

        case Surface::Wood:
            return 0.55f;

        case Surface::Asphalt:
            return 1.25f;

        default:
            return 1.0f;
    }
}

} // namespace

// =============================================================================
// TEXTURED BOX
// =============================================================================

void texturedBox(
    Box a,
    Surface surface)
{
    const GLuint texture =
        material(surface);

    glEnable(
        GL_TEXTURE_2D
    );

    glBindTexture(
        GL_TEXTURE_2D,
        texture
    );

    // Texture * vertex color.
    glTexEnvi(
        GL_TEXTURE_ENV,
        GL_TEXTURE_ENV_MODE,
        GL_MODULATE
    );

    const float x =
        a.x - a.w * 0.5f;

    const float X =
        a.x + a.w * 0.5f;

    const float y =
        a.y - a.h * 0.5f;

    const float Y =
        a.y + a.h * 0.5f;

    const float z =
        a.z - a.d * 0.5f;

    const float Z =
        a.z + a.d * 0.5f;

    const float repeat =
        textureRepeat(surface);

    // -------------------------------------------------------------------------
    // Face renderer
    // -------------------------------------------------------------------------

    auto face =
        [&](const std::array<
                std::array<float, 3>,
                4
            >& points,

            float u,
            float v,

            float shade,

            float nx,
            float ny,
            float nz)
        {
            glColor3f(
                std::clamp(
                    a.r * shade,
                    0.0f,
                    1.0f
                ),

                std::clamp(
                    a.g * shade,
                    0.0f,
                    1.0f
                ),

                std::clamp(
                    a.b * shade,
                    0.0f,
                    1.0f
                )
            );

            glBegin(
                GL_QUADS
            );

            glNormal3f(
                nx,
                ny,
                nz
            );

            const float uu =
                std::max(
                    0.01f,
                    u * repeat
                );

            const float vv =
                std::max(
                    0.01f,
                    v * repeat
                );

            glTexCoord2f(
                0.0f,
                0.0f
            );

            glVertex3fv(
                points[0].data()
            );

            glTexCoord2f(
                uu,
                0.0f
            );

            glVertex3fv(
                points[1].data()
            );

            glTexCoord2f(
                uu,
                vv
            );

            glVertex3fv(
                points[2].data()
            );

            glTexCoord2f(
                0.0f,
                vv
            );

            glVertex3fv(
                points[3].data()
            );

            glEnd();
        };

    // -------------------------------------------------------------------------
    // Front
    // -------------------------------------------------------------------------

    face(
        {{
            {{x, y, Z}},
            {{X, y, Z}},
            {{X, Y, Z}},
            {{x, Y, Z}}
        }},

        a.w,
        a.h,

        1.00f,

        0.0f,
        0.0f,
        1.0f
    );

    // -------------------------------------------------------------------------
    // Back
    // -------------------------------------------------------------------------

    face(
        {{
            {{X, y, z}},
            {{x, y, z}},
            {{x, Y, z}},
            {{X, Y, z}}
        }},

        a.w,
        a.h,

        0.82f,

        0.0f,
        0.0f,
        -1.0f
    );

    // -------------------------------------------------------------------------
    // Left
    // -------------------------------------------------------------------------

    face(
        {{
            {{x, y, z}},
            {{x, y, Z}},
            {{x, Y, Z}},
            {{x, Y, z}}
        }},

        a.d,
        a.h,

        0.88f,

        -1.0f,
        0.0f,
        0.0f
    );

    // -------------------------------------------------------------------------
    // Right
    // -------------------------------------------------------------------------

    face(
        {{
            {{X, y, Z}},
            {{X, y, z}},
            {{X, Y, z}},
            {{X, Y, Z}}
        }},

        a.d,
        a.h,

        0.91f,

        1.0f,
        0.0f,
        0.0f
    );

    // -------------------------------------------------------------------------
    // Top
    // -------------------------------------------------------------------------

    face(
        {{
            {{x, Y, Z}},
            {{X, Y, Z}},
            {{X, Y, z}},
            {{x, Y, z}}
        }},

        a.w,
        a.d,

        1.06f,

        0.0f,
        1.0f,
        0.0f
    );

    // -------------------------------------------------------------------------
    // Bottom
    //
    // This face was missing from the original implementation.
    // -------------------------------------------------------------------------

    face(
        {{
            {{x, y, z}},
            {{X, y, z}},
            {{X, y, Z}},
            {{x, y, Z}}
        }},

        a.w,
        a.d,

        0.72f,

        0.0f,
        -1.0f,
        0.0f
    );

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    glDisable(
        GL_TEXTURE_2D
    );
}

// =============================================================================
// DESTROY PROCEDURAL MATERIALS
// =============================================================================

void destroyMaterials()
{
    glDeleteTextures(
        static_cast<GLsizei>(
            textures.size()
        ),
        textures.data()
    );

    textures.fill(0);
}

// =============================================================================
// CYLINDER
// =============================================================================

void cylinder(
    float x,
    float y,
    float z,

    float radius,
    float height,

    float r,
    float g,
    float b,

    float topRadius)
{
    if (
        radius <= 0.0f ||
        height <= 0.0f
    ) {
        return;
    }

    if (topRadius < 0.0f)
        topRadius = radius;

    topRadius =
        std::max(
            0.0f,
            topRadius
        );

    constexpr int segments = 16;

    const float bottomY =
        y - height * 0.5f;

    const float topY =
        y + height * 0.5f;

    // Used to slightly tilt normals on tapered cylinders.
    const float slope =
        (radius - topRadius) /
        height;

    // =========================================================================
    // Side wall
    // =========================================================================

    glBegin(
        GL_QUAD_STRIP
    );

    for (int i = 0;
         i <= segments;
         ++i) {

        const float angle =
            static_cast<float>(i) *
            TWO_PI /
            static_cast<float>(
                segments
            );

        const float c =
            std::cos(angle);

        const float s =
            std::sin(angle);

        // Fake directional lighting used by the rest of this renderer.
        const float shade =
            0.80f +
            0.20f *
            c;

        glColor3f(
            std::clamp(
                r * shade,
                0.0f,
                1.0f
            ),

            std::clamp(
                g * shade,
                0.0f,
                1.0f
            ),

            std::clamp(
                b * shade,
                0.0f,
                1.0f
            )
        );

        // Proper approximate normal even for a tapered cylinder.
        const float normalLength =
            std::sqrt(
                1.0f +
                slope * slope
            );

        glNormal3f(
            c / normalLength,
            slope / normalLength,
            s / normalLength
        );

        glVertex3f(
            x + c * radius,
            bottomY,
            z + s * radius
        );

        glVertex3f(
            x + c * topRadius,
            topY,
            z + s * topRadius
        );
    }

    glEnd();

    // =========================================================================
    // Top
    // =========================================================================

    if (topRadius > 0.0f) {

        glColor3f(
            std::clamp(
                r * 1.05f,
                0.0f,
                1.0f
            ),

            std::clamp(
                g * 1.05f,
                0.0f,
                1.0f
            ),

            std::clamp(
                b * 1.05f,
                0.0f,
                1.0f
            )
        );

        glBegin(
            GL_TRIANGLE_FAN
        );

        glNormal3f(
            0.0f,
            1.0f,
            0.0f
        );

        glVertex3f(
            x,
            topY,
            z
        );

        for (int i = 0;
             i <= segments;
             ++i) {

            const float angle =
                static_cast<float>(i) *
                TWO_PI /
                static_cast<float>(
                    segments
                );

            glVertex3f(
                x +
                    std::cos(angle) *
                    topRadius,

                topY,

                z +
                    std::sin(angle) *
                    topRadius
            );
        }

        glEnd();
    }

    // =========================================================================
    // Bottom
    //
    // Also missing in the old implementation.
    // Reverse winding to face downward.
    // =========================================================================

    if (radius > 0.0f) {

        glColor3f(
            r * 0.72f,
            g * 0.72f,
            b * 0.72f
        );

        glBegin(
            GL_TRIANGLE_FAN
        );

        glNormal3f(
            0.0f,
            -1.0f,
            0.0f
        );

        glVertex3f(
            x,
            bottomY,
            z
        );

        for (int i = segments;
             i >= 0;
             --i) {

            const float angle =
                static_cast<float>(i) *
                TWO_PI /
                static_cast<float>(
                    segments
                );

            glVertex3f(
                x +
                    std::cos(angle) *
                    radius,

                bottomY,

                z +
                    std::sin(angle) *
                    radius
            );
        }

        glEnd();
    }
}

// =============================================================================
// ROD
// =============================================================================

void rod(
    float ax,
    float ay,
    float az,

    float bx,
    float by,
    float bz,

    float radius,

    float r,
    float g,
    float b)
{
    const float dx =
        bx - ax;

    const float dy =
        by - ay;

    const float dz =
        bz - az;

    const float length =
        std::sqrt(
            dx * dx +
            dy * dy +
            dz * dz
        );

    if (
        length < 0.0001f ||
        radius <= 0.0f
    ) {
        return;
    }

    glPushMatrix();

    glTranslatef(
        (ax + bx) * 0.5f,
        (ay + by) * 0.5f,
        (az + bz) * 0.5f
    );

    const float horizontal =
        std::hypot(
            dx,
            dz
        );

    if (horizontal > 0.0001f) {

        const float cosine =
            std::clamp(
                dy / length,
                -1.0f,
                1.0f
            );

        const float angle =
            std::acos(cosine) *
            180.0f /
            PI;

        // Cross product between +Y and target direction:
        //
        // (0,1,0) x (dx,dy,dz)
        // = (dz,0,-dx)
        glRotatef(
            angle,

            dz,
            0.0f,
            -dx
        );
    }
    else if (dy < 0.0f) {

        glRotatef(
            180.0f,
            1.0f,
            0.0f,
            0.0f
        );
    }

    cylinder(
        0.0f,
        0.0f,
        0.0f,

        radius,
        length,

        r,
        g,
        b
    );

    glPopMatrix();
}