#ifndef LEAK_LOGIC_HPP
#define LEAK_LOGIC_HPP

#include "leakguard/staticvector.hpp"

#include <cstdint>
#include <memory>
#include <optional>

#define LEAK_LOGIC_MAX_CRITERIA 10

namespace lg {

    enum class ActionType {
        NO_ACTION,
        CLOSE_VALVE
    };

    enum class ActionReason {
        NONE,
        EXCEEDED_FLOW_RATE,
        LEAK_DETECTED_BY_PROBE
    };

    struct SensorState {
        float flowRate;
        StaticVector<bool, 256>& probeStates;
    };

    class LeakPreventionAction {
    public:
        explicit LeakPreventionAction(
            const ActionType actionType = ActionType::NO_ACTION,
            const ActionReason reason = ActionReason::NONE,
            const uint8_t probeId = -1)
                : actionType(actionType), reason(reason), probeId(probeId) {}

        [[nodiscard]] ActionType getActionType() const { return actionType; }
        [[nodiscard]] ActionReason getActionReason() const { return reason; }
        [[nodiscard]] uint8_t getProbeId() const { return probeId; }

    private:
        ActionType actionType;
        ActionReason reason;
        uint8_t probeId;
    };

    class LeakDetectionCriterion {
    public:
        virtual void update(const SensorState& sensorState, time_t elapsedTime) = 0;

        [[nodiscard]] virtual std::optional<LeakPreventionAction> getAction() const = 0;

        virtual ~LeakDetectionCriterion() = default;
    };

    class TimeBasedFlowRateCriterion final : public LeakDetectionCriterion {
    public:
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
        static LeakLogic& getInstance() {
            static LeakLogic instance;
            return instance;
        }

        void update(const SensorState& sensorState, const time_t elapsedTime) {
            for (const auto& criterion : criteria) {
                criterion->update(sensorState, elapsedTime);
            }
        }

        [[nodiscard]] LeakPreventionAction getAction() const {
            for (auto& criterion : criteria) {
                if (const auto action = criterion->getAction()) {
                    return *action;
                }
            }
            return LeakPreventionAction(ActionType::NO_ACTION);
        }

        void addCriterion(std::unique_ptr<LeakDetectionCriterion> criterion) {
            criteria.Append(std::move(criterion));
        }

    private:
        StaticVector<std::unique_ptr<LeakDetectionCriterion>, LEAK_LOGIC_MAX_CRITERIA> criteria;
    };


}
#endif //LEAK_LOGIC_HPP
