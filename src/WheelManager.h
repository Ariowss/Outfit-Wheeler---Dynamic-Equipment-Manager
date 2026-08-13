#pragma once
#include <vector>
#include <cstdint>

namespace Wheeler {

    struct ItemData {
        RE::FormID formID;
        uint16_t uniqueID;
    };

    struct Slot {
        std::vector<ItemData> items;
        int activeIndex = 0;
    };

    struct Wheel {
        std::vector<Slot> slots;
    };

    class WheelManager {
    public:
        static WheelManager& GetSingleton() {
            static WheelManager instance;
            return instance;
        }

        std::vector<Wheel> wheels;
        int activeWheelIndex = 0;

        void Reset() {
            wheels.clear();
            wheels.emplace_back();
            activeWheelIndex = 0;
        }

    private:
        WheelManager() = default;
        ~WheelManager() = default;
        WheelManager(const WheelManager&) = delete;
        WheelManager& operator=(const WheelManager&) = delete;
    };

}