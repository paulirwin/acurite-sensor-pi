#pragma once

#include <bitset>
#include <cstdint>

#include "Channel.h"
#include "constants.h"

using std::bitset;

class Message
{
public:
    explicit Message(const uint64_t message)
        : message(message),
          bits(message),
          channel(UNKNOWN),
          tempC(0),
          humidity(0)
    {
    }

    bool parse();

    [[nodiscard]] const bitset<DATABITSCNT>& toBits() const
    {
        return bits;
    }

    [[nodiscard]] Channel getChannel() const
    {
        return channel;
    }

    [[nodiscard]] float getTempC() const
    {
        return tempC;
    }

    [[nodiscard]] float getTempF() const
    {
        return tempC * 9 / 5 + 32;
    }

    [[nodiscard]] uint32_t getHumidity() const
    {
        return humidity;
    }

private:
    const uint64_t message;
    const bitset<DATABITSCNT> bits;
    Channel channel;
    float tempC;
    uint32_t humidity;
};
