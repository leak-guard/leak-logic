#ifndef LEAK_LOGIC_HPP
#define LEAK_LOGIC_HPP

#include "leakguard/staticvector.hpp"
#include "leakguard/staticstring.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <cmath>
#include <ctime>

#define LEAK_LOGIC_MAX_CRITERIA 10
#define LEAK_LOGIC_MAX_SERIALIZE_LENGTH 256



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
        StaticVector<bool, 256> probeStates;
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

        [[nodiscard]] virtual StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialize() const = 0;
        static std::unique_ptr<LeakDetectionCriterion> deserialize(const StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH>& serialized);
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

        [[nodiscard]] float getRateThreshold() const { return rateThreshold; }
        [[nodiscard]] time_t getMinDuration() const { return minDuration; }

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

        [[nodiscard]] StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialize() const override {
            StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialized;
            serialized += StaticString<8>("T,");
            serialized += StaticString<8>::Of(static_cast<int>(rateThreshold * 100));
            serialized += StaticString<1>(",");
            serialized += StaticString<8>::Of(static_cast<int>(minDuration));
            serialized += StaticString<1>(",");

            return serialized;
        }

        static std::unique_ptr<TimeBasedFlowRateCriterion> deserialize(const StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH>& serialized) {
            StaticString<16> buffer;

            enum BufferState { TYPE, RATE_THRESH, MIN_DURATION };
            BufferState state = TYPE;

            float rateThreshold = 0.0f;
            time_t minDuration = 0;

            for (int i = 0; i < serialized.GetLength(); i++) {
                const char c = serialized[i];
                buffer += c;

                if (c == ',') {
                    buffer.Truncate(buffer.GetLength() - 1);
                    switch (state) {
                        case TYPE:
                            state = RATE_THRESH;
                        break;
                        case RATE_THRESH:
                            rateThreshold = static_cast<float>(buffer.ToInteger<int>()) / 100.0f;
                            state = MIN_DURATION;
                        break;
                        case MIN_DURATION:
                            minDuration = buffer.ToInteger<int>();
                            auto criterion = std::make_unique<TimeBasedFlowRateCriterion>(rateThreshold, minDuration);
                            return criterion;
                    }
                    buffer.Clear();
                }
            }

            return nullptr;
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

        [[nodiscard]] int getProbeId() const { return probeId; }

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

        [[nodiscard]] StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialize() const override {
            StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialized;
            serialized += StaticString<8>("P,");
            serialized += StaticString<8>::Of(probeId);
            serialized += StaticString<1>(",");

            return serialized;
        }

        static std::unique_ptr<ProbeLeakDetectionCriterion> deserialize(const StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH>& serialized) {
            StaticString<16> buffer;

            enum BufferState { TYPE, PROBE_ID };
            BufferState state = TYPE;

            int probeId = 0;

            for (int i = 0; i < serialized.GetLength(); i++) {
                const char c = serialized[i];
                buffer += c;

                if (c == ',') {
                    buffer.Truncate(buffer.GetLength() - 1);
                    switch (state) {
                        case TYPE:
                            state = PROBE_ID;
                        break;
                        case PROBE_ID:
                            probeId = buffer.ToInteger<int>();
                            return std::make_unique<ProbeLeakDetectionCriterion>(probeId);
                    }
                    buffer.Clear();
                }
            }

            return nullptr;
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
         * @return Whether the criterion was added successfully.
         */
        bool addCriterion(std::unique_ptr<LeakDetectionCriterion> criterion) {
            return criteria.Append(std::move(criterion));
        }

        /**
         * @brief Gets an iterator for the leak detection criteria list.
         */
        StaticVector<std::unique_ptr<LeakDetectionCriterion>, LEAK_LOGIC_MAX_CRITERIA>::Iterator getCriteria() {
            return criteria.begin();
        }

        /**
         * @brief Removes a criterion for leak detection from the list.
         *
         * @param index Index of the criterion to remove.
         * @return Whether the criterion was removed successfully.
         */
        bool removeCriterion(const uint8_t index) {
            return criteria.RemoveIndex(index);
        }

        void clearCriteria()
        {
            criteria.Clear();
        }

        [[nodiscard]] StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialize() const {
            StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> serialized;

            for (int i = 0; i < criteria.GetSize(); i++) {
                serialized += criteria[i]->serialize();
                serialized += StaticString<1>("|");
            }

            return serialized;
        }

        void loadFromString(const StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH>& serialized) {
            StaticString<LEAK_LOGIC_MAX_SERIALIZE_LENGTH> buffer;
            for (int i = 0; i < serialized.GetLength(); i++) {
                const char c = serialized[i];
                buffer += c;

                if (c == '|') {
                    buffer.Truncate(buffer.GetLength() - 1);
                    switch (buffer[0]) {
                        case 'T':
                            addCriterion(TimeBasedFlowRateCriterion::deserialize(buffer));
                        break;
                        case 'P':
                            addCriterion(ProbeLeakDetectionCriterion::deserialize(buffer));
                        break;
                        default:
                            break;
                    }
                    buffer.Clear();
                }
            }
        }


    private:
        StaticVector<std::unique_ptr<LeakDetectionCriterion>, LEAK_LOGIC_MAX_CRITERIA> criteria;
    };


}
#endif //LEAK_LOGIC_HPP
