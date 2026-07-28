#pragma once

#include "event.hpp"
#include <optional>

namespace engine
{

enum class OrderStatus
{
    New,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected,
};

struct Order
{
    OrderId     order_id;
    Side        side;
    Price       price;
    Quantity    original_quantity;
    Quantity    remaining_quantity;
    OrderStatus status;
};

void apply(std::optional<Order>& order, const Event& event);

} 