/***********************************************************************
  ONE–TAG TEST: UNO R4 WiFi + SparkFun Qwiic RFID → MIDI on Serial1
************************************************************************/

#include <Wire.h>
#include "SparkFun_Qwiic_Rfid.h"

// --- RFID (Qwiic on UNO R4 = Wire1) ---
#define RFID_ADDR 0x7D
Qwiic_Rfid rfid(RFID_ADDR);

// --- Single tag under test ---
const String TARGET_TAG = "07718512016836"; // <-- your tag

// --- MIDI (hardware DIN/TRS on Serial1 @ 31250) ---
const uint8_t MIDI_CH  = 0;   // 0 = Channel 1
const uint8_t MIDI_NOTE= 60;  // C4
const uint8_t MIDI_VEL = 100;
const unsigned long RETRIGGER_MS = 600; // ignore repeats within this window
const unsigned long GATE_MS      = 120; // NoteOn->NoteOff delay

bool noteOnActive = false;
unsigned long lastTrigMs = 0;

inline void midiWrite(uint8_t b)            { Serial1.write(b); }
inline void midiNoteOn(uint8_t ch,uint8_t n,uint8_t v){
  midiWrite(0x90 | (ch & 0x0F)); midiWrite(n & 0x7F); midiWrite(v & 0x7F);
}
inline void midiNoteOff(uint8_t ch,uint8_t n,uint8_t v){
  midiWrite(0x80 | (ch & 0x0F)); midiWrite(n & 0x7F); midiWrite(v & 0x7F);
}

void setup() {
  Serial.begin(115200); while(!Serial){}          // debug only
  Serial1.begin(31250);                           // hardware MIDI

  Wire1.begin();
  if (!rfid.begin(Wire1)) {
    Serial.println("ERROR: Qwiic RFID not found on Wire1 (0x7D/0x7C).");
    while (1) { delay(500); }
  }

//  rfid.setBeep(false); // silence the reader’s buzzer
  Serial.println("ONE–TAG TEST ready. Present the target EM4100 tag...");
}

void loop() {
  String tag = rfid.getTag(); // "" if none; "000000" if invalid/unsupported

  if (tag.length() > 0 && tag != "000000") {
    if (tag == TARGET_TAG) {
      unsigned long now = millis();
      if (now - lastTrigMs > RETRIGGER_MS) {
        midiNoteOn(MIDI_CH, MIDI_NOTE, MIDI_VEL);
        noteOnActive = true;
        lastTrigMs = now;
        Serial.println("[OK] Target tag detected → NoteOn");
      }
      // Optional: clear queue to avoid repeated returns of same scan
      // rfid.clearTags();
    } else {
      Serial.print("[INFO] Other valid tag seen: "); Serial.println(tag);
    }
  }

  if (noteOnActive && (millis() - lastTrigMs) > GATE_MS) {
    midiNoteOff(MIDI_CH, MIDI_NOTE, 0);
    noteOnActive = false;
    Serial.println("[OK] NoteOff");
  }

  delay(40); // gentle polling
}
