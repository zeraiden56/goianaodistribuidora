#pragma once
#include <array>
struct CustomerLook {
    const char* name;
    std::array<float,3> skin,shirt,trousers,hair;
    float height,width;
    int style;
};
inline constexpr int customerLookCount=8;
const std::array<CustomerLook,customerLookCount>& customerLooks();
