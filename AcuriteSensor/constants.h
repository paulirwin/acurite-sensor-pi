#pragma once

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
