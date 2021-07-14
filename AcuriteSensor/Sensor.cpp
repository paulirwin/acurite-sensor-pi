#include "Sensor.h"

#include <cstdint>
#include <wiringPi.h>

#include "constants.h"

uint32_t Sensor::pulseDurations[RING_BUFFER_SIZE];
uint32_t Sensor::syncIndex;
uint32_t Sensor::dataIndex;
bool Sensor::syncFound;
uint32_t Sensor::changeCount;

bool Sensor::initialize()
{
    if (wiringPiSetup() == -1)
        return false;

    pullUpDnControl(DATA_PIN, PUD_DOWN);
    wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);
    
    return true;
}

bool Sensor::isSync(const uint32_t idx)
{
    // check if we've received 4 pulses of matching timing
    for (int i = 0; i < SYNCPULSEEDGES; i += 2)
    {
        const uint32_t t1 = pulseDurations[(idx + RING_BUFFER_SIZE - i) % RING_BUFFER_SIZE];
        const uint32_t t0 = pulseDurations[(idx + RING_BUFFER_SIZE - i - 1) % RING_BUFFER_SIZE];

        // any of the preceding 8 pulses are out of bounds, short or long,
        // return false, no sync found
        if (t0 < (SYNC_HIGH - 100) || t0 >(SYNC_HIGH + 100) ||
            t1 < (SYNC_LOW - 100) || t1 >(SYNC_LOW + 100))
        {
            return false;
        }
    }
    return true;
}

/*
 * Convert pulse durations to bits.
 *
 * 1 bit ~0.4 ms high followed by ~0.2 ms low
 * 0 bit ~0.2 ms high followed by ~0.4 ms low
 */
int8_t convertTimingToBit(const uint32_t t0, const uint32_t t1)
{
    if (t0 > (BIT1_HIGH - 100) && t0 < (BIT1_HIGH + 100) && t1 >(BIT1_LOW - 100) && t1 < (BIT1_LOW + 100))
        return 1;

    if (t0 > (BIT0_HIGH - 100) && t0 < (BIT0_HIGH + 100) && t1 >(BIT0_LOW - 100) && t1 < (BIT0_LOW + 100))
        return 0;

    return -1; // undefined
}

uint64_t Sensor::getMessageFromTiming()
{
    uint64_t message = 0;

    for (uint32_t i = dataIndex % RING_BUFFER_SIZE, j = 0; j < DATABITSCNT; i = (i + 2) % RING_BUFFER_SIZE, j++)
    {
        const auto bit = convertTimingToBit(pulseDurations[i], pulseDurations[(i + 1) % RING_BUFFER_SIZE]);
        if (bit < 0)
        {
            return 0;
        }

        message = (message << 1) | bit;
    }

    return message;
}

void Sensor::handler()
{
    static uint32_t duration = 0;
    static uint32_t lastTime = 0;
    static uint32_t ringIndex = 0;

    const uint32_t time = micros();
    duration = time - lastTime;
    lastTime = time;

    // Known error in bit stream is runt/short pulses.
    // If we ever get a really short, or really long,
    // pulse we know there is an error in the bit stream
    // and should start over.
    if ((duration > (PULSE_LONG + 100)) || (duration < (PULSE_SHORT - 100)))
    {
        syncFound = false;
        changeCount = 0; // restart looking for data bits
    }

    // store data in ring buffer
    ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;
    pulseDurations[ringIndex] = duration;
    changeCount++; // found another edge

    // detect sync signal
    if (isSync(ringIndex))
    {
        syncFound = true;
        changeCount = 0; // restart looking for data bits
        syncIndex = ringIndex;
        dataIndex = (syncIndex + 1) % RING_BUFFER_SIZE;
    }

    // If a sync has been found the start looking for the
    // DATABITSEDGES data bit edges.
    if (syncFound)
    {
        if (changeCount == DATABITSEDGES)
        {
            const auto message = getMessageFromTiming();
            MessageQueue::getInstance().addMessage(message);

            syncFound = false;
            delay(1000);
        }
        else if (changeCount > DATABITSEDGES)
        {
            syncFound = false;
        }
    }
}
