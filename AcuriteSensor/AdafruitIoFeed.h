#pragma once

#include <optional>
#include <string>

#include "Channel.h"

using std::optional;
using std::string;

class AdafruitIoFeed
{
public:
    static optional<AdafruitIoFeed> tryCreate();

    void postData(const Channel channel, const float tempF) const;
private:
    AdafruitIoFeed(const string username, const string key)
        : username(username),
          key(key)
    {
    }
    
    string username;
    string key;
};
