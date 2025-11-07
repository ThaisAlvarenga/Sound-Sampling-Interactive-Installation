/***********************************************************************
  UNO R4 WiFi + SparkFun Qwiic RFID  → raw MIDI bytes over USB Serial
  - Qwiic RFID on Wire1
  - Sends Note On/Off as 3 raw bytes on Serial (USB CDC)
  - No DIN/TRS, no Serial1, no text prints (to avoid garbling the stream)
************************************************************************/

#include <Wire.h>
#include "SparkFun_Qwiic_Rfid.h"

// --- RFID (Qwiic on UNO R4 WiFi = Wire1) ---
#define RFID_ADDR 0x7D
Qwiic_Rfid rfid(RFID_ADDR);

// --- Single tag to test ---
const String TARGET_TAG = "07718512016836";   // <-- put your tag ID here

// --- MIDI message to send ---
const uint8_t MIDI_CH   = 0;   // 0 = Channel 1
const uint8_t MIDI_NOTE = 60;  // C4
const uint8_t MIDI_VEL  = 100;

const unsigned long RETRIGGER_MS = 600;  // block re-triggers within this window
const unsigned long GATE_MS      = 120;  // NoteOn → NoteOff delay

bool noteOnActive = false;
unsigned long lastTrigMs = 0;

// ---- send one raw byte on USB serial (CDC) ----
inline void midiWrite(uint8_t b) { Serial.write(b); }

inline void midiNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
  midiWrite(0x90 | (ch & 0x0F));
  midiWrite(note & 0x7F);
  midiWrite(vel  & 0x7F);
}

inline void midiNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
  midiWrite(0x80 | (ch & 0x0F));
  midiWrite(note & 0x7F);
  midiWrite(vel  & 0x7F);
}

void setup() {
  // Use a high baud so bridges don’t choke; also keeps latency low.
  Serial.begin(115200);
  while (!Serial) {}  // ensure USB CDC is up

  Wire1.begin();
  if (!rfid.begin(Wire1)) {
    // Do not print text to Serial (would corrupt MIDI stream).
    // If you must debug, temporarily enable prints and comment out midi sends.
    while (1) { delay(500); }
  }


  pinMode(LED_BUILTIN, OUTPUT);  // LED as visual feedback (no serial text)
}

void loop() {
  // Get latest tag: "" if none, "000000" if undecodable/unsupported
  String tag = rfid.getTag();

  if (tag.length() > 0 && tag != "000000") {
    if (tag == TARGET_TAG) {
      unsigned long now = millis();
      if (now - lastTrigMs > RETRIGGER_MS) {
        midiNoteOn(MIDI_CH, MIDI_NOTE, MIDI_VEL);   // send raw bytes on Serial
        noteOnActive = true;
        lastTrigMs = now;
        digitalWrite(LED_BUILTIN, HIGH);            // visual confirmation only
        // Optional: rfid.clearTags();
      }
    } else {
      // Unknown/other valid tag seen — ignored to keep serial pure MIDI.
      // If you need debugging, add prints temporarily (but they will mix with MIDI).
    }
  }

  if (noteOnActive && (millis() - lastTrigMs) > GATE_MS) {
    midiNoteOff(MIDI_CH, MIDI_NOTE, 0);
    noteOnActive = false;
    digitalWrite(LED_BUILTIN, LOW);
  }

  delay(40); // gentle polling
}
