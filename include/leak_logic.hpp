#ifndef LEAK_LOGIC_HPP
#define LEAK_LOGIC_HPP

#include <iostream>

namespace lg {

struct LeakLogicState {

};

/**
 * @brief Leak detection logic.
 */
class LeakLogic {
public:
    /**
     * @brief Updates the logic with the current state of the sensors and probes.
     *
     * @param state A struct describing the sensors' and probes' state.
     */
    void updateState(LeakLogicState&& state) {
        currentState = state;
    }

    /**
     * @brief Ticks the logic, evaluating all set schedules and determining whether a leak occurred.
     *
     * @param currentTime The current timestamp. TODO: pick a unit for this - Unix timestamp might be good
     */
    void tick(time_t currentTime) {
        // TODO
    }

private:
    LeakLogicState currentState;
};


}
#endif //LEAK_LOGIC_HPP
