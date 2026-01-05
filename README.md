# ChargeeSparkyNoCloud

Chargee Sparky DSMR reader without for Home Assistant without cloud access

## WARNING! Flashing this code will remove any existing code. You will not be able to revert back to the stock firmware.

## Why

I recently bought a [Sparky](https://www.chargee.energy/en/sparky-p1-meter) because they state on their website that it can integrate with Home Assistant. While it can, it cannot do it without cloud access. This defeats the purpose of having Home Assistant in the first place. So of course I reverse engineered the thing, which was made pretty easy. 

## How

Sparky contains an ESP32-C3-Mini-1 microcontroller. The first thing I had to do was check if I could write new firmware to it. At first it did not work, it's not writeable when it runs the original code. But... keep the reset(/boot) button pressed for about 5 seconds and the LED turns blue. Now you can write to it.

So what I want it to do is expose an endpoint for the [DSMR Smart Meter HA integration](https://www.home-assistant.io/integrations/dsmr/) just like the original, which is nothing more but a Serial to TCP/IP bridge. For this to work I needed to know on which port Pin 5 of the P1 port is connected. I was able figure it out by brute forcing GPIO pins and checking which one actually came back with DSMR telegrams.

To figure out the pins for the LED's I essentially did the same.

## Pinout

| PIN | Connected to |
| --- | ------------ |
| 5   | Blue LED     |
| 6   | Green LED    | 
| 7   | Red LED      |
| 10  | RX for P1    |

## Todo

I will probably not work on this code as it works for my situation. The code as it stands assumes:

1. You're using DSMR5
2. You're hardcoding WiFi SSID + Password
3. You have a way of finding out the IP-address of the device

## Connect to the device with Home Assistant

Just like the original:

1. `Open` the Home Assistant app
2. Go to `Settings > Devices & Services > +Add Integration`
3. Search for and select DSMR Smart Meter
4. Under 'Select connection type', choose `network`, then click `submit`
5. On the next screen, fill in the details below:
* Host: enter the `IP address` of your Sparky here
* Port: fill in `3602`
* Select DSMR version: enter the version number of your smart meter. Choose `5`.

# Last notes

If anyone wants to make to code suitable for other versions of the protocol and make it more robust, be my guest! Happy hacking!
