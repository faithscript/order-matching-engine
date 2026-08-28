#include "../include/engine/book.hpp"
#include "../include/engine/sequencer.hpp"
#include "../include/engine/fix.hpp"
#include "../include/engine/socket.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace engine;

int main() {
    Sequencer sequencer;
    Book book(sequencer);
    FixParser parser;
    FixEncoder encoder;
    TcpServer server(9876);
    server.start([&](const std::string& raw_msg) {
        auto parsed = parser.parse(raw_msg);
        if (std::holds_alternative<Reject>(parsed)) {
            const Reject& rej = std::get<Reject>(parsed);
            std::cout << encoder.encode(rej) << std::endl;
            return;
        }
        auto events = book.apply_fix(parsed);
        for (const auto& ev : events) {
            if (auto* trade = std::get_if<Trade>(&ev.payload)) {
                const Order& ord = book.orders().at(trade->aggressor_id);
                ExecutionReport report{trade->aggressor_id, ord.side, trade->price,
                                         trade->quantity, ord.remaining_quantity, trade->quantity};
                std::cout << encoder.encode(report) << std::endl;
            }
        }
    });
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}