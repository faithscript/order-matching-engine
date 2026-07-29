#pragma once

#include "event.hpp"
#include <optional>

namespace engine
{

// terminal states (filled/cancelled/rejected) shouldn't transition further
enum class OrderStatus
{
    New,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected,
};

// current live state of an order, built up by folding events
struct Order
{
    OrderId     order_id;
    Side        side;
    Price       price;
    Quantity    original_quantity;
    Quantity    remaining_quantity;
    OrderStatus status;
};

// optional because added creates the order and rejected never has one
struct ApplyVisitor
{
    std::optional<Order>& order;

    void operator()(const OrderAdded& e);
    void operator()(const OrderCancelled& e);
    void operator()(const OrderModified& e);
    void operator()(const Trade& e);
    void operator()(const OrderRejected& e);
};

// folds one event into an order's state, implemented in book.cpp
void apply(std::optional<Order>& order, const Event& event);

}  // namespace engine