// ESP32_Host_MIDI / Piano MIDI Legend BLE Lab
//
// A sensor-free bring-up sketch for an ESP32-S3 and Piano MIDI Legend.
// The ESP32 advertises as a standard BLE-MIDI peripheral. Use the Serial
// Monitor to send notes and the controls documented by Piano MIDI Legend.

#include <Arduino.h>
#include <ESP32_Host_MIDI.h>
#include <BLEConnection.h>

static constexpr char DEVICE_NAME[] = "MovingMusic S3";
static constexpr uint8_t MIDI_CHANNEL = 1;

BLEConnection bleMidi;

struct TimedNote {
    bool active = false;
    uint8_t note = 60;
    uint32_t offAtMs = 0;
};

static TimedNote timedNote;
static bool wasConnected = false;
static bool demoRunning = false;
static uint8_t demoStep = 0;
static uint32_t nextDemoEventMs = 0;

static const uint8_t DEMO_NOTES[] = {60, 64, 67, 72};  // C4 E4 G4 C5

static int clampInt(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static bool sendRaw2(uint8_t status, uint8_t data1) {
    uint8_t message[2] = {status, data1};
    return bleMidi.sendMidiMessage(message, sizeof(message));
}

static void noteOffNow() {
    if (!timedNote.active) return;
    midiHandler.sendNoteOff(MIDI_CHANNEL, timedNote.note, 0);
    timedNote.active = false;
}

static void playTimedNote(uint8_t note, uint8_t velocity, uint32_t durationMs) {
    noteOffNow();
    if (!midiHandler.sendNoteOn(MIDI_CHANNEL, note, velocity)) {
        Serial.println("[SEND] failed: no BLE-MIDI connection");
        return;
    }

    timedNote.active = true;
    timedNote.note = note;
    timedNote.offAtMs = millis() + durationMs;
    Serial.printf("[SEND] NoteOn note=%u velocity=%u duration=%lu ms\n",
                  note, velocity, (unsigned long)durationMs);
}

static void panic() {
    noteOffNow();
    demoRunning = false;
    // CC123 = All Notes Off; CC120 = All Sound Off.
    midiHandler.sendControlChange(MIDI_CHANNEL, 123, 0);
    midiHandler.sendControlChange(MIDI_CHANNEL, 120, 0);
    Serial.println("[SEND] panic: All Notes Off + All Sound Off");
}

static void printHelp() {
    Serial.println();
    Serial.println("Piano MIDI Legend BLE Lab commands:");
    Serial.println("  h                 show this help");
    Serial.println("  s                 show BLE status");
    Serial.println("  t                 play/stop C-major connection test");
    Serial.println("  n NOTE VEL MS     play a timed note (0..127)");
    Serial.println("  cc NUM VALUE      send Control Change (0..127)");
    Serial.println("  vol VALUE         app volume, CC7");
    Serial.println("  rev VALUE         app reverb, CC91");
    Serial.println("  del VALUE         app delay, CC92");
    Serial.println("  pc PROGRAM        select app patch 0..41");
    Serial.println("  next              next patch (Program Change 127)");
    Serial.println("  prev              previous patch (Program Change 126)");
    Serial.println("  pb VALUE          pitch bend -8192..8191");
    Serial.println("  sus 0|1           sustain pedal off/on (CC64)");
    Serial.println("  pressure VALUE    channel pressure 0..127");
    Serial.println("  panic             stop all notes");
    Serial.println();
}

static void handleCommand(String line) {
    line.trim();
    if (line.length() == 0) return;

    int a = 0, b = 0, c = 0;

    if (line == "h" || line == "help") {
        printHelp();
    } else if (line == "s" || line == "status") {
        Serial.printf("[BLE] %s; advertising name: %s\n",
                      bleMidi.isConnected() ? "connected" : "not connected",
                      DEVICE_NAME);
    } else if (line == "t") {
        if (!bleMidi.isConnected()) {
            Serial.println("[DEMO] connect Piano MIDI Legend first");
            return;
        }
        if (demoRunning) {
            panic();
        } else {
            demoRunning = true;
            demoStep = 0;
            nextDemoEventMs = 0;
            Serial.println("[DEMO] C-major test started");
        }
    } else if (sscanf(line.c_str(), "n %d %d %d", &a, &b, &c) == 3) {
        playTimedNote((uint8_t)clampInt(a, 0, 127),
                      (uint8_t)clampInt(b, 0, 127),
                      (uint32_t)clampInt(c, 20, 10000));
    } else if (sscanf(line.c_str(), "cc %d %d", &a, &b) == 2) {
        bool ok = midiHandler.sendControlChange(MIDI_CHANNEL,
                                                (uint8_t)clampInt(a, 0, 127),
                                                (uint8_t)clampInt(b, 0, 127));
        Serial.printf("[SEND] CC%u=%u %s\n", clampInt(a, 0, 127),
                      clampInt(b, 0, 127), ok ? "OK" : "FAILED");
    } else if (sscanf(line.c_str(), "vol %d", &a) == 1) {
        midiHandler.sendControlChange(MIDI_CHANNEL, 7, (uint8_t)clampInt(a, 0, 127));
    } else if (sscanf(line.c_str(), "rev %d", &a) == 1) {
        midiHandler.sendControlChange(MIDI_CHANNEL, 91, (uint8_t)clampInt(a, 0, 127));
    } else if (sscanf(line.c_str(), "del %d", &a) == 1) {
        midiHandler.sendControlChange(MIDI_CHANNEL, 92, (uint8_t)clampInt(a, 0, 127));
    } else if (sscanf(line.c_str(), "pc %d", &a) == 1) {
        midiHandler.sendProgramChange(MIDI_CHANNEL, (uint8_t)clampInt(a, 0, 41));
    } else if (line == "next") {
        midiHandler.sendProgramChange(MIDI_CHANNEL, 127);
    } else if (line == "prev") {
        midiHandler.sendProgramChange(MIDI_CHANNEL, 126);
    } else if (sscanf(line.c_str(), "pb %d", &a) == 1) {
        midiHandler.sendPitchBend(MIDI_CHANNEL, clampInt(a, -8192, 8191));
    } else if (sscanf(line.c_str(), "sus %d", &a) == 1) {
        midiHandler.sendControlChange(MIDI_CHANNEL, 64, a ? 127 : 0);
    } else if (sscanf(line.c_str(), "pressure %d", &a) == 1) {
        // MIDIHandler has no sendChannelPressure helper yet, so send raw MIDI.
        sendRaw2((uint8_t)(0xD0 | (MIDI_CHANNEL - 1)),
                 (uint8_t)clampInt(a, 0, 127));
    } else if (line == "panic") {
        panic();
    } else {
        Serial.println("[CMD] unknown command; enter h for help");
    }
}

static void serviceSerial() {
    static String line;
    while (Serial.available()) {
        char ch = (char)Serial.read();
        if (ch == '\r') continue;
        if (ch == '\n') {
            handleCommand(line);
            line = "";
        } else if (line.length() < 80) {
            line += ch;
        }
    }
}

static void serviceTimedNote(uint32_t now) {
    if (timedNote.active && (int32_t)(now - timedNote.offAtMs) >= 0) {
        noteOffNow();
    }
}

static void serviceDemo(uint32_t now) {
    if (!demoRunning || !bleMidi.isConnected()) return;
    if (nextDemoEventMs != 0 && (int32_t)(now - nextDemoEventMs) < 0) return;

    if (timedNote.active) {
        noteOffNow();
        nextDemoEventMs = now + 120;
        return;
    }

    if (demoStep >= sizeof(DEMO_NOTES)) {
        demoRunning = false;
        Serial.println("[DEMO] complete");
        return;
    }

    playTimedNote(DEMO_NOTES[demoStep++], 100, 330);
    nextDemoEventMs = now + 450;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    // Register before begin() so connection callbacks are already installed.
    midiHandler.addTransport(&bleMidi);
    bleMidi.begin(DEVICE_NAME);

    MIDIHandlerConfig config;
    config.maxEvents = 32;
    midiHandler.begin(config);

    Serial.println("=== Piano MIDI Legend BLE Lab ===");
    Serial.printf("Advertising standard BLE-MIDI as '%s'\n", DEVICE_NAME);
    Serial.println("Connect from Piano MIDI Legend, then enter t.");
    printHelp();
}

void loop() {
    midiHandler.task();
    serviceSerial();

    const bool connected = bleMidi.isConnected();
    if (connected != wasConnected) {
        wasConnected = connected;
        Serial.println(connected ? "[BLE] connected" : "[BLE] disconnected; advertising restarted");
        if (!connected) panic();
    }

    uint32_t now = millis();
    serviceTimedNote(now);
    serviceDemo(now);
    delay(1);
}
