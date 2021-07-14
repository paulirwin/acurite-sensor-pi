#include "MessageQueue.h"

using std::unique_lock;

std::mutex MessageQueue::queueMutex;

void MessageQueue::addMessage(const uint64_t message)
{
    unique_lock lock(queueMutex);

    if (message != lastMessage) {
        messageQueue.push(message);
        lastMessage = message;
    }
}

bool MessageQueue::tryGetMessage(uint64_t& message)
{
    unique_lock lock(queueMutex);

    if (messageQueue.empty())
        return false;

    message = messageQueue.front();
    messageQueue.pop();

    return true;
}
