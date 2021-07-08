#pragma once

// adapted from: https://stackoverflow.com/questions/21521282/basic-timer-with-stdthread-and-stdchrono

#include <thread>
#include <functional>

using std::thread;

class Timer
{
	thread th;

public:
	typedef std::chrono::milliseconds Interval;
	typedef std::function<void(void)> Timeout;

	void start(const Interval& interval, const Timeout& timeout)
	{
		th = thread([=]()
		{
			while (true)
			{
				std::this_thread::sleep_for(interval);
				timeout();
			}
		});
	}

	void join()
	{
		th.join();
	}
};
