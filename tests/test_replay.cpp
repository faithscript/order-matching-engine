#include "../include/engine/book.hpp"
#include "../include/engine/sequencer.hpp"
#include <cassert>
#include <iostream>

using namespace engine;

int main()
{
    // live run, process events as they happen
    Sequencer sequencer;
    Book book(sequencer);

    OrderAdded added1{NewOrderRequest{1, Side::Sell, 100, 5}};
    Event e1 = sequencer.sequence(added1);
    book.on_event(e1);

    OrderAdded added2{NewOrderRequest{2, Side::Buy, 100, 3}};
    Event e2 = sequencer.sequence(added2);
    book.on_event(e2);

    auto live_orders = book.orders();

    // rebuild from scratch by replaying the same log
    Sequencer replay_sequencer;  // separate counter, doesn't need to match original
    Book rebuilt(replay_sequencer);

    for (const Event& e : sequencer.log()) {
        rebuilt.on_event(e);
    }

    // compare live vs rebuilt
    assert(live_orders.size() == rebuilt.orders().size());
    for (const auto& [id, order] : live_orders) {
        const auto& rebuilt_order = rebuilt.orders().at(id);
        assert(order.remaining_quantity == rebuilt_order.remaining_quantity);
        assert(order.status == rebuilt_order.status);
        assert(order.side == rebuilt_order.side);
        assert(order.price == rebuilt_order.price);
    }

    std::cout << "Replay matches live state. Layer 0 deliverable confirmed.\n";
    return 0;
}