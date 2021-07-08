#include <bitset>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <queue>
#include <wiringPi.h>

#include "timer.h"

using std::cout, std::endl;
using std::bitset;
using std::queue;
using std::mutex;
using std::unique_lock;

constexpr auto RING_BUFFER_SIZE = 128;
constexpr auto SYNC_HIGH = 600;
constexpr auto SYNC_LOW = 600;
constexpr auto PULSE_LONG = 400;
constexpr auto PULSE_SHORT = 220;
constexpr auto BIT1_HIGH = PULSE_LONG;
constexpr auto BIT1_LOW = PULSE_SHORT;
constexpr auto BIT0_HIGH = PULSE_SHORT;
constexpr auto BIT0_LOW = PULSE_LONG;

constexpr auto DATA_PIN = 2; // wiringPi GPIO 2 (Pin 13 on RPi 3 B+)

constexpr auto SYNCPULSECNT = 4; // 4 pulses (8 edges)
constexpr auto SYNCPULSEEDGES = SYNCPULSECNT * 2;
constexpr auto DATABYTESCNT = 7;
constexpr auto DATABITSCNT = DATABYTESCNT * 8; // 7 bytes * 8 bits
constexpr auto DATABITSEDGES = DATABITSCNT * 2;

// The pulse durations are the measured time in micro seconds between
// pulse edges.
uint32_t pulseDurations[RING_BUFFER_SIZE];
uint32_t syncIndex = 0; // index of the last bit time of the sync signal
uint32_t dataIndex = 0; // index of the first bit time of the data bits (syncIndex+1)
bool syncFound = false; // true if sync pulses found
uint32_t changeCount = 0;

mutex queueMutex;
queue<uint64_t> messageQueue = {};
uint64_t lastMessage = 0;

bool isSync(const uint32_t idx)
{
    // check if we've received 4 pulses of matching timing
    for (int i = 0; i < SYNCPULSEEDGES; i += 2)
    {
        const uint32_t t1 = pulseDurations[(idx + RING_BUFFER_SIZE - i) % RING_BUFFER_SIZE];
        const uint32_t t0 = pulseDurations[(idx + RING_BUFFER_SIZE - i - 1) % RING_BUFFER_SIZE];

        // any of the preceding 8 pulses are out of bounds, short or long,
        // return false, no sync found
        if (t0 < (SYNC_HIGH - 100) || t0 > (SYNC_HIGH + 100) ||
            t1 < (SYNC_LOW - 100) || t1 > (SYNC_LOW + 100))
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
    if (t0 > (BIT1_HIGH - 100) && t0 < (BIT1_HIGH + 100) && t1 > (BIT1_LOW - 100) && t1 < (BIT1_LOW + 100))
        return 1;

    if (t0 > (BIT0_HIGH - 100) && t0 < (BIT0_HIGH + 100) && t1 > (BIT0_LOW - 100) && t1 < (BIT0_LOW + 100))
        return 0;

    return -1; // undefined
}

void addMessageToQueue(const uint64_t message)
{
    unique_lock lock(queueMutex);
    messageQueue.push(message);
}

uint64_t getMessageFromTiming()
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

void handler()
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

            if (message != lastMessage) {
                addMessageToQueue(message);
                lastMessage = message;
            }

            syncFound = false;
            delay(1000);
        }
        else if (changeCount > DATABITSEDGES)
        {
            syncFound = false;
        }
    }
}

enum channel { UNKNOWN, A, B, C };

channel getChannel(const bitset<DATABITSCNT>& bits)
{
    const bool bit0 = bits[DATABITSCNT - 1];
    const bool bit1 = bits[DATABITSCNT - 2];

    if (bit0 && bit1)
        return A;
    if (bit0)
        return B;
    if (!bit0 && !bit1)
        return C;

    return UNKNOWN;
}

char channelToChar(const channel channel)
{
    switch (channel)
    {
    case A:
        return 'A';
    case B:
        return 'B';
    case C:
        return 'C';
    default:
        return '?';
    }
}

float getTemperature(const bitset<DATABITSCNT>& bits)
{
    uint32_t temp = 0;

    // most significant 4 bits
    for (int i = (4 * 8 + 4); i < (4 * 8 + 8); i++)
    {
        temp = (temp << 1) + bits[DATABITSCNT - i - 1];
    }

    // least significant 7 bits
    for (int i = (5 * 8 + 1); i < (5 * 8 + 8); i++)
    {
        temp = (temp << 1) + bits[DATABITSCNT - i - 1];
    }

    return static_cast<float>((temp - 1024) + 0.5) / 10;
}

uint32_t getHumidity(const bitset<DATABITSCNT>& bits)
{
    uint32_t humidity = 0;

    for (int i = (3 * 8 + 1); i < (4 * 8); i++)
    {
        humidity = (humidity << 1) + bits[DATABITSCNT - i - 1];
    }

    return humidity;
}

bool getMessageFromQueue(uint64_t& message)
{
    unique_lock lock(queueMutex);

    if (messageQueue.empty())
        return false;

    message = messageQueue.front();
    messageQueue.pop();

    return true;
}

bool checksum(const uint64_t message)
{
    uint32_t runningSum = 0;
    const uint8_t expected = message & 0xff;

    for (int i = 1; i < 7; i++)
    {
        runningSum += (message >> (i * 8)) & 0xff;
    }

    return (runningSum % 256) == expected;
}

int main(int argc, char* argv[])
{
    if (wiringPiSetup() == -1)
    {
        cout << "No WiringPi detected" << endl;
        return -1;
    }

    cout << "WiringPi initialized successfully." << endl;

    wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);

    Timer loopTimer;
    loopTimer.start(std::chrono::seconds(1), []
    {
        uint64_t message;
        while (getMessageFromQueue(message)) {

            const bitset<DATABITSCNT> bits(message);

#ifdef DISPLAY_DATA_BYTES
            cout << "Raw data: " << bits << endl;
#endif

            if (!checksum(message))
            {
                cout << "Invalid checksum: " << bits << endl;
                continue;
            }

            const channel channel = getChannel(bits);
            const float tempC = getTemperature(bits);
            const uint32_t humidity = getHumidity(bits);

            if (tempC < 0)
            {
                cout << "Decoding error." << endl;
            }
            else
            {
                const float tempF = tempC * 9 / 5 + 32;

                cout << '[' << channelToChar(channel) << "]: " << tempC << "*C, " << tempF << "*F, " << humidity << "% RH"
                    << endl;
            }
            
            syncFound = false;
            wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);
        }
    });

    loopTimer.join();
}
