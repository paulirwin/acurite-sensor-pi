#pragma once

#include <cstdint>
#include <mutex>
#include <queue>

class MessageQueue
{
public:
    static MessageQueue& getInstance()
    {
        static MessageQueue instance;

        return instance;
    }

    MessageQueue(MessageQueue const&) = delete;
    void operator=(MessageQueue const&) = delete;

    void addMessage(const uint64_t message);
    bool tryGetMessage(uint64_t& message);
private:
    MessageQueue() = default;
    ~MessageQueue() = default;

    static std::mutex queueMutex;
    std::queue<uint64_t> messageQueue = {};
    uint64_t lastMessage = 0;
};

