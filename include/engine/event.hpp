#pragma once

#include <cstdint>
#include <variant>

namespace engine {

using OrderId     = std::uint64_t;
using SequenceNum = std::uint64_t;
using Quantity    = std::uint64_t;

using Price = std::int64_t;

enum class Side { Buy, Sell };

struct OrderAdded {
    OrderId  order_id;
    Side     side;
    Price    price;
    Quantity quantity;
};

struct OrderCancelled {
    OrderId order_id;
};

struct OrderModified {
    OrderId  order_id;
    Quantity quantity;

};

struct Trade {
    OrderId  aggressor_id;
    OrderId  resting_id;
    Price    price;
    Quantity quantity;
};

struct Event {
    SequenceNum seq;
    std::variant<OrderAdded, OrderCancelled, OrderModified, Trade> payload;
};

}