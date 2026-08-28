#pragma once

#include <string>
#include <variant>


#include "event.hpp"

namespace engine {


struct NewOrderSingle {
    OrderId   order_id;  
    Side      side;   
    Price     price;    
    Quantity  quantity;  
};


struct OrderCancelRequest {
    OrderId order_id; 
};


struct ExecutionReport {
    OrderId   order_id;  
    Side      side;   
    Price     price;    
    Quantity  quantity;  
    Quantity  leaves_qty; 
    Quantity  exec_qty;  

};


struct Reject {
    std::string reason;

};


class FixParser {
   public:

    std::variant<NewOrderSingle, OrderCancelRequest, ExecutionReport, Reject>
    parse(const std::string& raw_message);
};

} 
