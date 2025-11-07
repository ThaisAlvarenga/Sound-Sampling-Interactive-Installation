/* ********************************************************************** 
 * DC_Motor
 *   Uses: one DC Motor
 *         one L293D Quadruple half H-Bridge driver IC
 *         one Breadboard Power Supply, MB-102
 *   
 * Inspired by Elegoo Lesson 29 
 *   www.elegoo.com     2016.12.12
 *   modified by Ricardo Moreno
 *
 * Description:
 *   This sketch illustrates controlling a DC Motor using an
 *   L293D Quadruple half H-Bridge driver IC.  Note that the "D" 
 *   indicates inductive transient suppression diodes are 
 *   integrated in the IC. It's recommended that a separate
 *   power supply is used.
 *   
 * Circuit:             L293D
 *                     _______
 *    Arduino 5---EN12-|     |-VCC1---5V
 *    Ardunio 4-----1A-|     |-4A
 *     DCMotor+-----1Y-|     |-4Y
 *          GND--------|     |--------GND
 *          GND--------|     |--------GND
 *     DCMotor- ----2Y-|     |-3Y     
 *    Arduino 3-----2A-|     |-3A
 *      4.5-36V---VCC2-|     |-EN34
 *                     -------
 * History:
 *  6/16/2019 v1.0 - Initial release
 *  7/01/2019 v1.1 - Additional comments, minor rev to void loop
 *                   
 ********************************************************************* */

/* ***************************************************
 *                Global Constants                   *
 *************************************************** */

/* DC motor speed with potentiometer + Serial debug
   L293D: EN12 -> D5 (PWM), 1A -> D4, 2A -> D3
   Pot wiper -> A0
*/

const int Enable12 = 5;   // PWM to L293D EN12 (pin 1)
const int Driver1A  = 4;  // L293D 1A (pin 2)
const int Driver2A  = 3;  // L293D 2A (pin 7)
const int POT       = A0; // potentiometer wiper

// Optional: tweak these if your pot doesn't swing to 0 or 1023
const int POT_MIN = 0;
const int POT_MAX = 1023;

// Optional: avoid buzzing/stall at very low PWM
const int PWM_FLOOR = 30;     // ~12% duty
const int STOP_THRESH = 8;    // pot counts under which we force full stop

void setup() {
  pinMode(Enable12, OUTPUT);
  pinMode(Driver1A, OUTPUT);
  pinMode(Driver2A, OUTPUT);

  // fixed direction (swap HIGH/LOW to reverse)
  digitalWrite(Driver1A, HIGH);
  digitalWrite(Driver2A, LOW);

  Serial.begin(115200);
  delay(200); // give USB a moment
  Serial.println("Pot->PWM debug started");
}

void loop() {
  int potRaw = analogRead(POT);                // 0..1023 on UNO R4
  // Constrain then map to PWM range
  potRaw = constrain(potRaw, POT_MIN, POT_MAX);
  int duty = map(potRaw, POT_MIN, POT_MAX, 0, 255);

  // Anti-stall: either fully stop, or ensure a small kick
  if (potRaw <= STOP_THRESH) {
    duty = 0;
  } else if (duty > 0 && duty < PWM_FLOOR) {
    duty = PWM_FLOOR;
  }

  analogWrite(Enable12, duty);

  // --- Serial debug output ---
  // Good for Serial Monitor:
  Serial.print("pot: ");  Serial.print(potRaw);
  Serial.print("\tPWM: "); Serial.print(duty);
  Serial.print("\t%: ");   Serial.println((duty * 100) / 255);

  // If you prefer Serial Plotter instead, comment the 4 lines above
  // and uncomment the line below (two series: pot and duty):
  // Serial.print("pot:"); Serial.print(potRaw); Serial.print(", PWM:"); Serial.println(duty);

  delay(50);
}

 
