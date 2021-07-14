#include <chrono>
#include <iostream>

#include "AdafruitIoFeed.h"
#include "Channel.h"
#include "Message.h"
#include "MessageQueue.h"
#include "Sensor.h"
#include "Timer.h"

using std::cout, std::endl;
using std::optional;

int main(int argc, char* argv[])
{
    if (!Sensor::initialize())
    {
        cout << "No WiringPi detected" << endl;
        return -1;
    }

    cout << "WiringPi initialized successfully." << endl;

    const optional<AdafruitIoFeed> adafruitIoFeed = AdafruitIoFeed::tryCreate();

    if (adafruitIoFeed.has_value())
    {
        cout << "Found Adafruit IO env vars." << endl;
    }
    else
    {
        cout << "Not using Adafruit IO. To use, set ADAFRUIT_IO_USERNAME and ADAFRUIT_IO_KEY env vars." << endl;
    }

    Timer loopTimer;
    loopTimer.start(std::chrono::seconds(1), [adafruitIoFeed]
    {
        auto& queue = MessageQueue::getInstance();

        uint64_t message;
        while (queue.tryGetMessage(message)) {

            Message m(message);
            
#ifdef DISPLAY_DATA_BYTES
            cout << "Raw data: " << m.toBits() << endl;
#endif

            if (!m.parse())
            {
                cout << "Invalid checksum: " << m.toBits() << endl;
                continue;
            }

            const auto tempC = m.getTempC();

            if (tempC < 0 || tempC > 60)
            {
                cout << "Decoding error: " << m.toBits() << endl;
            }
            else
            {
                const auto tempF = m.getTempF();
                const auto humidity = m.getHumidity();
                const auto channel = m.getChannel();

                cout << '[' << channelToUpperChar(channel) << "]: "
                    << tempC << "*C, "
                    << tempF << "*F, "
                    << humidity << "% RH"
                    << endl;

                if (adafruitIoFeed.has_value())
                    adafruitIoFeed->postData(channel, tempF);
            }
        }
    });

    loopTimer.join();
}
