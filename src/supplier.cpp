#include "game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr int DEFAULT_PENDING_UNITS = 12;

constexpr float AUTO_RESTOCK_INTERVAL = 1.0f;

// Largest lots first.
//
// Auto-restock attempts the largest currently unlocked lot that:
//   - does not exceed orderSize;
//   - fits in storage;
//   - respects the cash reserve.
constexpr std::array<int, 6> RESTOCK_LOTS{
    288,
    192,
    96,
    48,
    24,
    12
};

// =============================================================================
// Helpers
// =============================================================================

bool validPositiveAmount(int units)
{
    return units > 0;
}

bool stockAtOrBelowThreshold(
    int stock,
    int capacity,
    int thresholdPercent)
{
    if (capacity <= 0)
        return false;

    // int64_t prevents overflow if capacities become much larger later.
    const std::int64_t stockPercentSide =
        static_cast<std::int64_t>(stock) *
        100;

    const std::int64_t thresholdSide =
        static_cast<std::int64_t>(capacity) *
        thresholdPercent;

    return stockPercentSide <= thresholdSide;
}

// Returns true if product A has a lower stock/capacity ratio than product B.
//
// Instead of:
//
//     stockA / capacityA < stockB / capacityB
//
// use cross multiplication to avoid floating point:
//
//     stockA * capacityB < stockB * capacityA
//
bool lowerRelativeStock(
    int stockA,
    int capacityA,
    int stockB,
    int capacityB)
{
    if (capacityA <= 0)
        return false;

    if (capacityB <= 0)
        return true;

    const std::int64_t left =
        static_cast<std::int64_t>(stockA) *
        capacityB;

    const std::int64_t right =
        static_cast<std::int64_t>(stockB) *
        capacityA;

    return left < right;
}

} // namespace

// =============================================================================
// PLACE ORDER
// =============================================================================

bool Game::order(
    int product,
    int units)
{
    // Zero means "use the currently selected lot size".
    if (units == 0)
        units = orderSize;

    // -------------------------------------------------------------------------
    // Validate product
    // -------------------------------------------------------------------------

    if (
        product < 0 ||
        product >= availableProducts()
    ) {
        message =
            "PRODUTO AINDA NAO LIBERADO.";

        return false;
    }

    // -------------------------------------------------------------------------
    // Validate lot
    // -------------------------------------------------------------------------

    if (
        !validPositiveAmount(units) ||
        !allowedLot(units)
    ) {
        message =
            "LOTE AINDA NAO LIBERADO.";

        return false;
    }

    // -------------------------------------------------------------------------
    // Only one supplier delivery can be active at once.
    // -------------------------------------------------------------------------

    if (pending != -1) {
        message =
            "AGUARDE A ENTREGA ATUAL.";

        return false;
    }

    // -------------------------------------------------------------------------
    // Calculate cost exactly once.
    // -------------------------------------------------------------------------

    const int price =
        orderCost(
            product,
            units
        );

    if (price < 0) {
        message =
            "VALOR DE ENCOMENDA INVALIDO.";

        return false;
    }

    if (cash < price) {
        message =
            "SALDO INSUFICIENTE.";

        return false;
    }

    // -------------------------------------------------------------------------
    // Capacity
    //
    // occupied() includes stock plus items temporarily outside storage
    // (player, employees, checkout, pending deliveries, etc.).
    // -------------------------------------------------------------------------

    const std::int64_t futureOccupied =
        static_cast<std::int64_t>(
            occupied(product)
        ) +
        units;

    const std::int64_t capacity =
        products[product].capacity;

    if (futureOccupied > capacity) {
        message =
            "SEM ESPACO PARA ESTE LOTE. "
            "VENDA OU REDUZA A QUANTIDADE.";

        return false;
    }

    // -------------------------------------------------------------------------
    // Commit purchase
    // -------------------------------------------------------------------------

    cash -= price;

    pending =
        product;

    pendingUnits =
        units;

    const int seconds =
        std::max(
            0,
            deliverySeconds()
        );

    delivery =
        static_cast<float>(
            seconds
        );

    // -------------------------------------------------------------------------
    // Instant delivery
    // -------------------------------------------------------------------------

    if (seconds == 0) {
        receiveDelivery();
    }
    else {
        message =
            "ENCOMENDA PAGA. ENTREGA EM " +
            std::to_string(seconds) +
            " SEGUNDOS.";
    }

    return true;
}

// =============================================================================
// RECEIVE DELIVERY
// =============================================================================

void Game::receiveDelivery()
{
    if (pending < 0)
        return;

    // Defensive validation.
    //
    // A corrupted/old save should not be able to index products outside
    // the product array.
    if (
        pending >= productCount ||
        pendingUnits <= 0
    ) {
        pending = -1;
        pendingUnits =
            DEFAULT_PENDING_UNITS;

        delivery = 0.0f;

        message =
            "ENTREGA INVALIDA CANCELADA.";

        return;
    }

    const int product =
        pending;

    const int units =
        pendingUnits;

    // -------------------------------------------------------------------------
    // Order capacity was checked when purchased.
    //
    // Clamp defensively anyway so corrupted save data cannot push stock beyond
    // an integer-safe value or create a negative stock.
    // -------------------------------------------------------------------------

    const std::int64_t newStock =
        static_cast<std::int64_t>(
            products[product].stock
        ) +
        units;

    products[product].stock =
        static_cast<int>(
            std::min<std::int64_t>(
                newStock,
                2000000000LL
            )
        );

    // -------------------------------------------------------------------------
    // Clear delivery before publishing the message.
    // -------------------------------------------------------------------------

    pending = -1;

    pendingUnits =
        DEFAULT_PENDING_UNITS;

    delivery = 0.0f;

    ++deliveriesReceived;

    message =
        "ENTREGA RECEBIDA: " +
        std::to_string(units) +
        " UN. DE " +
        products[product].name +
        ".";
}

// =============================================================================
// DELIVERY UPGRADE
// =============================================================================

bool Game::upgradeDelivery()
{
    if (deliveryLevel >= 3) {
        message =
            "ENTREGA INSTANTANEA JA LIBERADA.";

        return false;
    }

    const int cost =
        deliveryUpgradeCost();

    if (cost < 0) {
        message =
            "VALOR DE MELHORIA INVALIDO.";

        return false;
    }

    if (cash < cost) {
        message =
            "SALDO INSUFICIENTE PARA "
            "MELHORAR A ENTREGA.";

        return false;
    }

    cash -= cost;

    ++deliveryLevel;

    const int newSeconds =
        std::max(
            0,
            deliverySeconds()
        );

    message =
        "PRAZO DE ENTREGA: " +
        std::to_string(newSeconds) +
        " SEGUNDOS.";

    // -------------------------------------------------------------------------
    // Existing delivery also benefits from the upgrade.
    // -------------------------------------------------------------------------

    if (pending >= 0) {

        delivery =
            std::max(
                0.0f,
                delivery
            );

        delivery =
            std::min(
                delivery,
                static_cast<float>(
                    newSeconds
                )
            );

        if (
            newSeconds == 0 ||
            delivery <= 0.0f
        ) {
            receiveDelivery();
        }
    }

    return true;
}

// =============================================================================
// AUTO RESTOCK TOGGLE
// =============================================================================

bool Game::toggleAutoRestock()
{
    autoRestockEnabled =
        !autoRestockEnabled;

    // Force an immediate inventory check next time tickRestock() runs.
    restockTimer = 0.0f;

    // Keep cursor valid in case a save came from an older version.
    const int available =
        availableProducts();

    if (available > 0) {
        restockCursor =
            (
                restockCursor %
                available +
                available
            ) %
            available;
    }
    else {
        restockCursor = 0;
    }

    if (autoRestockEnabled) {
        restockStatus =
            "VERIFICANDO ESTOQUE. "
            "COMPRAS USAM O DINHEIRO DO CAIXA.";
    }
    else {
        restockStatus =
            "REPOSICAO AUTOMATICA DESLIGADA.";
    }

    message =
        restockStatus;

    return true;
}

// =============================================================================
// AUTO RESTOCK THRESHOLD
// =============================================================================

void Game::cycleRestockThreshold()
{
    switch (restockThreshold) {

        case 10:
            restockThreshold = 25;
            break;

        case 25:
            restockThreshold = 50;
            break;

        default:
            restockThreshold = 10;
            break;
    }

    restockTimer = 0.0f;

    message =
        "REPOR QUANDO O ESTOQUE CHEGAR A " +
        std::to_string(restockThreshold) +
        "% DA CAPACIDADE.";
}

// =============================================================================
// AUTO RESTOCK CASH RESERVE
// =============================================================================

void Game::cycleRestockReserve()
{
    switch (restockReserve) {

        case 0:
            restockReserve = 250;
            break;

        case 250:
            restockReserve = 500;
            break;

        case 500:
            restockReserve = 1000;
            break;

        case 1000:
            restockReserve = 2500;
            break;

        default:
            restockReserve = 0;
            break;
    }

    restockTimer = 0.0f;

    message =
        "COMPRAS AUTOMATICAS PRESERVAM R$ " +
        std::to_string(restockReserve) +
        " NO CAIXA.";
}

// =============================================================================
// AUTO RESTOCK
// =============================================================================

void Game::tickRestock(float dt)
{
    if (!autoRestockEnabled)
        return;

    // -------------------------------------------------------------------------
    // Defensive dt handling
    // -------------------------------------------------------------------------

    if (!std::isfinite(dt))
        dt = 0.0f;

    dt =
        std::max(
            0.0f,
            dt
        );

    restockTimer =
        std::max(
            0.0f,
            restockTimer - dt
        );

    if (restockTimer > 0.0f)
        return;

    // One automatic purchase at most per check.
    //
    // This is especially important once instant delivery is unlocked:
    // without a cooldown the system could buy many lots in a single frame.
    restockTimer =
        AUTO_RESTOCK_INTERVAL;

    // -------------------------------------------------------------------------
    // Existing delivery blocks another supplier order.
    // -------------------------------------------------------------------------

    if (pending >= 0) {
        restockStatus =
            "AGUARDANDO A ENTREGA ATUAL. "
            "NAO COMPRA EM DUPLICIDADE.";

        return;
    }

    const int available =
        availableProducts();

    if (available <= 0) {
        restockCursor = 0;

        restockStatus =
            "NENHUM PRODUTO DISPONIVEL "
            "PARA REPOSICAO.";

        return;
    }

    // Keep persisted cursor valid after unlock/progression changes.
    restockCursor =
        (
            restockCursor %
            available +
            available
        ) %
        available;

    // -------------------------------------------------------------------------
    // Available money without touching the configured reserve.
    // -------------------------------------------------------------------------

    const std::int64_t spendableCash =
        static_cast<std::int64_t>(
            cash
        ) -
        std::max(
            0,
            restockReserve
        );

    // -------------------------------------------------------------------------
    // Find best candidate.
    // -------------------------------------------------------------------------

    int chosen =
        -1;

    int chosenUnits =
        0;

    int chosenCost =
        0;

    bool lowStock =
        false;

    bool blockedByCash =
        false;

    bool blockedBySpace =
        false;

    // Start searching at restockCursor.
    //
    // If two products have the exact same relative stock, the first one in
    // this rotated order wins. After purchase cursor advances, providing
    // natural round-robin fairness.
    for (
        int offset = 0;
        offset < available;
        ++offset
    ) {
        const int product =
            (
                restockCursor +
                offset
            ) %
            available;

        const int stock =
            std::max(
                0,
                products[product].stock
            );

        const int capacity =
            products[product].capacity;

        if (
            !stockAtOrBelowThreshold(
                stock,
                capacity,
                restockThreshold
            )
        ) {
            continue;
        }

        lowStock = true;

        // ---------------------------------------------------------------------
        // Find largest possible lot for this product.
        // ---------------------------------------------------------------------

        int candidateUnits =
            0;

        int candidateCost =
            0;

        bool productBlockedBySpace =
            false;

        bool productBlockedByCash =
            false;

        for (int units : RESTOCK_LOTS) {

            // Auto-restock never exceeds the lot size selected by the player.
            if (units > orderSize)
                continue;

            if (!allowedLot(units))
                continue;

            const std::int64_t futureOccupied =
                static_cast<std::int64_t>(
                    occupied(product)
                ) +
                units;

            if (
                futureOccupied >
                static_cast<std::int64_t>(
                    capacity
                )
            ) {
                productBlockedBySpace = true;
                continue;
            }

            const int cost =
                orderCost(
                    product,
                    units
                );

            if (
                cost < 0 ||
                static_cast<std::int64_t>(
                    cost
                ) >
                spendableCash
            ) {
                productBlockedByCash = true;
                continue;
            }

            candidateUnits =
                units;

            candidateCost =
                cost;

            // RESTOCK_LOTS is descending, so first valid lot is largest.
            break;
        }

        if (candidateUnits <= 0) {
            blockedBySpace =
                blockedBySpace ||
                productBlockedBySpace;

            blockedByCash =
                blockedByCash ||
                productBlockedByCash;

            continue;
        }

        // ---------------------------------------------------------------------
        // Lowest relative stock wins.
        //
        // Do NOT replace when ratios are exactly equal. Because iteration starts
        // at restockCursor, keeping the first equal candidate provides the
        // intended rotating tie-break.
        // ---------------------------------------------------------------------

        if (
            chosen < 0 ||
            lowerRelativeStock(
                stock,
                capacity,
                std::max(
                    0,
                    products[chosen].stock
                ),
                products[chosen].capacity
            )
        ) {
            chosen =
                product;

            chosenUnits =
                candidateUnits;

            chosenCost =
                candidateCost;
        }
    }

    // =========================================================================
    // Nothing can be ordered
    // =========================================================================

    if (chosen < 0) {

        if (!lowStock) {
            restockStatus =
                "ESTOQUES ACIMA DO LIMIAR. "
                "NENHUMA COMPRA NECESSARIA.";

            return;
        }

        if (
            blockedByCash &&
            blockedBySpace
        ) {
            restockStatus =
                "REPOSICAO AGUARDANDO SALDO OU ESPACO. "
                "RESERVA: R$ " +
                std::to_string(
                    restockReserve
                ) +
                ".";
        }
        else if (blockedByCash) {
            restockStatus =
                "REPOSICAO AGUARDANDO SALDO. "
                "RESERVA: R$ " +
                std::to_string(
                    restockReserve
                ) +
                ".";
        }
        else if (blockedBySpace) {
            restockStatus =
                "REPOSICAO AGUARDANDO ESPACO "
                "NO ESTOQUE.";
        }
        else {
            restockStatus =
                "AGUARDANDO UM LOTE COMPATIVEL "
                "COM A CONFIGURACAO ATUAL.";
        }

        return;
    }

    // =========================================================================
    // Place automatic order
    // =========================================================================

    if (!order(
            chosen,
            chosenUnits)) {

        // Normally impossible because we just validated everything.
        // If another rule rejects it, preserve the useful Game::order message.
        restockStatus =
            "AUTO: " +
            message;

        return;
    }

    // Rotate starting point for the next check.
    restockCursor =
        (
            chosen + 1
        ) %
        available;

    restockStatus =
        "AUTO: " +
        std::to_string(
            chosenUnits
        ) +
        " UN. DE " +
        products[chosen].name +
        " POR R$ " +
        std::to_string(
            chosenCost
        ) +
        ".";

    message =
        restockStatus;
}