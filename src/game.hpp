#pragma once
#include <array>
#include <random>
#include <string>
#include "helper.hpp"
#include "customers.hpp"

struct Product {const char* name; int cost,price,stock,capacity;};
struct Checkout {
    bool customer=false;
    int wanted=0,quantity=1,delivered=0,customerStyle=1;
    float patience=65,arrival=4;
};
struct Game {
    std::array<Product,4> products{{{"CERVEJA",4,8,18,48},{"CIGARRO",7,12,10,36},
        {"DESTILADO",18,30,6,24},{"GELO",3,7,12,36}}};
    static constexpr int expansionCost=600,helperCost=350;
    bool expanded=false,tvOn=false;
    HelperState helper,secondHelper;
    Checkout second;
    bool secondCheckout=false;
    int bagCapacity=1,orderSize=12,pendingUnits=12;
    static constexpr int checkoutCost=500;
    int cash=250,sold=0,level=1,day=1,reputation=100;
    int wanted=0,quantity=2,pending=-1,held=-1,delivered=0;
    int consumed=0,consuming=-1,customerStyle=0;
    std::array<float,2> fridgeTime{}; // Transient door animation, no inventory state.
    float delivery=0,patience=65,arrival=2,time=0;
    float intoxication=0,consumeTime=0,smokeTime=0;
    bool customer=false;
    bool reinforced=false; // Legacy save field; secondCheckout controls the new upgrade.
    std::string message="PEGUE OS PRODUTOS COM E E LEVE AO CAIXA A ESQUERDA.";
    std::mt19937 random{42};
    bool order(int i);
    int orderCost(int i) const;
    int marginBonus() const;
    int salePrice(int i) const;
    bool upgradeCheckout();
    bool upgradeBag();
    bool cycleOrderSize();
    void tickCustomer(float dt,int lane);
    bool pickup(int i,bool frontStorage=true);
    bool sell();
    bool deliver(int& carried,int lane=0);
    bool expand();
    bool hire();
    void refreshCapacity();
    bool consume();
    void touchStorage(int product);
    bool upgradeLevel();
    void tick(float dt);
    int occupied(int i) const;
    std::string stockLabel(int i) const;
};
