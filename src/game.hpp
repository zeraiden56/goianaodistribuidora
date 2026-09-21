#pragma once
#include <array>
#include <random>
#include <string>
#include "helper.hpp"
#include "customers.hpp"

struct Product {const char* name; int cost,price,stock,capacity;};
struct Checkout {
    bool customer=false,wholesale=false;
    int wanted=0,quantity=1,delivered=0,customerStyle=1;
    float patience=65,arrival=4;
};
struct MixedOrder {
    std::array<int,6> requested{},delivered{}; // Additional lines; the first line retains the legacy save fields.
};
struct CustomerMotion {
    float approach=0,departure=0;
    int departingStyle=0;
    bool purchased=false;
};
struct Game {
    static constexpr int productCount=6;
    std::array<Product,productCount> products{{{"CERVEJA",4,8,18,48},{"CIGARRO",7,12,10,36},
        {"DESTILADO",18,30,6,24},{"GELO",3,7,12,36},
        {"CERVEJA LATA",3,6,0,48},{"REFRIGERANTE",5,10,0,48}}};
    static constexpr int expansionCost=600,helperCost=350;
    bool expanded=false,tvOn=false;
    HelperState helper,secondHelper,thirdHelper;
    Checkout second,third;
    std::array<Checkout,2> annex;
    std::array<HelperState,2> annexHelpers;
    int annexCheckouts=0,staffSpeedLevel=0;
    bool wholesale=false,heldPacked=false;
    static constexpr int crateUnits=12;
    bool secondCheckout=false,thirdCheckout=false,largeStore=false;
    int storageLevel=0,heldCount=1;
    static constexpr int largeExpansionCost=1200,thirdCheckoutCost=750;
    int bagCapacity=3,orderSize=12,pendingUnits=12;
    int deliveryLevel=0,restockThreshold=25,restockReserve=0,restockCursor=0;
    bool autoRestockEnabled=false;
    float restockTimer=0;
    unsigned deliveriesReceived=0; // Transient notification counter, never an economic balance.
    std::string restockStatus="REPOSICAO AUTOMATICA DESLIGADA.";
    static constexpr int checkoutCost=500;
    int cash=250,sold=0,level=1,day=1,reputation=100;
    std::string companyName="GOIANAO DISTRIBUIDORA";
    int prestige=0,perk=0;
    int wanted=0,quantity=2,pending=-1,held=-1,delivered=0;
    int consumed=0,consuming=-1,customerStyle=0;
    std::array<float,4> fridgeTime{}; // Transient door animation, no inventory state.
    float delivery=0,patience=65,arrival=2,time=0;
    float intoxication=0,consumeTime=0,smokeTime=0;
    bool customer=false;
    bool reinforced=false; // Legacy save field; secondCheckout controls the new upgrade.
    std::string message="CLIQUE PARA PEGAR +1. COMPLETE O PEDIDO SOBRE O CLIENTE. TAB MOSTRA OS CONTROLES.";
    float messageTime=6;
    std::string lastMessage=message;
    std::mt19937 random{42};
    std::array<MixedOrder,5> mixedOrders{};
    std::array<CustomerMotion,5> customerMotion{};
    bool customerActive(int lane) const;
    int requested(int lane,int product) const;
    int fulfilled(int lane,int product) const;
    int remaining(int lane,int product) const;
    int nextProduct(int lane) const;
    int orderTotal(int lane) const;
    int orderDelivered(int lane) const;
    void leaveCustomer(int lane,bool purchased=false);
    bool returnHeld(bool frontStorage=true);
    bool order(int i,int units=0);
    int deliverySeconds() const {return deliveryLevel==0?12:deliveryLevel==1?6:deliveryLevel==2?3:0;}
    int deliveryUpgradeCost() const {return deliveryLevel==0?300:deliveryLevel==1?600:1200;}
    bool upgradeDelivery();
    bool toggleAutoRestock();
    void cycleRestockThreshold();
    void cycleRestockReserve();
    void receiveDelivery();
    void tickRestock(float dt);
    int availableProducts() const {return largeStore?productCount:4;}
    int capacityFor(int i) const;
    int expansionPrice() const {return expanded?largeExpansionCost:expansionCost;}
    int checkoutPrice() const {return checkoutCount()>=3?1000+annexCheckouts*250:secondCheckout?thirdCheckoutCost:checkoutCost;}
    int checkoutCount() const {return thirdCheckout?3+annexCheckouts:secondCheckout?2:1;}
    Checkout& extraCheckout(int lane) {return lane==2?third:annex[lane-3];}
    const Checkout& extraCheckout(int lane) const {return lane==2?third:annex[lane-3];}
    HelperState& staff(int lane) {return lane==0?helper:lane==1?secondHelper:lane==2?thirdHelper:annexHelpers[lane-3];}
    const HelperState& staff(int lane) const {return lane==0?helper:lane==1?secondHelper:lane==2?thirdHelper:annexHelpers[lane-3];}
    int staffCount() const;
    float staffSpeed() const {return 1.8f+staffSpeedLevel*.9f;}
    bool upgradeStaffSpeed();
    bool allowedLot(int units) const;
    int lotDiscount(int units=0) const;
    bool upgradeStorage();
    bool deliverPlayer(int lane=0);
    int orderCost(int i,int units=0) const;
    int marginBonus() const;
    int salePrice(int i) const;
    bool upgradeCheckout();
    bool upgradeBag();
    bool cycleOrderSize();
    void tickCustomer(float dt,int lane);
    bool pickup(int i,bool frontStorage=true,bool packed=false);
    bool sell();
    bool deliver(int& carried,int lane=0);
    bool expand();
    bool hire();
    void refreshCapacity();
    bool consume();
    void touchStorage(int product);
    bool upgradeLevel();
    bool resetWithPerk();
    int prestigeMarginBonus() const {return prestige*5;}
    void tick(float dt);
    int occupied(int i) const;
    std::string stockLabel(int i) const;
};
