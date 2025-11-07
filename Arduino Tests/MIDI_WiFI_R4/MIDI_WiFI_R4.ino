#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "SparkFun_Qwiic_Rfid.h"

// ---------- WiFi ----------
const char* WIFI_SSID = "123";
const char* WIFI_PASS = "thais242";

// Where to send the MIDI packets (your computer's IP) and port
IPAddress DEST_IP(172, 20, 10, 3); // <-- change to your laptop/DAW machine IP
const uint16_t DEST_PORT = 5006;     // any UDP port you prefer

WiFiUDP udp;

// ---------- RFID (Qwiic on UNO R4 WiFi = Wire1) ----------
#define RFID_ADDR 0x7D
Qwiic_Rfid rfid(RFID_ADDR);

// The tag string returned by getTag() to trigger on
const String TARGET_TAG = "07718512016836";

// ---------- MIDI ----------
const uint8_t MIDI_CH   = 0;   // 0 = channel 1
const uint8_t MIDI_NOTE = 60;  // C4
const uint8_t MIDI_VEL  = 100;

const unsigned long RETRIGGER_BLOCK_MS = 600;
unsigned long lastTrigMs = 0;
bool noteIsOn = false;

// If you prefer sending as OSC for easy receiving in SuperCollider/Max, set to 1.
// OSC payload: /midi, i i i  (status, data1, data2)
#define SEND_AS_OSC 0

// ---- helpers ----
void sendMidiUDP(uint8_t status, uint8_t d1, uint8_t d2) {
  udp.beginPacket(DEST_IP, DEST_PORT);
#if SEND_AS_OSC
  // Minimal OSC encoder for /midi with 3 int32 args
  // OSC address (null padded to 4 bytes)
  udp.write((const uint8_t*)"/midi", 5);
  // pad to 8 bytes total ("/midi" + 3 zeros)
  udp.write((uint8_t)0); udp.write((uint8_t)0); udp.write((uint8_t)0);

  // Type tag string ",iii" (comma + 3 ints), pad to 8 bytes
  udp.write((const uint8_t*)",iii", 4);
  udp.write((uint8_t)0); udp.write((uint8_t)0); udp.write((uint8_t)0); udp.write((uint8_t)0);

  // Write 3 big-endian int32 values
  auto w32 = [&](uint32_t v){
    udp.write((uint8_t)((v >> 24) & 0xFF));
    udp.write((uint8_t)((v >> 16) & 0xFF));
    udp.write((uint8_t)((v >> 8) & 0xFF));
    udp.write((uint8_t)(v & 0xFF));
  };
  w32(status); w32(d1); w32(d2);
#else
  // Raw 3-byte MIDI payload in UDP packet
  udp.write(status);
  udp.write(d1);
  udp.write(d2);
#endif
  udp.endPacket();
}

inline void sendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
  sendMidiUDP(0x90 | (ch & 0x0F), note & 0x7F, vel & 0x7F);
}

inline void sendNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
  sendMidiUDP(0x80 | (ch & 0x0F), note & 0x7F, vel & 0x7F);
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) break; // 15s timeout
    delay(200);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Debug over USB serial (keep prints minimal if Hairless is also in use elsewhere)
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("Connecting WiFi...");
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK. IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed. Check SSID/password and try again.");
  }

  udp.begin(DEST_PORT); // local port for UDP (can be any; here same as dest)

  // Qwiic on UNO R4 WiFi = Wire1
  Wire1.begin();
  if (!rfid.begin(Wire1)) {
    Serial.println("ERROR: Qwiic RFID not found on Wire1 (0x7D/0x7C).");
    while (1) { delay(500); }
  }
  Serial.println("RFID ready. Present EM4100/TK4100 125 kHz tag...");
}

void loop() {
  String tag = rfid.getTag();                  // "" if none; "000000" if undecodable
  if (tag.length() > 0 && tag != "000000") {
    if (tag == TARGET_TAG) {
      unsigned long now = millis();
      if ((now - lastTrigMs) > RETRIGGER_BLOCK_MS) {
        sendNoteOn(MIDI_CH, MIDI_NOTE, MIDI_VEL);
        digitalWrite(LED_BUILTIN, HIGH);
        noteIsOn = true;
        lastTrigMs = now;
        Serial.println("WiFi MIDI: NoteOn");
      }
    } else {
      // Uncomment to log other tags:
      // Serial.print("Saw tag: "); Serial.println(tag);
    }
  }

  if (noteIsOn && (millis() - lastTrigMs) > 120) {
    sendNoteOff(MIDI_CH, MIDI_NOTE, 0);
    digitalWrite(LED_BUILTIN, LOW);
    noteIsOn = false;
    Serial.println("WiFi MIDI: NoteOff");
  }

  delay(40);
}
