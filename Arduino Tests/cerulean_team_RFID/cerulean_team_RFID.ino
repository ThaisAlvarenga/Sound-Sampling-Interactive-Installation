// UNO R4 WiFi + SparkFun Qwiic RFID (ID-12LA) --> MIDI over Serial bridge
// - Default: USB bridge mode (Hairless/loopMIDI or IAC). Serial @ 115200 (stable).
// - Optional: DIN/TRS hardware MIDI on Serial1 @ 31250 (set USE_USB_BRIDGE to 0).

#include <Wire.h>
#include "SparkFun_Qwiic_Rfid.h"

// ---------------- Config: select output transport ----------------
#define USE_USB_BRIDGE 1
// 1 = USB-Serial bridge (Hairless). Serial @ 115200.
// 0 = Hardware DIN/TRS MIDI on Serial1 @ 31250 (TX1 pin to DIN 5-pin w/ proper circuit)

// ---------------- RFID config ----------------
#define RFID_ADDR 0x7D          // I2C address seen on Wire1 scan
Qwiic_Rfid rfid(RFID_ADDR);

// Set the tag you want to trigger on (exact string from getTag())
const String TARGET_TAG = "07718512016836";

// ---------------- MIDI config ----------------
const uint8_t MIDI_CH   = 0;     // 0 = channel 1
const uint8_t MIDI_NOTE = 60;    // C4
const uint8_t MIDI_VEL  = 100;   // velocity
const unsigned long RETRIGGER_BLOCK_MS = 600;

unsigned long lastTriggerMs = 0;
bool noteIsOn = false;

// ---- MIDI send helpers (raw bytes) ----
inline void midiWriteByte(uint8_t b) {
#if USE_USB_BRIDGE
  Serial.write(b);               // USB CDC -> Hairless
#else
  Serial1.write(b);              // DIN/TRS 31250 baud
#endif
}

inline void midiNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
  midiWriteByte(0x90 | (ch & 0x0F));
  midiWriteByte(note & 0x7F);
  midiWriteByte(vel  & 0x7F);
}

inline void midiNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
  midiWriteByte(0x80 | (ch & 0x0F));
  midiWriteByte(note & 0x7F);
  midiWriteByte(vel  & 0x7F);
}

void setup() {
#if USE_USB_BRIDGE
  Serial.begin(115200);          // Hairless works  at 115200
  while (!Serial) {}
  Serial.println("USB-Serial MIDI bridge mode (115200).");
  Serial.println("Use Hairless + IAC (macOS) / loopMIDI (Windows).");
#else
  Serial.begin(115200);          // debug messages
  while (!Serial) {}
  Serial1.begin(31250);          // DIN/TRS hardware MIDI baud
  Serial.println("Hardware DIN/TRS MIDI on Serial1 @ 31250.");
#endif

  // Qwiic on UNO R4 WiFi = Wire1
  Wire1.begin();
  if (!rfid.begin(Wire1)) {
    Serial.println("ERROR: Qwiic RFID not found on Wire1 @ 0x7D/0x7C.");
    while (1) { delay(500); }
  }
  Serial.println("Qwiic RFID ready. Present a 125 kHz EM4100/TK4100 tag 1–2 cm over the can...");
}

void loop() {
  // Poll RFID ("" if none; "000000" if undecodable/non-EM4100 format)
  String tag = rfid.getTag();

  if (tag.length() > 0 && tag != "000000") {
    if (tag == TARGET_TAG) {
      unsigned long now = millis();
      if ((now - lastTriggerMs) > RETRIGGER_BLOCK_MS) {
        midiNoteOn(MIDI_CH, MIDI_NOTE, MIDI_VEL);
        noteIsOn = true;
        lastTriggerMs = now;
#if USE_USB_BRIDGE
        Serial.print("NoteOn via USB bridge for tag ");
        Serial.println(tag);
#else
        Serial.print("NoteOn via Serial1 (DIN/TRS) for tag ");
        Serial.println(tag);
#endif
      }
    } else {
      // Uncomment for debugging other tags:
      // Serial.print("Saw other tag: "); Serial.println(tag);
    }
  }

  // Short gate, then NoteOff
  if (noteIsOn && (millis() - lastTriggerMs) > 120) {
    midiNoteOff(MIDI_CH, MIDI_NOTE, 0);
    noteIsOn = false;
#if USE_USB_BRIDGE
    Serial.println("NoteOff (USB bridge)");
#else
    Serial.println("NoteOff (Serial1 DIN/TRS)");
#endif
  }

  delay(40); // gentle polling
}
