#include "Message.h"

bool checksum(const uint64_t message)
{
    uint32_t runningSum = 0;
    const uint8_t expected = message & 0xff;

    for (int i = 1; i < 7; i++)
    {
        runningSum += static_cast<uint32_t>((message >> (i * 8)) & 0xff);
    }

    return (runningSum % 256) == expected;
}

float getTemperatureFromBits(const bitset<DATABITSCNT>& bits)
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

uint32_t getHumidityFromBits(const bitset<DATABITSCNT>& bits)
{
    uint32_t humidity = 0;

    for (int i = (3 * 8 + 1); i < (4 * 8); i++)
    {
        humidity = (humidity << 1) + bits[DATABITSCNT - i - 1];
    }

    return humidity;
}

Channel getChannelFromBits(const std::bitset<DATABITSCNT>& bits)
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

bool Message::parse()
{
    if (!checksum(message))
        return false;

    channel = getChannelFromBits(bits);
    tempC = getTemperatureFromBits(bits);
    humidity = getHumidityFromBits(bits);

    return true;
}
