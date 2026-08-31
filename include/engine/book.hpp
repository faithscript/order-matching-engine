#pragma once

#include "event.hpp"
#include "fix.hpp"

#include "order.hpp"
#include "sequencer.hpp"

#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

namespace engine
{

// book's current state — open, halted, or closed
enum class BookStatus { Open, Halted, Closed };

// fifo of order ids at one price level
using PriceLevel = std::deque<OrderId>;

class Book
{
public:
    // borrows the sequencer, doesn't own it
    explicit Book(Sequencer& sequencer) : sequencer_(sequencer) {}
    std::vector<Event> apply_fix(const std::variant<NewOrderSingle, OrderCancelRequest, Reject>& msg);

    // feed in an event, get back any new events it produced
    std::vector<Event> on_event(const Event& event);

    // read-only access for replay/verification
    const std::unordered_map<OrderId, Order>& orders() const { return orders_; }
    BookStatus status() const { return status_; }
    void set_status(BookStatus new_status) { status_ = new_status; }

private:
    // dispatches on event type
    struct EventVisitor
    {
        Book& book;
        SequenceNum seq;
        std::vector<Event>& new_events;

        void operator()(const OrderAdded& e);
        void operator()(const OrderCancelled& e);
        void operator()(const OrderModified& e);
        void operator()(const Trade& e);
        void operator()(const OrderRejected& e);
    };

    // bids high to low
    std::map<Price, PriceLevel, std::greater<Price>> bids_;

    // asks low to high
    std::map<Price, PriceLevel> asks_;

    // all live orders by id
    std::unordered_map<OrderId, Order> orders_;

    BookStatus status_ = BookStatus::Open;

    // shared, not owned
    Sequencer& sequencer_;
};

}  // namespace engine