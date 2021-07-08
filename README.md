# acurite-sensor-pi
Code to pull data from the Acurite 00592TXR sensor on a Raspberry Pi with WiringPi.

Successfully decodes:
1. Temperature
2. Humidity
3. Channel (A/B/C for up to 3 devices)

Generously uses most of the Arduino code from https://github.com/bhunting/Acurite_00592TX_sniffer and adapted to standard C++17 with WiringPi, 
merged with the Raspberry Pi code for a different sensor from https://rayshobby.net/wordpress/reverse-engineer-wireless-temperature-humidity-rain-sensors-part-1/.

## Hardware

Connect a [433MHz superheterodyne receiver](https://www.sparkfun.com/products/10532) to your Pi. 
There are many pins on this receiver but you only need three: +5V, DATA (aka Digital Out), and GND. 
Connect +5V and GND to their respective pins, and DATA to pin 13 (WiringPi pin 2) on a 40-pin model like the 3 B+.

## Building

Enable SSH on your Pi. Currently this is a Visual Studio 2019 C++ project that uses SSH to your Pi to build and deploy. It builds on the pi with g++, not MSVC. 

Build the app in Visual Studio and it should prompt you for the SSH credentials and host/IP. You can see the build output in the Output pane. 
If it says build successful, that means it successfully copied the files to your Pi and built them on that device over SSH. (Note that on first load, it may take a
while for headers to be pulled from the Pi, so you might get red errors in the editor until they are synced.)

You should now be able to run and debug the app in Visual Studio, or navigate to the `/home/pi/projects/AcuriteSensor/bin/ARM/Debug` folder and run `./AcuriteSensor.out`.

## Future enhancements

I'd like this code to produce the data somewhere other than the console so that other apps on the Pi can easily use the data, maybe a UDP packet?

I'm also planning on adding a CMake project so that this can be easily built on the Pi without needing Visual Studio on another machine.
