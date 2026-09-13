#include "helper.hpp"
#include "game.hpp"
#include "player.hpp"
#include "shop_layout.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>

namespace {
constexpr int columns=53,rows=17,nodes=columns*rows;
HelperPoint point(int index){return {-4.5f+(index%columns)*.25f,-1.5f+(index/columns)*.25f};}
bool clearSegment(HelperPoint from,HelperPoint to,bool large) {
    int steps=std::max(1,int(std::hypot(to.x-from.x,to.z-from.z)/.06f)+1);
    for(int i=0;i<=steps;++i){float f=float(i)/steps;if(!walkable(from.x+(to.x-from.x)*f,from.z+(to.z-from.z)*f,large,large))return false;}
    return true;
}
int nearest(HelperPoint position,bool large) {
    int selected=-1;float distance=std::numeric_limits<float>::max();
    for(int i=0;i<nodes;++i) {
        auto p=point(i);float d=std::hypot(p.x-position.x,p.z-position.z);
        if(d<distance&&walkable(p.x,p.z,large,large)&&clearSegment(position,p,large)){selected=i;distance=d;}
    }
    return selected;
}
void plan(HelperState& helper,HelperPoint destination,bool large) {
    int start=nearest({helper.x,helper.z},large),goal=nearest(destination,large);if(start<0||goal<0)return;
    std::array<int,nodes> parent;parent.fill(-1);parent[start]=start;std::queue<int> queue;queue.push(start);
    while(!queue.empty()&&parent[goal]<0) {
        int current=queue.front();queue.pop();
        for(int delta:{-1,1,-columns,columns}) {
            int next=current+delta;
            if(next<0||next>=nodes||(std::abs(delta)==1&&next/columns!=current/columns)||parent[next]>=0)continue;
            auto p=point(next);if(!walkable(p.x,p.z,large,large)||!clearSegment(point(current),p,large))continue;
            parent[next]=current;queue.push(next);
        }
    }
    if(parent[goal]<0)return;
    std::vector<HelperPoint> reversed;
    for(int at=goal;at!=start;at=parent[at])reversed.push_back(point(at));
    reversed.push_back(point(start));helper.route.assign(reversed.rbegin(),reversed.rend());helper.route.push_back(destination);
}
bool travel(HelperState& helper,HelperPoint goal,float dt,bool large,float speed) {
    if(std::hypot(helper.x-goal.x,helper.z-goal.z)<.025f)return true;
    if(helper.route.empty())plan(helper,goal,large);
    float remaining=std::clamp(dt,0.f,.1f)*speed;
    while(remaining>0&&!helper.route.empty()) {
        auto next=helper.route.front();float distance=std::hypot(next.x-helper.x,next.z-helper.z);
        if(distance<.002f){helper.route.erase(helper.route.begin());continue;}
        float step=std::min(remaining,distance),x=helper.x+(next.x-helper.x)*step/distance,z=helper.z+(next.z-helper.z)*step/distance;
        if(!walkable(x,z,large,large)){helper.route.clear();return false;}
        helper.x=x;helper.z=z;remaining-=step;
        if(step==distance)helper.route.erase(helper.route.begin());
    }
    return std::hypot(helper.x-goal.x,helper.z-goal.z)<.025f;
}
void task(HelperState& h,HelperTask next,int target){h.task=next;h.target=target;h.route.clear();}
HelperPoint shelf(int product) {
    return {stockLocations[product].workerX,stockLocations[product].workerZ};
}
}
void tickHelper(Game& g,float dt,int lane) {
    auto& h=g.staff(lane);
    const auto& customer=lane>=2?g.extraCheckout(lane).customer:lane?g.second.customer:g.customer;
    const auto& wanted=lane>=2?g.extraCheckout(lane).wanted:lane?g.second.wanted:g.wanted;
    const auto& quantity=lane>=2?g.extraCheckout(lane).quantity:lane?g.second.quantity:g.quantity;
    const auto& delivered=lane>=2?g.extraCheckout(lane).delivered:lane?g.second.delivered:g.delivered;if(!h.hired)return;
    h.wait=std::max(0.f,h.wait-dt);if(h.wait>0)return;
    if(h.held>=0&&(!customer||wanted!=h.held)&&h.task!=HelperTask::Returning)task(h,HelperTask::Returning,h.held);
    if(h.task==HelperTask::Fetching&&(!customer||h.target!=wanted))task(h,HelperTask::Idle,-1);
    if(h.task==HelperTask::Idle) {
        if(customer&&g.products[wanted].stock>0)task(h,HelperTask::Fetching,wanted);
        else return;
    }
    if(h.task==HelperTask::Fetching) {
        if(!travel(h,shelf(h.target),dt,g.largeStore,g.staffSpeed()))return;
        if(g.products[h.target].stock>0) {
            h.packed=lane>=2?g.extraCheckout(lane).wholesale:lane?g.second.wholesale:g.wholesale;
            h.count=std::min({g.bagCapacity*(h.packed?Game::crateUnits:1),quantity-delivered,g.products[h.target].stock});g.products[h.target].stock-=h.count;g.touchStorage(h.target);h.held=h.target;task(h,HelperTask::Delivering,h.target);h.wait=.35f;
        } else task(h,HelperTask::Idle,-1);
    } else if(h.task==HelperTask::Delivering) {
        if(!travel(h,{checkoutX(lane),-1.45f},dt,g.largeStore,g.staffSpeed()))return;
        while(h.count>0&&customer&&wanted==h.held) {
            int unit=h.held;if(!g.deliver(unit,lane))break;--h.count;
        }
        if(h.count==0){h.held=-1;h.packed=false;task(h,HelperTask::Idle,-1);}
        else task(h,HelperTask::Returning,h.held);
        h.wait=.4f;
    } else if(h.task==HelperTask::Returning) {
        if(!travel(h,shelf(h.held),dt,g.largeStore,g.staffSpeed()))return;
        g.products[h.held].stock+=h.count;h.count=0;g.touchStorage(h.held);h.held=-1;h.packed=false;task(h,HelperTask::Idle,-1);h.wait=.2f;
    }
}
std::string helperStatus(const Game& g) {
    if(!g.helper.hired)return "NAO CONTRATADO";
    switch(g.helper.task) {
    case HelperTask::Fetching:return "BUSCANDO "+std::string(g.products[g.helper.target].name);
    case HelperTask::Delivering:return "LEVANDO AO CAIXA";
    case HelperTask::Returning:return "DEVOLVENDO PRODUTO";
    default:return g.customer&&g.products[g.wanted].stock==0?"AGUARDANDO ESTOQUE":"AGUARDANDO CLIENTE";
    }
}
