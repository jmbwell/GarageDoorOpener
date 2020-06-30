# GarageDoorOpener
A simple HomeKit GarageDoorOpener implemented using https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266

This implements a HAP GarageDoorOpener accessory and service. I have it running on a NodeMCU module. It provides current door state and target door state.

## Open and close sensors

In my installation, there is one magnetic reed switch clamped on the lower portion of the track and another clamped on the upper portion of the track. These are positioned so that, when the door is fully closed, the magnet on the door is aligned with the lower switch, and when the door is fully open, the magnet is aligned with the upper switch. This gives me a reliable indicator of the door's open or closed state.

In addition to open or closed, HomeKit expects to know whether the door is opening or closing. To determine that, I use some logic; if neither switch is open, then it's "opening" when the previous state was "closed" and "closing" if the previous state was "open."

HomeKit also expects the target and current state to be in sync. That's handled automatically when HomeKit is issuing commands, but if the door is operated using the button in the garage, then we have to notify HomeKit ourselves. So if the state changes outside of HomeKit's control, we apply some logic to determine what the new target state should be.

So that everything can respond to the switches as soon as their state changes, I attach interrupts to those pins. The interrupt function just sets a global variable, which if TRUE when the next loop() runs, handles the state change inside the loop.

To operate the door, I take D5 high, which activates a transistor (an 8050 in my case), which in turn energizes a relay to close the normally-open contacts, which are connected in parallel with the door operator's own button. A resistor pulls D5 to ground otherwise, helping prevent accidentally activating the door when resetting the module.

## Usage

If you want a setup like mine:
- Build your circuit according to the schematic
- Add your wifi_info.h file configured for your wifi (see Mixiaoxiao's examples)
- Consider changing the name of the accessory and service in my_accessory.c
- Compile, upload, pair the device in HomeKit
- Shut the door, clamp a reed switch to the track (so that it won't interfere with the door's motion, obviously), and mount the magnet to the door in alignment with the switch
- Open the door, and clamp a reed switch to the track so that it aligns with the magnet
- Wire the "closed" sensor to D1, wire the "open" sensor to D2
- Wire the relay's normally open outputs in parallel with your door operator's button — assuming your door operator can be signaled to operate with a simple contact closure (check this!)
- Power the module somehow (USB adapter plugged into the outlet in the ceiling perhaps?)


## To-do:
- Detect an obstruction
- Clean up all the code, add proper header files, etc.
