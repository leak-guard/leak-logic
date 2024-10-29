#ifndef LEAK_LOGIC_HPP
#define LEAK_LOGIC_HPP

#include "leakguard/staticvector.hpp"

#include <cstdint>
#include <memory>
#include <optional>

#define LEAK_LOGIC_MAX_CRITERIA 10

namespace lg {

    /**
     * @brief Leak prevention action type.
     */
    enum class ActionType {
        NO_ACTION,
        CLOSE_VALVE
    };

    /**
     * @brief Leak prevention action reason.
     */
    enum class ActionReason {
        NONE,
        EXCEEDED_FLOW_RATE,
        LEAK_DETECTED_BY_PROBE
    };

    /**
     * @brief State of sensors used for leak detection.
     */
    struct SensorState {
        /**
         * @brief Water flow rate from the flow meter, specified in liters per minute.
         */
        float flowRate;

        /**
         * @brief Array of probe states - true if the probe detected a leak, false otherwise.
         */
        StaticVector<bool, 256>& probeStates;
    };

    /**
     * @brief Action determined by the leak logic.
     */
    class LeakPreventionAction {
    public:
        explicit LeakPreventionAction(
            const ActionType actionType = ActionType::NO_ACTION,
            const ActionReason reason = ActionReason::NONE,
            const uint8_t probeId = -1)
                : actionType(actionType), reason(reason), probeId(probeId) {}

        /**
         * @brief The action type.
         */
        [[nodiscard]] ActionType getActionType() const { return actionType; }

        /**
         * @brief The action reason.
         */
        [[nodiscard]] ActionReason getActionReason() const { return reason; }

        /**
         * @brief The probe ID. Valid only if action reason is LEAK_DETECTED_BY_PROBE.
         */
        [[nodiscard]] uint8_t getProbeId() const { return probeId; }

    private:
        ActionType actionType;
        ActionReason reason;
        uint8_t probeId;
    };

    /**
     * @brief Abstract class for defining leak detection criteria.
     */
    class LeakDetectionCriterion {
    public:
        virtual void update(const SensorState& sensorState, time_t elapsedTime) = 0;

        [[nodiscard]] virtual std::optional<LeakPreventionAction> getAction() const = 0;

        virtual ~LeakDetectionCriterion() = default;
    };

    /**
     * @brief Detection of leaks based on a flow rate threshold and a duration.
     *
     * If the flow rate exceeds a specified value for a given duration, the CLOSE_VALVE action is taken.
     */
    class TimeBasedFlowRateCriterion final : public LeakDetectionCriterion {
    public:
        /**
        * @param rateThreshold Flow rate threshold, in liters per minute.
        * @param minDuration Minimum duration for exceeded flow rate, in seconds.
        */
        TimeBasedFlowRateCriterion(const float rateThreshold, const time_t minDuration)
            : rateThreshold(rateThreshold), minDuration(minDuration),
              accumulatedTime(0), active(false) {}

        void update(const SensorState& sensorState, const time_t elapsedTime) override {
            if (sensorState.flowRate >= rateThreshold) {
                accumulatedTime += elapsedTime;
                active = true;
            }
            else {
                accumulatedTime = 0;
                active = false;
            }
        }

        [[nodiscard]] std::optional<LeakPreventionAction> getAction() const override {
            if (active && accumulatedTime >= minDuration) {
                return LeakPreventionAction(
                    ActionType::CLOSE_VALVE,
                    ActionReason::EXCEEDED_FLOW_RATE
                );
            }
            return std::nullopt;
        }

    private:
        float rateThreshold;
        time_t minDuration;
        time_t accumulatedTime;

        bool active;
    };

    /**
     * @brief Detection of leaks based on flood signals from a specific probe.
     *
     * If the specified probe has emitted a signal, the CLOSE_VALVE action is taken.
     */
    // Maybe we shouldn't have this? This is done for clarity, but it'd require separate criterion objects for each probe
    class ProbeLeakDetectionCriterion final : public LeakDetectionCriterion {
    public:
        explicit ProbeLeakDetectionCriterion(const uint8_t probeId) : probeId(probeId) {}

        void update(const SensorState& sensorState, const time_t elapsedTime) override {
            leakDetected = false;
            for (const auto state : sensorState.probeStates) {
                if (state) {
                    leakDetected = true;
                }
            }
        }

        [[nodiscard]] std::optional<LeakPreventionAction> getAction() const override {
            if (leakDetected) {
                return LeakPreventionAction(
                    ActionType::CLOSE_VALVE,
                    ActionReason::LEAK_DETECTED_BY_PROBE,
                    probeId
                );
            }
            return std::nullopt;
        }

    private:
        uint8_t probeId;
        bool leakDetected = false;
    };

    /**
     * @brief Leak detection logic.
     */
    class LeakLogic {
    public:
        /**
         * @brief Get the global logic singleton.
         */
        static LeakLogic& getInstance() {
            static LeakLogic instance;
            return instance;
        }

        /**
         * @brief Update the leak logic with the current sensor state and time elapsed since the last update.
         *
         * @param sensorState The current sensor state.
         * @param elapsedTime Time in seconds since the last update.
         */
        void update(const SensorState& sensorState, const time_t elapsedTime) {
            for (const auto& criterion : criteria) {
                criterion->update(sensorState, elapsedTime);
            }
        }

        /**
         * @brief Get the action determined by specified leak detection criteria.
         */
        [[nodiscard]] LeakPreventionAction getAction() const {
            for (auto& criterion : criteria) {
                if (const auto action = criterion->getAction()) {
                    return *action;
                }
            }
            return LeakPreventionAction(ActionType::NO_ACTION);
        }

        /**
         * @brief Add a criterion for leak detection.
         *
         * @param criterion A unique_ptr to a leak detection criterion object.
         */
        void addCriterion(std::unique_ptr<LeakDetectionCriterion> criterion) {
            criteria.Append(std::move(criterion));
        }

    private:
        StaticVector<std::unique_ptr<LeakDetectionCriterion>, LEAK_LOGIC_MAX_CRITERIA> criteria;
    };


}
#endif //LEAK_LOGIC_HPP
