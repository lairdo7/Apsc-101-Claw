/*
  Program to deal with servo control on claw. Closes if sensor detects 
  given range for > 3 seconds, and re opens when it detects another range transition. 
*/


#include <Servo.h>

// Pin setup
const int SERVO_PIN = A3;
const int TRIG_PIN  = 9;
const int ECHO_PIN  = 10;

// Servo Angles
const int OPEN_DEG  = 5;     // open / idle
const int CLOSE_DEG = 110;   // close / grab

// Triggering Window
const float MIN_CM = 5.0;
const float MAX_CM = 30.0;

// Time buffer
const unsigned long HOLD_TIME_MS = 3000;

// Sampling period
const unsigned long SAMPLE_PERIOD_MS = 50;

Servo claw;

// State + timing
bool clawClosed = false;         
unsigned long inRangeStartMs = 0;  

// Distance Check
float readDistanceCm() {
  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo with timeout so we don't hang
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return 9999.0;
  return duration / 58.0;                     
}

bool inTriggerWindow(float cm) {
  return (cm >= MIN_CM && cm <= MAX_CM);
}

void applyClawState() {
  claw.write(clawClosed ? CLOSE_DEG : OPEN_DEG);
}

void toggleClaw() {
  clawClosed = !clawClosed;
  applyClawState();
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  claw.attach(SERVO_PIN);
  clawClosed = false;          // start open
  applyClawState();

  Serial.begin(9600);          // optional debugging
}

void loop() {
  static unsigned long lastSampleMs = 0;
  unsigned long now = millis();

  if (now - lastSampleMs < SAMPLE_PERIOD_MS) return;
  lastSampleMs = now;

  float cm = readDistanceCm();
  bool inRange = inTriggerWindow(cm);

  //need something to help let me know if its bugging out
  Serial.print("cm: "); Serial.print(cm);
  Serial.print(" inRange: "); Serial.print(inRange);
  Serial.print(" clawClosed: "); Serial.println(clawClosed);

  if (inRange) {

    if (inRangeStartMs == 0) inRangeStartMs = now;

    if (now - inRangeStartMs >= HOLD_TIME_MS) {
      toggleClaw();
      inRangeStartMs = 0;

      while (true) {
        float cm2 = readDistanceCm();
        if (!inTriggerWindow(cm2)) break;
        delay(30);
      }
    }
  } else {
    inRangeStartMs = 0;
  }
}