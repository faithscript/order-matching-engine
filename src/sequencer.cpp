#include "../include/engine/sequencer.hpp"

namespace engine
{

Event Sequencer::sequence(std::variant<OrderAdded, OrderCancelled, OrderModified,
                                        Trade, OrderRejected> payload)
{
    // the one place a seq number ever gets attached to a payload
    Event event{next_seq_, std::move(payload)};

    // increment after use, so the first event gets seq 0
    ++next_seq_;

    // stamp and log together so nothing can sequence without logging
    log_.push_back(event);

    return event;
}

}  // namespace engine