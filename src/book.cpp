#include "../include/engine/book.hpp"
#include "../include/engine/fix.hpp"
#include <algorithm>
#include <cassert>

namespace engine
{

// applying events to orders lives here since that's book's job

void ApplyVisitor::operator()(const OrderAdded& e)
{
    const auto& req = e.request;
    order = Order{
        .order_id           = req.order_id,
        .side               = req.side,
        .price              = req.price,
        .original_quantity  = req.quantity,
        .remaining_quantity = req.quantity,
        .status             = OrderStatus::New,
    };
}

void ApplyVisitor::operator()(const OrderCancelled& e)
{
    if (!order) return;
    assert(order->order_id == e.order_id && "apply() called with mismatched order/event");
    order->status = OrderStatus::Cancelled;
}

void ApplyVisitor::operator()(const OrderModified& e)
{
    if (!order) return;
    assert(order->order_id == e.order_id && "apply() called with mismatched order/event");
    order->remaining_quantity = e.quantity;
}

void ApplyVisitor::operator()(const Trade& e)
{
    if (!order) return;
    assert((order->order_id == e.aggressor_id || order->order_id == e.resting_id) &&
           "apply() called with order not party to this trade");
    order->remaining_quantity -= e.quantity;
    order->status = (order->remaining_quantity == 0)
                        ? OrderStatus::Filled
                        : OrderStatus::PartiallyFilled;
}

void ApplyVisitor::operator()(const OrderRejected& e)
{
    // no-op, nothing to create
}
void apply(std::optional<Order>& order, const Event& event)
{
    std::visit(ApplyVisitor{order}, event.payload);
}

// dispatches one sequenced event and collects any trades it produces

std::vector<Event> Book::on_event(const Event& event)
{
    std::vector<Event> new_events;
    EventVisitor visitor{*this, event.seq, new_events};
    std::visit(visitor, event.payload);
    return new_events;
}

std::vector<Event> Book::apply_fix(const std::variant<NewOrderSingle, OrderCancelRequest>& msg) {
    std::vector<Event> result;
    std::visit([&](auto&& m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T, NewOrderSingle>) {
            OrderAdded added{NewOrderRequest{m.order_id, m.side, m.price, m.quantity}};
            Event ev = sequencer_.sequence(added);
            result.push_back(ev);
            auto more = on_event(ev);
            result.insert(result.end(), more.begin(), more.end());
        } else if constexpr (std::is_same_v<T, OrderCancelRequest>) {
            OrderCancelled cancelled{m.order_id};
            Event ev = sequencer_.sequence(cancelled);
            result.push_back(ev);
            auto more = on_event(ev);
            result.insert(result.end(), more.begin(), more.end());
        }
    }, msg);
    return result;
}

// validation plus the matching loop

void Book::EventVisitor::operator()(const OrderAdded& e)
{
    const auto& req = e.request;

    // reject if halted or duplicate id, rejections get sequenced too
    if (book.status_ != BookStatus::Open) {
        OrderRejected rejected{req, RejectReason::BookHalted};
        new_events.push_back(book.sequencer_.sequence(rejected));
        return;
    }
    if (book.orders_.count(req.order_id)) {
        OrderRejected rejected{req, RejectReason::DuplicateOrderId};
        new_events.push_back(book.sequencer_.sequence(rejected));
        return;
    }

    Order incoming{req.order_id, req.side, req.price,
                    req.quantity, req.quantity, OrderStatus::New};

    // buy crosses asks at or below its price, sell crosses bids at or above
    auto match_against = [&](auto& opposite_side, auto crosses) {
        while (incoming.remaining_quantity > 0 && !opposite_side.empty()) {
            auto level_it = opposite_side.begin();
            Price level_price = level_it->first;
            if (!crosses(level_price)) break;

            PriceLevel& level = level_it->second;
            OrderId resting_id = level.front();
            Order& resting = book.orders_.at(resting_id);

            Quantity fill_qty = std::min(incoming.remaining_quantity,
                                          resting.remaining_quantity);

            Trade trade{req.order_id, resting_id, level_price, fill_qty};
            Event trade_event = book.sequencer_.sequence(trade);
            new_events.push_back(trade_event);

            incoming.remaining_quantity -= fill_qty;
            incoming.status = (incoming.remaining_quantity == 0)
                                   ? OrderStatus::Filled
                                   : OrderStatus::PartiallyFilled;

            resting.remaining_quantity -= fill_qty;
            resting.status = (resting.remaining_quantity == 0)
                                  ? OrderStatus::Filled
                                  : OrderStatus::PartiallyFilled;

            if (resting.remaining_quantity == 0) {
                level.pop_front();
                if (level.empty()) opposite_side.erase(level_it);
            }
        }
    };

    if (req.side == Side::Buy) {
        match_against(book.asks_, [&](Price p) { return p <= req.price; });
    } else {
        match_against(book.bids_, [&](Price p) { return p >= req.price; });
    }

    book.orders_[req.order_id] = incoming;

    // leftover quantity rests on the book at its own price
    if (incoming.remaining_quantity > 0) {
        if (req.side == Side::Buy) {
            book.bids_[req.price].push_back(req.order_id);
        } else {
            book.asks_[req.price].push_back(req.order_id);
        }
    }
}

void Book::EventVisitor::operator()(const OrderCancelled& e)
{
    auto it = book.orders_.find(e.order_id);
    if (it == book.orders_.end()) return;

    Order& order = it->second;
    if (order.status == OrderStatus::Filled ||
        order.status == OrderStatus::Cancelled ||
        order.status == OrderStatus::Rejected) {
        return;
    }

    order.status = OrderStatus::Cancelled;

    if (order.side == Side::Buy) {
        auto level_it = book.bids_.find(order.price);
        if (level_it != book.bids_.end()) {
            auto& level = level_it->second;
            level.erase(std::remove(level.begin(), level.end(), order.order_id),
                        level.end());
            if (level.empty()) book.bids_.erase(level_it);
        }
    } else {
        auto level_it = book.asks_.find(order.price);
        if (level_it != book.asks_.end()) {
            auto& level = level_it->second;
            level.erase(std::remove(level.begin(), level.end(), order.order_id),
                        level.end());
            if (level.empty()) book.asks_.erase(level_it);
        }
    }
}

void Book::EventVisitor::operator()(const OrderModified& e)
{
    auto it = book.orders_.find(e.order_id);
    if (it == book.orders_.end()) return;

    it->second.remaining_quantity = e.quantity;
}

// trades only ever come from the matching loop above, never as top-level input

void Book::EventVisitor::operator()(const Trade& e)
{
    // intentionally empty for layer 0
}

// same deal, rejections only ever generated internally

void Book::EventVisitor::operator()(const OrderRejected& e)
{
    // intentionally empty for layer 0
}

}  // namespace engine