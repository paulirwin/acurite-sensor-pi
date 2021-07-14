#include "AdafruitIoFeed.h"

#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

unsigned dummyWriteCurlResponseData(char* buffer, const unsigned size, const unsigned nmemb, void* userp)
{
    return size * nmemb;
}

optional<AdafruitIoFeed> AdafruitIoFeed::tryCreate()
{
    const char* adafruitUsernameEnv = getenv("ADAFRUIT_IO_USERNAME");
    const char* adafruitKeyEnv = getenv("ADAFRUIT_IO_KEY");

    if (adafruitUsernameEnv != nullptr && adafruitKeyEnv != nullptr)
    {
        return AdafruitIoFeed(adafruitUsernameEnv, adafruitKeyEnv);
    }

    return {};
}

void AdafruitIoFeed::postData(const Channel channel, const float tempF) const
{
    if (channel == UNKNOWN)
        return;
    
    try {
        curlpp::Easy request;

        request.setOpt(new curlpp::options::Url("https://io.adafruit.com/api/v2/" + username + "/feeds/acurite-sensor-" + channelToLowerChar(channel) + "-temp/data"));
        request.setOpt(new curlpp::options::Verbose(false));
        request.setOpt(new curlpp::options::WriteFunctionCurlFunction(dummyWriteCurlResponseData));

        std::list<std::string> header;
        header.push_back("X-AIO-Key: " + key);

        request.setOpt(new curlpp::options::HttpHeader(header));
        request.setOpt(new curlpp::options::PostFields("value=" + std::to_string(tempF)));

        request.perform();
    }
    catch (curlpp::LogicError& e) {
        std::cout << e.what() << std::endl;
    }
    catch (curlpp::RuntimeError& e) {
        std::cout << e.what() << std::endl;
    }
}
