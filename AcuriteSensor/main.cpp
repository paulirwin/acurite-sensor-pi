#include <wiringPi.h>
#include <cstdlib>
#include <iostream>
#include <bitset>

using std::cout, std::endl;
using std::bitset;

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
bool received = false; // true if sync plus enough bits found
uint32_t changeCount = 0;

const uint32_t tempOffset10th = 0; // offset in 10th degrees C

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

void handler()
{
	static uint32_t duration = 0;
	static uint32_t lastTime = 0;
	static uint32_t ringIndex = 0;
	
	if (received == true)
	{
		return;
	}

	const uint32_t time = micros();
	duration = time - lastTime;
	lastTime = time;

	// Known error in bit stream is runt/short pulses.
	// If we ever get a really short, or really long,
	// pulse we know there is an error in the bit stream
	// and should start over.
	if ((duration > (PULSE_LONG + 100)) || (duration < (PULSE_SHORT - 100)))
	{
		received = false;
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
		// if not enough bits yet, no message received yet
		if (changeCount < DATABITSEDGES)
		{
			received = false;
		}
		else if (changeCount > DATABITSEDGES)
		{
			// if too many bits received then reset and start over
			received = false;
			syncFound = false;
		}
		else
		{
			received = true;
		}
	}
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
	{
		return 1;
	}
	else if (t0 > (BIT0_HIGH - 100) && t0 < (BIT0_HIGH + 100) && t1 > (BIT0_LOW - 100) && t1 < (BIT0_LOW + 100))
	{
		return 0;
	}
	return -1; // undefined
}

enum channel { UNKNOWN, A, B, C };

channel getChannel()
{
	uint32_t channel = 0;

	const uint32_t startIndex = dataIndex % RING_BUFFER_SIZE;
	const uint32_t stopIndex = (dataIndex + 4) % RING_BUFFER_SIZE;
	for (uint32_t i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
	{
		const auto bit = convertTimingToBit(pulseDurations[i], pulseDurations[(i + 1) % RING_BUFFER_SIZE]);
		channel = (channel << 1) + bit;
		if (bit < 0)
			return UNKNOWN;
	}

	switch (channel)
	{
	case 3: // 11
		return A;
	case 2: // 10
		return B;
	case 0: // 00
		return C;
	default:
		return UNKNOWN;
	}
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

float getTemperature()
{
	uint32_t temp = 0;

	// most significant 4 bits
	uint32_t startIndex = (dataIndex + (4 * 8 + 4) * 2) % RING_BUFFER_SIZE;
	uint32_t stopIndex = (dataIndex + (4 * 8 + 8) * 2) % RING_BUFFER_SIZE;
	for (uint32_t i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
	{
		const auto bit = convertTimingToBit(pulseDurations[i], pulseDurations[(i + 1) % RING_BUFFER_SIZE]);
		temp = (temp << 1) + bit;
		if (bit < 0)
			return -1.0;
	}
	// least significant 7 bits
	startIndex = (dataIndex + (5 * 8 + 1) * 2) % RING_BUFFER_SIZE;
	stopIndex = (dataIndex + (5 * 8 + 8) * 2) % RING_BUFFER_SIZE;
	for (uint32_t i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
	{
		const auto bit = convertTimingToBit(pulseDurations[i % RING_BUFFER_SIZE],
			pulseDurations[(i + 1) % RING_BUFFER_SIZE]);
		temp = (temp << 1) + bit; // shift and insert next bit
		if (bit < 0)
			return -1.0;
	}

	return static_cast<float>((temp - 1024) + tempOffset10th + 0.5) / 10;
}

uint32_t getHumidity()
{
	uint32_t humidity = 0;

	const uint32_t startIndex = (dataIndex + (3 * 8 + 1) * 2) % RING_BUFFER_SIZE;
	const uint32_t stopIndex = (dataIndex + (4 * 8) * 2) % RING_BUFFER_SIZE;
	for (uint32_t i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
	{
		const auto bit = convertTimingToBit(pulseDurations[i], pulseDurations[(i + 1) % RING_BUFFER_SIZE]);
		humidity = (humidity << 1) + bit;
		if (bit < 0)
			return -1;
	}
	
	return humidity;
}

void displayDataBytes()
{
	uint8_t dataBytes[DATABYTESCNT] = {0};
	uint32_t ringIndex = (syncIndex + 1) % RING_BUFFER_SIZE;
	bool fail = false;

	for (int i = 0; i < DATABITSCNT; i++)
	{
		const auto bit = convertTimingToBit(pulseDurations[ringIndex % RING_BUFFER_SIZE],
		                                   pulseDurations[(ringIndex + 1) % RING_BUFFER_SIZE]);

		if (bit < 0)
		{
			fail = true;
			break;      // exit loop
		}
		else
		{
			dataBytes[i / 8] |= bit << (7 - (i % 8));
		}

		ringIndex += 2;
	}

	if (fail)
	{
		cout << "Data Byte Display : Decoding error." << endl;
	}
	else
	{
		cout << "Raw Data: ";
		
		for (uint8_t& dataByte : dataBytes)
		{
			bitset<8> bs(static_cast<uint32_t>(dataByte));
			cout << bs;
		}
		
		cout << endl;
	}
}

int main(int argc, char* argv[])
{	
	if (wiringPiSetup() == -1)
	{
		cout << "No WiringPi detected" << endl;
		return 0;
	}
	else
	{
		cout << "WiringPi initialized successfully." << endl;
	}
	
	wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);
		
	while (true)
	{
		if (received != true)
			continue;

		system("/usr/bin/gpio edge 2 none");

#ifdef DISPLAY_DATA_BYTES
		displayDataBytes();
#endif

		const channel channel = getChannel();
		const float tempC = getTemperature();
		const uint32_t humidity = getHumidity();

		if (tempC < 0)
		{
			cout << "Decoding error." << endl;
		}
		else
		{
			const float tempF = tempC * 9 / 5 + 32;

			cout << '[' << channelToChar(channel) << "]: " << tempC << "*C, " << tempF << "*F, " << humidity << "% RH" << endl;
		}

		delay(1000);
		received = false;
		syncFound = false;
		wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);
	}
}
