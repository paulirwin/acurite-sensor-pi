#pragma once

#include "constants.h"
#include "MessageQueue.h"

class Sensor
{
public:
    static bool initialize();

private:
    Sensor() = default;

    static void handler();
    static uint64_t getMessageFromTiming();
    static bool isSync(uint32_t idx);
    
    // The pulse durations are the measured time in micro seconds between pulse edges.
    static uint32_t pulseDurations[RING_BUFFER_SIZE];
    static uint32_t syncIndex; // index of the last bit time of the sync signal
    static uint32_t dataIndex; // index of the first bit time of the data bits (syncIndex+1)
    static bool syncFound; // true if sync pulses found
    static uint32_t changeCount;
};

