#include "game.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>

#include "shop_layout.hpp"

namespace {

constexpr int kMaxCheckouts = 5;
constexpr int kMaxStorageLevel = 3;
constexpr int kMaxStaffSpeedLevel = 3;
constexpr int kMaxBagCapacity = 5;
constexpr int kMaxLevel = 1000;

constexpr float kDayDuration = 180.0f;
constexpr float kMessageDuration = 4.0f;
constexpr float kCustomerRespawnDelay = 4.0f;
constexpr float kNormalPatience = 65.0f;
constexpr float kWholesalePatience = 180.0f;
constexpr float kDoorOpenTime = 1.6f;
constexpr float kCustomerDepartureDuration = 5.0f;

constexpr int kCashLimit = 2'000'000'000;

int clampCash(std::int64_t value)
{
    return static_cast<int>(std::clamp<std::int64_t>(value, 0, kCashLimit));
}

int randomIndex(int count)
{
    if (count <= 1)
        return 0;

    return std::rand() % count;
}

int randomRangeInclusive(int minimum, int maximum)
{
    if (maximum <= minimum)
        return minimum;

    return minimum + randomIndex(maximum - minimum + 1);
}

} // namespace

bool Game::returnHeld(bool frontStorage)
{
    if (consumeTime > 0.0f || held < 0)
        return false;

    if (held >= productCount) {
        message = "ITEM INVALIDO NA SACOLA.";
        return false;
    }

    if (frontStorage)
        touchStorage(held);

    products[held].stock += std::max(0, heldCount);

    held = -1;
    heldCount = 1;
    heldPacked = false;

    message = "PRODUTOS DEVOLVIDOS AO ESTOQUE.";
    return true;
}

bool Game::pickup(int i, bool frontStorage, bool packed)
{
    if (i < 0 || i >= availableProducts()) {
        message = "PRODUTO LIBERADO NA SEGUNDA AMPLIACAO.";
        return false;
    }

    if (consumeTime > 0.0f) {
        message = "AGUARDE TERMINAR DE CONSUMIR.";
        return false;
    }

    if (held >= 0 && (held != i || heldPacked != packed)) {
        message = "ENTREGUE A SACOLA OU USE Q NA ORIGEM PARA DEVOLVER.";
        return false;
    }

    const int unitsPerPickup = packed ? crateUnits : 1;

    if (unitsPerPickup <= 0) {
        message = "CONFIGURACAO DE CARGA INVALIDA.";
        return false;
    }

    if (packed && !largeStore) {
        message = "AMPLIE A LOJA PARA CARREGAR ENGRADADOS.";
        return false;
    }

    const int currentUnits = held < 0 ? 0 : heldCount;
    const int maxUnits = bagCapacity * unitsPerPickup;

    if (currentUnits + unitsPerPickup > maxUnits) {
        message = "SACOLA CHEIA. ENTREGUE OU USE Q NA ORIGEM.";
        return false;
    }

    if (products[i].stock < unitsPerPickup) {
        message = "SEM ESTOQUE SUFICIENTE. ENCOMENDE NO COMPUTADOR.";
        return false;
    }

    held = i;
    heldCount = currentUnits + unitsPerPickup;
    heldPacked = packed;
    products[i].stock -= unitsPerPickup;

    if (frontStorage)
        touchStorage(i);

    message = carriedLabel(heldCount, packed) +
              " NA SACOLA. CLIQUE NOVAMENTE PARA PEGAR MAIS.";

    return true;
}

bool Game::customerActive(int lane) const
{
    if (lane < 0 || lane >= checkoutCount())
        return false;

    if (lane == 0)
        return customer;

    if (lane == 1)
        return second.customer;

    return extraCheckout(lane).customer;
}

int Game::requested(int lane, int product) const
{
    if (lane < 0 || lane >= checkoutCount())
        return 0;

    if (product < 0 || product >= productCount)
        return 0;

    const int first =
        lane == 0 ? wanted :
        lane == 1 ? second.wanted :
                    extraCheckout(lane).wanted;

    if (product == first) {
        return lane == 0 ? quantity :
               lane == 1 ? second.quantity :
                           extraCheckout(lane).quantity;
    }

    return mixedOrders[lane].requested[product];
}

int Game::fulfilled(int lane, int product) const
{
    if (lane < 0 || lane >= checkoutCount())
        return 0;

    if (product < 0 || product >= productCount)
        return 0;

    const int first =
        lane == 0 ? wanted :
        lane == 1 ? second.wanted :
                    extraCheckout(lane).wanted;

    if (product == first) {
        return lane == 0 ? delivered :
               lane == 1 ? second.delivered :
                           extraCheckout(lane).delivered;
    }

    return mixedOrders[lane].delivered[product];
}

int Game::remaining(int lane, int product) const
{
    if (!customerActive(lane))
        return 0;

    return std::max(0, requested(lane, product) - fulfilled(lane, product));
}

int Game::nextProduct(int lane) const
{
    if (!customerActive(lane))
        return -1;

    for (int i = 0; i < availableProducts(); ++i) {
        if (remaining(lane, i) > 0 && products[i].stock > 0)
            return i;
    }

    return -1;
}

int Game::orderTotal(int lane) const
{
    if (lane < 0 || lane >= checkoutCount())
        return 0;

    int total = 0;

    for (int i = 0; i < productCount; ++i)
        total += requested(lane, i);

    return total;
}

int Game::orderDelivered(int lane) const
{
    if (lane < 0 || lane >= checkoutCount())
        return 0;

    int total = 0;

    for (int i = 0; i < productCount; ++i)
        total += fulfilled(lane, i);

    return total;
}

void Game::leaveCustomer(int lane, bool purchased)
{
    if (lane < 0 || lane >= checkoutCount())
        return;

    auto& motion = customerMotion[lane];

    motion.approach = 0.0f;
    motion.departure = 1.0f;
    motion.purchased = purchased;

    motion.departingStyle =
        lane == 0 ? customerStyle :
        lane == 1 ? second.customerStyle :
                    extraCheckout(lane).customerStyle;

    mixedOrders[lane] = {};
}

bool Game::sell()
{
    return deliverPlayer();
}

bool Game::deliverPlayer(int lane)
{
    if (lane < 0 || lane >= checkoutCount()) {
        message = "CAIXA INVALIDO.";
        return false;
    }

    if (consumeTime > 0.0f) {
        message = "AGUARDE TERMINAR DE CONSUMIR.";
        return false;
    }

    if (held < 0) {
        message = "PEGUE OS ITENS INDICADOS SOBRE O CLIENTE.";
        return false;
    }

    bool deliveredAny = false;

    while (held >= 0 && heldCount > 0) {
        if (deliveredAny && remaining(lane, held) <= 0)
            break;

        int unit = held;

        if (!deliver(unit, lane))
            break;

        deliveredAny = true;

        --heldCount;

        if (heldCount <= 0) {
            held = -1;
            heldCount = 1;
            heldPacked = false;
            break;
        }

        if (!customerActive(lane))
            break;
    }

    return deliveredAny;
}

bool Game::deliver(int& carried, int lane)
{
    if (lane < 0 || lane >= checkoutCount()) {
        message = "CAIXA INVALIDO.";
        return false;
    }

    if (&carried == &held)
        return deliverPlayer(lane);

    if (!customerActive(lane)) {
        message = "AGUARDE UM CLIENTE PARA ENTREGAR.";
        return false;
    }

    if (customerMotion[lane].approach > 0.0f) {
        message = "O CLIENTE ESTA CHEGANDO AO BALCAO.";
        return false;
    }

    if (carried < 0 || carried >= availableProducts()) {
        message = "PEGUE OS ITENS INDICADOS SOBRE O CLIENTE.";
        return false;
    }

    if (remaining(lane, carried) <= 0) {
        message = "ESTE ITEM NAO FALTA NO PEDIDO. Q NA ORIGEM DEVOLVE.";
        return false;
    }

    const int first =
        lane == 0 ? wanted :
        lane == 1 ? second.wanted :
                    extraCheckout(lane).wanted;

    auto& firstDelivered =
        lane == 0 ? delivered :
        lane == 1 ? second.delivered :
                    extraCheckout(lane).delivered;

    if (carried == first)
        ++firstDelivered;
    else
        ++mixedOrders[lane].delivered[carried];

    carried = -1;

    if (orderDelivered(lane) < orderTotal(lane)) {
        message = "ITEM ENTREGUE. COMPLETE OS OUTROS ITENS DO PEDIDO.";
        return true;
    }

    std::int64_t payment = 0;

    for (int i = 0; i < productCount; ++i) {
        payment +=
            static_cast<std::int64_t>(requested(lane, i)) *
            static_cast<std::int64_t>(salePrice(i));
    }

    const int soldThisOrder = orderTotal(lane);

    cash = clampCash(static_cast<std::int64_t>(cash) + payment);

    if (soldThisOrder > 0) {
        sold = static_cast<int>(std::min<std::int64_t>(
            std::numeric_limits<int>::max(),
            static_cast<std::int64_t>(sold) + soldThisOrder));
    }

    auto& active =
        lane == 0 ? customer :
        lane == 1 ? second.customer :
                    extraCheckout(lane).customer;

    auto& nextArrival =
        lane == 0 ? arrival :
        lane == 1 ? second.arrival :
                    extraCheckout(lane).arrival;

    active = false;
    firstDelivered = 0;
    nextArrival = kCustomerRespawnDelay;

    leaveCustomer(lane, true);

    reputation = std::min(100, reputation + 2);

    message = "PEDIDO COMPLETO! +R$ " + std::to_string(payment);
    return true;
}

void Game::tick(float dt)
{
    if (dt <= 0.0f)
        return;

    for (auto& door : fridgeTime)
        door = std::max(0.0f, door - dt);

    consumeTime = std::max(0.0f, consumeTime - dt);
    smokeTime = std::max(0.0f, smokeTime - dt);
    intoxication = std::max(0.0f, intoxication - dt * 0.5f);

    if (pending != -1) {
        delivery = std::max(0.0f, delivery - dt);

        if (delivery <= 0.0f)
            receiveDelivery();
    }

    time += dt;

    while (time >= kDayDuration) {
        time -= kDayDuration;
        ++day;
        message = "NOVO DIA! CONTINUE EXPANDINDO A LOJA.";
    }

    const int lanes = std::clamp(checkoutCount(), 0, kMaxCheckouts);

    for (int lane = 0; lane < lanes; ++lane) {
        tickCustomer(dt, lane);
        tickHelper(*this, dt, lane);
    }

    tickRestock(dt);

    if (message != lastMessage) {
        messageTime = kMessageDuration;
        lastMessage = message;
    }
    else {
        messageTime = std::max(0.0f, messageTime - dt);
    }
}

void Game::tickCustomer(float dt, int lane)
{
    if (dt <= 0.0f || lane < 0 || lane >= checkoutCount())
        return;

    auto& currentCustomer =
        lane >= 2 ? extraCheckout(lane).customer :
        lane == 1 ? second.customer :
                    this->customer;

    auto& currentWanted =
        lane >= 2 ? extraCheckout(lane).wanted :
        lane == 1 ? second.wanted :
                    this->wanted;

    auto& currentQuantity =
        lane >= 2 ? extraCheckout(lane).quantity :
        lane == 1 ? second.quantity :
                    this->quantity;

    auto& currentDelivered =
        lane >= 2 ? extraCheckout(lane).delivered :
        lane == 1 ? second.delivered :
                    this->delivered;

    auto& currentArrival =
        lane >= 2 ? extraCheckout(lane).arrival :
        lane == 1 ? second.arrival :
                    this->arrival;

    auto& currentPatience =
        lane >= 2 ? extraCheckout(lane).patience :
        lane == 1 ? second.patience :
                    this->patience;

    auto& currentStyle =
        lane >= 2 ? extraCheckout(lane).customerStyle :
        lane == 1 ? second.customerStyle :
                    this->customerStyle;

    auto& currentWholesale =
        lane >= 2 ? extraCheckout(lane).wholesale :
        lane == 1 ? second.wholesale :
                    this->wholesale;

    auto& motion = customerMotion[lane];

    motion.departure = std::max(
        0.0f,
        motion.departure - dt / kCustomerDepartureDuration);

    // The customer only starts losing patience after reaching the counter.
    if (currentCustomer && motion.approach > 0.0f) {
        const float walkingTime =
            std::min(dt, motion.approach * kCustomerDepartureDuration);

        motion.approach = std::max(
            0.0f,
            motion.approach - walkingTime / kCustomerDepartureDuration);

        dt -= walkingTime;

        if (dt <= 0.0f)
            return;
    }

    if (currentCustomer) {
        currentPatience -= dt;

        if (currentPatience > 0.0f)
            return;

        currentPatience = 0.0f;

        // Products already placed on the checkout are returned to stock.
        for (int i = 0; i < productCount; ++i)
            products[i].stock += fulfilled(lane, i);

        currentDelivered = 0;
        currentCustomer = false;
        currentArrival = kCustomerRespawnDelay;

        leaveCustomer(lane, false);

        reputation = std::max(0, reputation - 8);
        message = "CLIENTE DESISTIU. ITENS DO BALCAO VOLTARAM AO ESTOQUE.";
        return;
    }

    currentArrival -= dt;

    if (currentArrival > 0.0f)
        return;

    const int available = availableProducts();

    if (available <= 0) {
        currentArrival = kCustomerRespawnDelay;
        return;
    }

    currentArrival = 0.0f;
    currentCustomer = true;
    currentDelivered = 0;
    mixedOrders[lane] = {};
    motion.approach = 1.0f;

    currentWholesale =
        largeStore &&
        (lane >= 3 || (level >= 3 && randomIndex(3) == 0));

    if (currentWholesale) {
        constexpr std::array<int, 4> drinks{0, 2, 4, 5};

        std::array<int, 4> validDrinks{};
        int validDrinkCount = 0;

        for (int product : drinks) {
            if (product >= 0 && product < available)
                validDrinks[validDrinkCount++] = product;
        }

        // In case progression rules change later, gracefully fall back to a
        // regular customer instead of indexing a locked product.
        if (validDrinkCount == 0) {
            currentWholesale = false;
        }
        else {
            currentWanted = validDrinks[randomIndex(validDrinkCount)];

            const int progressionCrates =
                2 + (std::max(1, level) - 1) / 2 +
                (std::max(1, day) - 1) / 3;

            const int maxCrates = std::clamp(progressionCrates, 1, 5);

            currentQuantity =
                randomRangeInclusive(1, maxCrates) * crateUnits;

            currentPatience = kWholesalePatience;
        }
    }

    if (!currentWholesale) {
        currentWanted = randomIndex(available);

        const int maxQuantity =
            std::clamp(level + 1, 1, 5);

        currentQuantity = randomRangeInclusive(1, maxQuantity);
        currentPatience = kNormalPatience;

        // Mixed order: only possible when there is another product to choose.
        if (available > 1 && randomIndex(3) != 0) {
            int other = randomIndex(available - 1);

            if (other >= currentWanted)
                ++other;

            mixedOrders[lane].requested[other] =
                randomRangeInclusive(1, 2);

            // Higher levels can add a third distinct product.
            if (level >= 3 && available > 2 && randomIndex(3) == 0) {
                int third = randomIndex(available - 2);

                for (int candidate = 0; candidate < available; ++candidate) {
                    if (candidate == currentWanted || candidate == other)
                        continue;

                    if (third == 0) {
                        mixedOrders[lane].requested[candidate] = 1;
                        break;
                    }

                    --third;
                }
            }
        }
    }

    if (customerLookCount > 1) {
        const int offset = 1 + randomIndex(customerLookCount - 1);
        currentStyle = (currentStyle + offset) % customerLookCount;
    }
    else {
        currentStyle = 0;
    }
}

int Game::occupied(int i) const
{
    if (i < 0 || i >= productCount)
        return 0;

    std::int64_t total = products[i].stock;

    if (held == i)
        total += heldCount;

    if (pending == i)
        total += pendingUnits;

    for (int lane = 0; lane < kMaxCheckouts; ++lane) {
        if (lane < checkoutCount() && customerActive(lane))
            total += fulfilled(lane, i);

        const auto& worker = staff(lane);

        if (worker.held == i)
            total += worker.count;
    }

    return static_cast<int>(std::clamp<std::int64_t>(
        total,
        0,
        std::numeric_limits<int>::max()));
}

std::string Game::stockLabel(int i) const
{
    if (i < 0 || i >= productCount)
        return "0/0";

    return std::to_string(products[i].stock) +
           "/" +
           std::to_string(products[i].capacity);
}

bool Game::consume()
{
    if (held < 0 || consumeTime > 0.0f) {
        message = "PEGUE UMA BEBIDA OU CIGARRO PRIMEIRO.";
        return false;
    }

    if (held >= availableProducts()) {
        message = "ITEM INVALIDO NA SACOLA.";
        return false;
    }

    if (held == 3) {
        message = "GELO NAO PODE SER CONSUMIDO. LEVE AO CLIENTE.";
        return false;
    }

    consuming = held;

    --heldCount;

    if (heldCount <= 0) {
        held = -1;
        heldCount = 1;
        heldPacked = false;
    }

    consumeTime = 2.0f;
    ++consumed;

    if (consuming == 1) {
        smokeTime = 5.0f;
        message = "FUMANDO. UMA UNIDADE SAIU DO ESTOQUE.";
    }
    else {
        const float effect =
            consuming == 5 ? 0.0f :
            consuming == 2 ? 35.0f :
                              15.0f;

        intoxication = std::min(100.0f, intoxication + effect);
        message = "BEBENDO. O EFEITO PASSA COM O TEMPO.";
    }

    return true;
}

int Game::capacityFor(int i) const
{
    constexpr std::array<int, 6> base{
        48,
        36,
        24,
        36,
        48,
        48
    };

    if (i < 0 || i >= static_cast<int>(base.size()))
        return 0;

    const int storeMultiplier =
        largeStore ? 4 :
        expanded   ? 2 :
                     1;

    return base[i] * (storeMultiplier + storageLevel);
}

void Game::refreshCapacity()
{
    for (int i = 0; i < productCount; ++i)
        products[i].capacity = capacityFor(i);
}

bool Game::expand()
{
    if (largeStore) {
        message = "LOJA NO TAMANHO MAXIMO.";
        return false;
    }

    const int price = expansionPrice();

    if (cash < price) {
        message = "SALDO INSUFICIENTE PARA AMPLIAR.";
        return false;
    }

    cash -= price;

    if (expanded)
        largeStore = true;
    else
        expanded = true;

    refreshCapacity();

    message = largeStore
        ? "NOVA ALA ABERTA! ENCOMENDE LATAS E REFRIGERANTES."
        : "LOJA AMPLIADA! DEPOSITO, SOFA E TV NOS FUNDOS.";

    return true;
}

bool Game::upgradeStorage()
{
    if (storageLevel >= kMaxStorageLevel) {
        message = "PRATELEIRAS DE ESTOQUE NO MAXIMO.";
        return false;
    }

    const int price = 250 * (storageLevel + 1);

    if (cash < price) {
        message = "SALDO INSUFICIENTE PARA ESTOQUE.";
        return false;
    }

    cash -= price;
    ++storageLevel;

    refreshCapacity();

    message = "CAPACIDADE AUMENTADA! ENCOMENDE PARA REPOR OS PRODUTOS.";
    return true;
}

int Game::staffCount() const
{
    int count = 0;

    for (int lane = 0; lane < kMaxCheckouts; ++lane) {
        if (staff(lane).hired)
            ++count;
    }

    return count;
}

bool Game::hire()
{
    const int lanes = std::clamp(checkoutCount(), 0, kMaxCheckouts);

    int lane = -1;

    for (int i = 0; i < lanes; ++i) {
        if (!staff(i).hired) {
            lane = i;
            break;
        }
    }

    if (lane < 0) {
        message = "ABRA MAIS UM CAIXA PARA CONTRATAR.";
        return false;
    }

    if (cash < helperCost) {
        message = "SALDO INSUFICIENTE PARA CONTRATAR.";
        return false;
    }

    cash -= helperCost;

    auto& worker = staff(lane);

    worker.hired = true;
    worker.x = checkoutX(lane);
    worker.z = -0.9f;

    message = "ATENDENTE " + std::to_string(lane + 1) +
              " CONTRATADO NO SEU CAIXA.";

    return true;
}

bool Game::upgradeStaffSpeed()
{
    if (staffSpeedLevel >= kMaxStaffSpeedLevel) {
        message = "EQUIPE NA VELOCIDADE MAXIMA.";
        return false;
    }

    if (staffCount() == 0) {
        message = "CONTRATE UM ATENDENTE PRIMEIRO.";
        return false;
    }

    const int price = 400 * (staffSpeedLevel + 1);

    if (cash < price) {
        message = "SALDO INSUFICIENTE PARA TREINAMENTO.";
        return false;
    }

    cash -= price;
    ++staffSpeedLevel;

    message = "EQUIPE MAIS RAPIDA! VALE PARA TODOS OS ATENDENTES.";
    return true;
}

void Game::touchStorage(int product)
{
    int timer = -1;

    switch (product) {
        case 0: timer = 0; break;
        case 2: timer = 1; break;
        case 4: timer = 2; break;
        case 5: timer = 3; break;
        default: break;
    }

    if (timer >= 0)
        fridgeTime[timer] = kDoorOpenTime;
}

bool Game::upgradeLevel()
{
    if (level >= kMaxLevel) {
        message = "NIVEL MAXIMO ATINGIDO.";
        return false;
    }

    const std::int64_t price =
        static_cast<std::int64_t>(200) *
        std::max(1, level);

    if (price > cash) {
        message = "SALDO INSUFICIENTE PARA MELHORAR O NIVEL.";
        return false;
    }

    cash -= static_cast<int>(price);
    ++level;

    message = "NIVEL MELHORADO! CLIENTES COMPRAM MAIS.";
    return true;
}

bool Game::resetWithPerk()
{
    if (sold < 25) {
        message = "VENDA PELO MENOS 25 UNIDADES PARA APOSENTAR A LOJA.";
        return false;
    }

    const std::string preservedName = companyName;
    const int nextPrestige = prestige + 1;

    Game fresh;

    fresh.companyName = preservedName;
    fresh.prestige = nextPrestige;
    fresh.perk = nextPrestige;
    fresh.cash = clampCash(
        static_cast<std::int64_t>(fresh.cash) +
        static_cast<std::int64_t>(nextPrestige) * 100);

    fresh.message =
        "NOVO CICLO! PERK DE PRESTIGIO ATIVO: MARGEM +" +
        std::to_string(nextPrestige * 5) +
        "%.";

    *this = fresh;
    return true;
}

bool Game::allowedLot(int units) const
{
    switch (units) {
        case 12:
            return true;

        case 24:
        case 48:
            return expanded;

        case 96:
        case 192:
        case 288:
            return largeStore;

        default:
            return false;
    }
}

int Game::lotDiscount(int units) const
{
    if (units == 0)
        units = orderSize;

    switch (units) {
        case 24:  return 10;
        case 48:  return 20;
        case 96:  return 25;
        case 192: return 30;
        case 288: return 35;
        default:  return 0;
    }
}

int Game::orderCost(int i, int units) const
{
    if (i < 0 || i >= availableProducts())
        return 0;

    if (units == 0)
        units = orderSize;

    const std::int64_t subtotal =
        static_cast<std::int64_t>(products[i].cost) *
        static_cast<std::int64_t>(units) *
        static_cast<std::int64_t>(100 - lotDiscount(units));

    const std::int64_t rounded =
        (subtotal + 99) / 100;

    return static_cast<int>(std::clamp<std::int64_t>(
        rounded,
        0,
        std::numeric_limits<int>::max()));
}

int Game::marginBonus() const
{
    const std::int64_t bonus =
        static_cast<std::int64_t>(std::max(0, day - 1)) * 2 +
        static_cast<std::int64_t>(std::max(0, level - 1)) * 5 +
        prestigeMarginBonus();

    return static_cast<int>(std::clamp<std::int64_t>(
        bonus,
        0,
        std::numeric_limits<int>::max()));
}

int Game::salePrice(int i) const
{
    if (i < 0 || i >= productCount)
        return 0;

    const std::int64_t price = products[i].price;
    const std::int64_t margin =
        static_cast<std::int64_t>(products[i].price) -
        static_cast<std::int64_t>(products[i].cost);

    const std::int64_t bonus = marginBonus();

    const std::int64_t result =
        price + (margin * bonus + 50) / 100;

    return static_cast<int>(std::clamp<std::int64_t>(
        result,
        0,
        std::numeric_limits<int>::max()));
}

bool Game::cycleOrderSize()
{
    if (!expanded) {
        message = "AMPLIE A LOJA PARA COMPRAR LOTES DE 24 OU 48.";
        return false;
    }

    switch (orderSize) {
        case 12:
            orderSize = 24;
            break;

        case 24:
            orderSize = 48;
            break;

        case 48:
            orderSize = largeStore ? 96 : 12;
            break;

        case 96:
            orderSize = largeStore ? 192 : 12;
            break;

        case 192:
            orderSize = largeStore ? 288 : 12;
            break;

        default:
            orderSize = 12;
            break;
    }

    message =
        "LOTE: " + std::to_string(orderSize) +
        " UNIDADES. DESCONTO: " +
        std::to_string(lotDiscount()) +
        " POR CENTO.";

    return true;
}

bool Game::upgradeCheckout()
{
    if (checkoutCount() >= kMaxCheckouts) {
        message = "OS CINCO CAIXAS JA ESTAO ABERTOS.";
        return false;
    }

    if (!expanded) {
        message = "AMPLIE A LOJA ANTES DE MELHORAR A GRADE.";
        return false;
    }

    if (secondCheckout && !largeStore) {
        message = "COMPRE A SEGUNDA AMPLIACAO PARA O TERCEIRO CAIXA.";
        return false;
    }

    const int price = checkoutPrice();

    if (cash < price) {
        message = "SALDO INSUFICIENTE PARA GRADE E CAIXA.";
        return false;
    }

    cash -= price;

    if (thirdCheckout)
        ++annexCheckouts;
    else if (secondCheckout)
        thirdCheckout = true;
    else
        secondCheckout = true;

    message = "GRADE AMPLIADA! NOVO CAIXA ABERTO.";
    return true;
}

bool Game::upgradeBag()
{
    if (bagCapacity >= kMaxBagCapacity) {
        message = "SACOLAS JA CARREGAM 5 UNIDADES.";
        return false;
    }

    const int price = 100 * std::max(1, bagCapacity);

    if (cash < price) {
        message = "SALDO INSUFICIENTE PARA SACOLAS.";
        return false;
    }

    cash -= price;
    ++bagCapacity;

    message =
        "SACOLA DO JOGADOR E ATENDENTES: " +
        std::to_string(bagCapacity) +
        " UNIDADES POR VIAGEM.";

    return true;
}