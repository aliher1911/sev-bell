/**
 * Startup: 
 *   position servo to the center.
 * Loop
 *   listen for char '1' on serial, and restart seq every time it is received
 *   sequence is defined as (pos, delay) tuples
 */
#include <Servo.h>

const int SERIAL_SPEED = 9600;
const int SERVO_PIN = 9;

const int sweepBackDelay = 300;
const int low = 30;
const int high = 135;
const int mid = (high + low) / 2;

Servo myservo;

struct Stroke {
  int pos;
  long delay;
};

const int maxStrokes = 8;
const int kEndOfList = -1;

Stroke sequence[maxStrokes] = {
  {low, 150},
  {high, 300},
  {mid, 150},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
};

void setup() {
  // servo pin 9
  myservo.attach(SERVO_PIN);
  myservo.write(mid);
  // led pin
  pinMode(LED_BUILTIN, OUTPUT);
  // commands from serial
  Serial.begin(SERIAL_SPEED);
}

long nextUpdate = 0;
size_t next = maxStrokes;
bool defer = false;
void startServo() {
  if (next != maxStrokes) {
    // prev run still active
    defer = true;
  } else {
    next = 0;
    // set update 1 ms in the past to trigger first run
    nextUpdate = millis() - 1;
    refreshServo();
  }
}

void refreshServo() {
  if (next != maxStrokes && nextUpdate < millis()) {
    if (sequence[next].pos == kEndOfList) {
      // we reached end of list
      next = maxStrokes;
      if (defer) {
        // bell was enqueued while ringing, restart
        defer = false;
        startServo(); 
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    } else {
      nextUpdate += sequence[next].delay;
      myservo.write(sequence[next].pos);
      next++;
      // signal software people here
      digitalWrite(LED_BUILTIN, (next + 1) % 2);
    }
  }
}

int inByte;
void processSerial() {
  inByte = Serial.read();
  // we can have more sophisticated proto here with line reads
  // for changing sequence
  if (inByte == '1') {
    startServo();
  }
}

void loop() {
  if (Serial.available() > 0) {
    processSerial();
  }
  refreshServo();
}
