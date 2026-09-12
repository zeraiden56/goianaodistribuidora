#pragma once
#include <string>
#include <vector>
struct Game;
enum class HelperTask {Idle,Fetching,Delivering,Returning};
struct HelperPoint {float x,z;};
struct HelperState {
    bool hired=false;
    float x=-1.2f,z=-.9f,wait=0;
    int held=-1,target=-1,count=0;
    HelperTask task=HelperTask::Idle;
    // Navigation is rebuilt after loading; economic state is serialized.
    std::vector<HelperPoint> route;
};
void tickHelper(Game& game,float dt,int lane=0);
std::string helperStatus(const Game& game);
