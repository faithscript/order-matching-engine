#include "../include/engine/fix.hpp"


#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, std::string> parse_tags(const std::string& msg) {
    std::unordered_map<std::string, std::string> tags;
    std::string token;
    std::istringstream ss(msg);
    while (std::getline(ss, token, '\x01')) {
        if (token.empty()) continue;
        auto pos = token.find('=');
        if (pos == std::string::npos) continue;
        tags[token.substr(0, pos)] = token.substr(pos + 1);
    }
    return tags;
}

static Side parse_side(const std::string& val) {
    return val == "1" ? Side::Buy : Side::Sell;
}

static std::string encode_message(const std::string& body) {
    std::string header = "8=FIX.4.4\x01";
    std::string length = "9=" + std::to_string(body.size()) + "\x01";
    std::string pre = header + length;
    std::string without_checksum = pre + body;
    unsigned int sum = 0;
    for (unsigned char c : without_checksum) sum += c;
    unsigned int checksum = sum % 256;
    char buf[4];
    snprintf(buf, sizeof(buf), "%03u", checksum);
    return without_checksum + "10=" + buf + "\x01";
}

std::variant<NewOrderSingle, OrderCancelRequest, Reject> engine::FixParser::parse(const std::string& raw_message) {
    auto tags = parse_tags(raw_message);
    auto it = tags.find("35");
    if (it == tags.end()) return Reject{"Missing MsgType"};
    const std::string& type = it->second;
    if (type == "D") {
        if (tags.find("11") == tags.end() || tags.find("54") == tags.end() || tags.find("44") == tags.end() || tags.find("38") == tags.end())
            return Reject{"Missing required fields for NewOrderSingle"};
        NewOrderSingle nos;
        nos.order_id = std::stoull(tags["11"]);
        nos.side = parse_side(tags["54"]);
        nos.price = std::stoll(tags["44"]);
        nos.quantity = std::stoull(tags["38"]);
        return nos;
    } else if (type == "F") {
        if (tags.find("11") == tags.end()) return Reject{"Missing required fields for OrderCancelRequest"};
        OrderCancelRequest ocr;
        ocr.order_id = std::stoull(tags["11"]);
        return ocr;
    } else {
        return Reject{"Unsupported MsgType"};
    }
}

std::string engine::FixEncoder::encode(const ExecutionReport& report) {
    std::ostringstream body;
    body << "35=8\x01";
    body << "11=" << report.order_id << "\x01";
    body << "54=" << (report.side == Side::Buy ? "1" : "2") << "\x01";
    body << "44=" << report.price << "\x01";
    body << "38=" << report.quantity << "\x01";
    body << "151=" << report.leaves_qty << "\x01";
    body << "14=" << report.exec_qty << "\x01";
    return encode_message(body.str());
}

std::string engine::FixEncoder::encode(const Reject& rej) {
    std::ostringstream body;
    body << "35=3\x01";
    body << "58=" << rej.reason << "\x01";
    return encode_message(body.str());
}
