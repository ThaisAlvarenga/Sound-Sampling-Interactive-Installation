/***********************************************************************
   Project: Sampler Interactive Installation
   Group:   Cerulean Team
   Author:  Thais Alvarenga
   Date:    Nov 9

   Description:
   --------------------------------------------------------------------
   
   This project sketch uses the SparkFun Qwiic RFID Reader (ID-12LA module) 
   connected to the Arduino UNO R4 WiFi via the Qwiic I²C port (Wire1).
   It reads 125 kHz EM4100-style RFID tags. When a specific RFID tag is 
   detected, the Arduino sends a serial message at the MIDI hardware baud rate. 

   The host program (Unity + Ableton custom app) listens to the serial port and
   plays the corresponding audio track.

   One-port mode:
   - All messages go to USB Serial @ 115200.
   - Host listens for lines starting with [DATA] for song triggers.
   - Debug is everything starting with [DEBUG].
   
*/

//  ---- Libraries

#include <Wire.h>
#include "SparkFun_Qwiic_Rfid.h"

//  ---- RFID reader
#define RFID_ADDR 0x7D  // I2C address seen on Wire1 scan
Qwiic_Rfid myRfid(RFID_ADDR); // Create RFID instance on that address

/* ---- Tag → index mapping ----
 *  
 *  Tag: 07718512016836   SONG: 
 *  Tag: 07718512015717   SONG:
 *  Tag: 07718512129144   SONG:
*/

struct recordTags {
  const char* ID;
  const char* songName;
 };

recordTags vinylTags[] = {
  {"07718512016836", "Drake"},
  {"07718512015717", "Kendrik"},
  {"07718512129144", "Kanye"}
  
};

const int NUM_TAGS = sizeof(vinylTags) / sizeof(vinylTags[0]);

// ---- Trigger Debounce / Cooldown 

bool canTrigger = true;
unsigned long lastTriggerTime = 0;
const unsigned long RETRIGGER_TIME = 600; // Delay window (ms) between triggers

// ------------- Helpers (debug/data) -------------
#define DEBUG_ON        1              // set to 0 to silence debug
#if DEBUG_ON
  #define DBG(msg)   do { Serial.print(F("[DEBUG] ")); Serial.println(msg); } while (0)
#else
  #define DBG(msg)   do {} while (0)
#endif

// Send a clean data line for the host app to parse
void sendData(const char* songName) {
  Serial.print(F("[DATA] "));
  Serial.println(songName);
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  DBG(F("USB Serial online @ 115200"));

  // set up RFID Qwiic bus
  Wire1.begin(); // UNO R4 WiFi Qwiic bus
  
  // send message if RFID not found
  if (!myRfid.begin(Wire1)) 
  {
    DBG(F("Could not communicate with Qwiic RFID on Wire1!"));
    while (1) { delay(500); } // Freeze execution
  }
  // RFID confirmation message
  DBG(F("Ready. Present a 125 kHz EM4100/TK4100 tag 1–2 cm over the can..."));

}

void loop() {
  // 1. READ RFID TAG
  String myTag = myRfid.getTag();

  // if string is not empty AND not the reader's "ghost" value
  if(myTag.length() > 0  && myTag !="000000")
  {
    unsigned long now = millis();
    DBG(String("Tag detected: ")+ myTag);

    // Allow trigger only if cooldown time has passed
    if (canTrigger && (now - lastTriggerTime > RETRIGGER_TIME))
    {
      // Loop over our mapping table to see if scanned tag matches one of ours
      for (int i = 0; i < NUM_TAGS; i++)
      {
        // IF RECOGNIZABLE RFID TAG IS READ
        if (myTag == vinylTags[i].ID)
        {
          DBG(F("Tag matched. Sending song name to host."));

          
          // 2. SEND SERIAL MESSAGE WITH TAG'S CORRESPONDING SONG
          sendData(vinylTags[i].songName); //host listens for this

          DBG(String("Song sent: ")+ vinylTags[i].songName);
          
          // Visual feedback (LED on)
          // digitalWrite(LED_BUILTIN, HIGH);

          // Update state to block retriggering until timeout passes
          lastTriggerTime = now;
          canTrigger = false;         // disarm until re-armed below
          break;
        }
      }
    }
  }

  // Reset trigger permission once cooldown time has passed
  if (millis() - lastTriggerTime > RETRIGGER_TIME) 
  {
    canTrigger = true;
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED when re-armed
  }

  // Small delay stabilizes RFID polling rate
  delay(40); 

  // ---- CODE LOGIC:

  // IF RECOGNIZABLE RFID TAG IS READ
  
    // 2. SEND SERIAL MESSAGE WITH TAG'S CORRESPONDING SONG
    
    // 3. START RECORD PLAYER MOTOR

  // 4. IF RFID TAG IS REMOVED

    // 5. STOP RECORD PLAYER MOTOR

  // IF NO TAG IS DETECTED
  
    // DO NOTHING

  
}
