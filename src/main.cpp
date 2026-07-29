#include "../include/engine/book.hpp"
#include "../include/engine/sequencer.hpp"
#include <iostream>

using namespace engine;

int main()
{
    Sequencer sequencer;
    Book book(sequencer);

    OrderAdded added1{NewOrderRequest{1, Side::Sell, 100, 5}};
    Event e1 = sequencer.sequence(added1);
    book.on_event(e1);

    OrderAdded added2{NewOrderRequest{2, Side::Buy, 100, 3}};
    Event e2 = sequencer.sequence(added2);
    auto trades = book.on_event(e2);

    std::cout << "Generated " << trades.size() << " trade(s)\n";
    for (const auto& t : trades) {
        const auto& trade = std::get<Trade>(t.payload);
        std::cout << "Trade: qty=" << trade.quantity
                   << " price=" << trade.price << "\n";
    }

    const auto& order1 = book.orders().at(1);
    std::cout << "Order 1 remaining: " << order1.remaining_quantity << "\n";

    return 0;
}