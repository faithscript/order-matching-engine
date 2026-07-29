#pragma once

#include "event.hpp"
#include <vector>

namespace engine
{

// the one place that hands out sequence numbers, so the whole system
// has a single strictly increasing order of events
class Sequencer
{
public:
    // stamps a payload with the next seq number, logs it, returns the event
    Event sequence(std::variant<OrderAdded, OrderCancelled, OrderModified,
                                 Trade, OrderRejected> payload);

    // full log so far, used for replay/rebuild
    const std::vector<Event>& log() const { return log_; }

private:
    SequenceNum next_seq_ = 0;

    std::vector<Event> log_;
};

}  // namespace engine