#include <iostream>
#include <cassert>
#include "../include/engine/fix.hpp"

int main() {
    // Simple NewOrderSingle FIX message (SOH = \x01)
    std::string msg = "8=FIX.4.4\x019=20\x0135=D\x0111=123\x0154=1\x0144=100\x0138=10\x0110=000\x01";
    engine::FixParser parser;
    auto result = parser.parse(msg);
    assert(std::holds_alternative<engine::NewOrderSingle>(result));
    const auto& nos = std::get<engine::NewOrderSingle>(result);
    assert(nos.order_id == 123);
    assert(nos.side == engine::Side::Buy);
    assert(nos.price == 100);
    assert(nos.quantity == 10);
    // Encode a dummy ExecutionReport
    engine::ExecutionReport report{nos.order_id, nos.side, nos.price, nos.quantity, 5, 5};
    engine::FixEncoder encoder;
    std::string encoded = encoder.encode(report);
    // Basic sanity: should contain MsgType=8 and OrderID
    assert(encoded.find("35=8") != std::string::npos);
    assert(encoded.find("11=123") != std::string::npos);
    std::cout << "All FIX tests passed\n";
    return 0;
}
