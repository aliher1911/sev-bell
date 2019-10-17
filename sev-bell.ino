/**
 * Startup: 
 *   position servo to the center.
 * Loop
 *   listen for char '1' on serial, and restart seq every time it is received
 *   sequence is defined as (pos, delay) tuples
 */
#include <Servo.h>
#include <SoftwareSerial.h>

#define DEBUG_LOG

#ifdef DEBUG_LOG
#define DEBUG_MSG(v) debugSerial.println(v)
#define DEBUG_MSG1(v) debugSerial.print(v)
#else
#define DEBUG_MSG(v)
#define DEBUG_MSG1(v)
#endif

// control serial from device
const long SERIAL_SPEED = 9600;

// pin where servo is attached
const int SERVO_PIN = 5;
// ping controlling servo power
const int SERVO_POWER_PIN = 7;

// debug serial console where status messages are printed
const long DEBUG_SERIAL_SPEED = 115200;
const int SERIAL_TX = 3;
const int SERIAL_RX = 2;


const int low = 30;
const int high = 135;
const int mid = (high + low) / 2;

Servo myservo;
SoftwareSerial debugSerial(SERIAL_RX, SERIAL_TX);

struct Stroke {
  int pos;
  long delay;
};

const int maxStrokes = 17;
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
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0},
  {kEndOfList, 0}, // can't override that
};


void setup() {
  // turn servo power off until we init
  // it should also float while uninitialized
  pinMode(SERVO_POWER_PIN, OUTPUT);
  digitalWrite(SERVO_POWER_PIN, HIGH);
  // servo pin 9
  myservo.attach(SERVO_PIN);
  myservo.write(mid);
  // led pin
  pinMode(LED_BUILTIN, OUTPUT);
  // commands from serial
  Serial.begin(SERIAL_SPEED);
  debugSerial.begin(DEBUG_SERIAL_SPEED);
  DEBUG_MSG(F("Start!"));
  // enable servo power
  digitalWrite(SERVO_POWER_PIN, LOW);
}

// time in millis when next servo angle is applied
long nextUpdate = 0;
// index of next servo move
// 0 - maxStrokes, but maxStroke is never addressed
size_t next = maxStrokes;
// if true, next seq is requested while prev is in progress
bool defer = false;
void startServo() {
  if (next != maxStrokes) {
    DEBUG_MSG(F("ACTION: Defer"));
    // prev run still active
    defer = true;
  } else {
    DEBUG_MSG(F("ACTION: Start"));
    next = 0;
    // set update 1 ms in the past to trigger first run
    nextUpdate = millis() - 1;
    refreshServo();
  }
}

void refreshServo() {
  if (next != maxStrokes && nextUpdate < millis()) {
    if (sequence[next].pos == kEndOfList) {
      DEBUG_MSG(F("ACTION: Finished"));
      // we reached end of list
      next = maxStrokes;
      if (defer) {
        DEBUG_MSG(F("ACTION: Deferred found"));
        // bell was enqueued while ringing, restart
        defer = false;
        startServo(); 
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    } else {
      DEBUG_MSG1(F("ACTION: Next step"));
      DEBUG_MSG(next);
      nextUpdate += sequence[next].delay;
      myservo.write(sequence[next].pos);
      next++;
      // signal software people here
      digitalWrite(LED_BUILTIN, (next + 1) % 2);
    }
  }
}

void stopAction() {
  DEBUG_MSG(F("ACTION: Stop"));
  next = maxStrokes;
  defer = false;
  myservo.write(mid);
}

const int max_buffer = 25;
char buffer[max_buffer];
int pos = 0;
void processSerial() {
  int inByte = Serial.read();
  switch (inByte) {
    case 0:
      DEBUG_MSG(F("FRAME: Reset"));
      pos = 0;
      break;
    case '\n':
    case '\r':
      DEBUG_MSG(F("FRAME: Finished"));
      if (pos == max_buffer) {
        // buffer reached limit so it was truncated so we should ignore
        DEBUG_MSG(F("FRAME: Overflow"));
        pos = 0;
      }
      if (pos != 0) {
        switch(buffer[0]) {
          case '0':
            if (pos==1) {
              stopAction();
            } else {
              DEBUG_MSG(F("PARSER: Stop cmd has payload"));
            }
            break;
          case '1':
            if (pos==1) {
              startServo();
            } else {
              DEBUG_MSG(F("PARSER: Start cmd has payload"));
            }
            break;
          case '2':
            changeServo();
            break;
          default:
            DEBUG_MSG(F("PARSER: Unknown command"));
            break;
        }
        // reset command buffer
        pos=0;
      }
      break;
    default:
      // echo done with minimum overhead to avoid blocking normal calls
      DEBUG_MSG(inByte);
      if (pos < max_buffer) {
        buffer[pos++] = inByte;
      }
  }
}

// Message format
// 2:<index>,<angle>,<delay>
// index - [0, 15] index of step to change
// pos - [0, 180] angle
// delay - [0, 60000] milliseconds (1 minute max)
const int colon = 1;
void changeServo() {
  if (pos != max_buffer) {
    DEBUG_MSG(F("PARSER: Wrong message size"));
    return;
  }
  if (buffer[colon] != ':') {
    DEBUG_MSG(F("PARSER: No colon"));
    return;
  }
  int index;
  int angle;
  long delay;
  auto next = buffer + colon;
  auto end = buffer + pos;
  next = parseTo(next + 1, end, index);
  if (next == end || *next != ',') {
    DEBUG_MSG(F("PARSER: no first comma"));
    return;
  }
  next = parseTo(next + 1, end, angle);
  if (next == end || *next != ',') {
    DEBUG_MSG(F("PARSER: no second comma"));
    return;
  }
  next = parseTo(next + 1, end, delay);
  if (next != end) {
    DEBUG_MSG(F("PARSER: extra chars"));
    return;
  }
  // -1 because we reserve last entry as terminator
  if (index >= maxStrokes-1) {
    DEBUG_MSG(F("CHECK: Index out of range"));
    return;
  }
  if (angle > 180) {
    DEBUG_MSG(F("CHECK: Angle out of range"));
    return;
  }
  DEBUG_MSG1(F("New position: "));
  DEBUG_MSG1(index);
  DEBUG_MSG1(angle);
  DEBUG_MSG(delay);

  sequence[index].pos = angle;
  sequence[index].delay = delay;
}

// return first non digit entry or end of string
template <typename T> char* parseTo(char* begin, char* end, T& val) {
  long value = 0;
  while(begin != end) {
    auto val = *begin;
    if (val < '0' || val > '9') {
      // found first non digit
      break;
    }
    value *= 10;
    value += (val - '0');
    begin++;
  }
  val = value;
  return begin;
}

void loop() {
  if (Serial.available() > 0) {
    processSerial();
  }
  refreshServo();
}
