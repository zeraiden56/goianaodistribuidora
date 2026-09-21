#include "computer.hpp"
#include "render.hpp"
#include "shop_layout.hpp"
#include <SDL_opengl.h>
#include <cmath>

void renderTerminal(const Game& g,int terminal) {
    const auto& location=terminals[terminal];
    glPushMatrix();glTranslatef(location.x,location.y,location.z);
    glScalef(location.scale/shopScaleX,location.scale,location.scale/shopScaleZ);
    // Local front is +Z. Rotating the whole assembly points display, keys and mouse at the grade.
    glRotatef(180,0,1,0);
    box({0,0,-.095f,1.27f,.79f,.22f,.73f,.74f,.65f});
    box({0,.02f,-.26f,1.04f,.64f,.17f,.56f,.59f,.54f});
    box({0,0,.02f,1.15f,.67f,.028f,.055f,.078f,.078f});
    for(float side:{-1.f,1.f})box({side*.598f,0,.042f,.056f,.71f,.032f,.86f,.86f,.75f});
    for(float side:{-1.f,1.f})box({0,side*.355f,.039f,1.2f,.038f,.033f,.84f,.84f,.74f});
    box({0,-.46f,-.12f,.16f,.22f,.14f,.49f,.55f,.52f});
    box({0,-.51f,-.055f,.55f,.045f,.35f,.62f,.65f,.58f});
    for(int i=0;i<9;++i)box({-.37f+i*.09f,.18f,-.353f,.039f,.24f,.009f,.12f,.16f,.16f});
    box({.48f,-.365f,.068f,.045f,.021f,.012f,.21f,.91f,.45f});
    for(float x:{.31f,.38f})box({x,-.365f,.065f,.043f,.024f,.013f,.37f,.42f,.4f});
    // Keyboard with separate keys, space bar, mouse and connected cables.
    box({-.06f,-.49f,.42f,.82f,.055f,.27f,.69f,.7f,.63f});
    for(int row=0;row<4;++row)for(int col=0;col<12;++col)
        box({-.41f+col*.06f,-.452f,.335f+row*.051f,.049f,.019f,.04f,.83f,.82f,.73f});
    box({-.035f,-.431f,.5f,.28f,.015f,.034f,.48f,.51f,.47f});
    box({.54f,-.5f,.43f,.27f,.015f,.34f,.12f,.2f,.21f});
    box({.54f,-.46f,.43f,.12f,.07f,.17f,.74f,.75f,.68f});
    box({.54f,-.418f,.398f,.008f,.012f,.073f,.23f,.27f,.26f});
    rod(.54f,-.472f,.33f,.64f,-.47f,.07f,.01f,.14f,.16f,.15f);
    rod(-.2f,-.47f,.27f,-.48f,-.47f,-.1f,.012f,.14f,.16f,.15f);
    box({.88f,-.17f,-.12f,.3f,.65f,.57f,.28f,.34f,.33f});
    box({.88f,-.17f,.174f,.266f,.59f,.023f,.64f,.67f,.59f});
    box({.88f,.035f,.192f,.21f,.052f,.018f,.16f,.21f,.2f});
    for(int i=0;i<6;++i)box({.88f,-.31f+i*.028f,.193f,.2f,.012f,.015f,.16f,.21f,.2f});
    box({.94f,-.08f,.195f,.04f,.04f,.016f,.18f,.75f,.44f});
    box({.86f,-.11f,.197f,.013f,.013f,.012f,.65f+.3f*std::sin(g.time*7),.52f,.11f});
    rod(.4f,-.25f,-.15f,.69f,-.41f,-.29f,.017f,.12f,.15f,.15f);
    // Render the real live desktop onto the display: the same inventory, balance and clock.
    glPushMatrix();glTranslatef(-.55f,.309375f,.042f);glScalef(1.1f/960,-.61875f/540,1);
    // Keep the world depth test, but do not let coplanar UI layers occlude each other.
    glDepthMask(GL_FALSE);renderComputer(g,ComputerUI{});glDepthMask(GL_TRUE);glPopMatrix();
    glPopMatrix();
}
