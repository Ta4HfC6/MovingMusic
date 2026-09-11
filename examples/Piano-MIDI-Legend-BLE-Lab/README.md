# Piano MIDI Legend BLE Lab

This sensor-free example turns an ESP32-S3 into a standard BLE-MIDI
controller named **MovingMusic S3**. It is intended to verify the complete
path to Piano MIDI Legend before gesture and distance sensors are added.

## Arduino IDE

1. Install the Espressif `esp32` board package, version 3.0.0 or newer.
2. Add this repository as an Arduino library (copy it into the sketchbook
   `libraries` directory, or install a ZIP made from the repository root).
3. Open `Piano-MIDI-Legend-BLE-Lab.ino` from the library examples.
4. Select the matching ESP32-S3 board. For a generic board, use
   **ESP32S3 Dev Module**.
5. Upload, then open Serial Monitor at **115200 baud** with **Newline** line
   ending.

No sensor wiring is needed. A normal USB data cable is sufficient.

## Connect and verify

1. Install and open **Bluetooth MIDI Connect** by ROCKRELAY APPS. Piano MIDI
   Legend does not scan BLE peripherals itself on the tested Android device.
2. Grant the Nearby devices/Bluetooth permission. Some Android releases also
   require Location permission and the global Location switch while scanning.
3. In Bluetooth MIDI Connect, scan for and connect to **MovingMusic S3**.
4. Leave the connector running and switch to Piano MIDI Legend. Do not rely
   only on the Android audio-Bluetooth pairing page: BLE-MIDI is a MIDI
   service, not a Bluetooth speaker profile.
5. The Serial Monitor should print `[BLE] connected`.
6. Enter `t`. The app should play C4, E4, G4, C5.

Useful follow-up commands:

```text
n 60 100 500   play middle C for 500 ms
vol 40         volume via CC7
rev 100        reverb via CC91
del 80         delay via CC92
pc 5           choose patch 5
next           next patch via Program Change 127
prev           previous patch via Program Change 126
pb 4096        pitch bend upward
pb 0           return pitch bend to center
sus 1          sustain on
sus 0          sustain off
panic          stop all notes
```

If the device is visible but does not connect, capture the complete Serial
Monitor output from boot through the connection attempt. If it connects but
is silent, first verify that tapping the app's own keyboard produces sound,
then send `n 60 100 1000` and report both the serial output and what the app
shows.

## Tested behavior (2026-09-10)

Tested with an ESP32-S3, arduino-esp32 3.3.10, Bluetooth MIDI Connect, and
Piano MIDI Legend on Android:

| MIDI input | Result | Good future sensor mapping |
|---|---|---|
| Note On/Off | Working | Discrete gesture triggers |
| Note velocity | Working, clearly audible | Gesture/motion speed |
| CC7 volume | Working, clearly audible | Continuous hand distance |
| CC64 sustain | Working, clearly audible | Open/closed hand state |
| Program Change 0..41 | Working | Swipe or pose selects sound |
| Pitch Bend | Affects the next Note On | Distance-defined pitch offset |
| Pitch Bend during a sounding note | No continuous glide heard | Avoid for continuous expression in this app |
| CC91 reverb | No audible change in current app state | Check effect enable/unlock state |
| CC92 delay | No audible echo in current app state | Check effect enable/unlock state |

The Pitch Bend transport and encoding were verified by sending the same note
at `-8192` and `+8191`; the second note was clearly higher. Piano MIDI Legend
did not retune an already sounding voice in this test.
