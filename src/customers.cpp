#include "customers.hpp"
const std::array<CustomerLook,customerLookCount>& customerLooks() {
    static const std::array<CustomerLook,customerLookCount> looks{{
        {"ANA",{.69f,.46f,.31f},{.72f,.3f,.23f},{.16f,.2f,.31f},{.12f,.075f,.04f},.94f,.95f,0},
        {"CARLOS",{.4f,.25f,.16f},{.19f,.42f,.62f},{.12f,.15f,.22f},{.055f,.045f,.035f},1.05f,1.14f,1},
        {"MARCIA",{.8f,.6f,.43f},{.45f,.25f,.59f},{.16f,.17f,.2f},{.56f,.49f,.4f},.96f,1.08f,2},
        {"RAFAEL",{.6f,.38f,.23f},{.24f,.5f,.28f},{.2f,.23f,.31f},{.11f,.065f,.03f},1.1f,1.f,3},
        {"JOAO",{.82f,.62f,.47f},{.62f,.49f,.23f},{.14f,.2f,.26f},{.25f,.14f,.065f},1.02f,1.2f,4},
        {"BRUNA",{.36f,.22f,.14f},{.73f,.55f,.22f},{.12f,.17f,.27f},{.035f,.03f,.025f},.98f,1.f,5},
        {"PAULO",{.58f,.38f,.25f},{.69f,.69f,.59f},{.19f,.22f,.24f},{.48f,.46f,.4f},1.06f,1.1f,6},
        {"LUIZA",{.75f,.52f,.35f},{.2f,.58f,.53f},{.27f,.19f,.3f},{.35f,.15f,.06f},1.f,.96f,7}
    }};
    return looks;
}
