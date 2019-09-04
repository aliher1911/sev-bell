/**
 * Startup: 
 *   position servo to the center.
 * Loop
 *   listen for char '1' on serial, and restart seq every time it is received
 *   sequence is defined as (pos, delay) tuples
 */
#include <Servo.h>

#define DEBUG_LOG

#ifdef DEBUG_LOG
#define DEBUG_MSG(v) Serial.println(v)
#define DEBUG_MSG1(v) Serial.print(v)
#else
#define DEBUG_MSG(v)
#define DEBUG_MSG1(v)
#endif

const int SERIAL_SPEED = 9600;
const int SERVO_PIN = 5;

const int low = 30;
const int high = 135;
const int mid = (high + low) / 2;

Servo myservo;

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
  // servo pin 9
  myservo.attach(SERVO_PIN);
  myservo.write(mid);
  // led pin
  pinMode(LED_BUILTIN, OUTPUT);
  // commands from serial
  Serial.begin(SERIAL_SPEED);
  DEBUG_MSG(F("Start!"));
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
    DEBUG_MSG(F("Defer action"));
    // prev run still active
    defer = true;
  } else {
    DEBUG_MSG(F("Start action"));
    next = 0;
    // set update 1 ms in the past to trigger first run
    nextUpdate = millis() - 1;
    refreshServo();
  }
}

void refreshServo() {
  if (next != maxStrokes && nextUpdate < millis()) {
    if (sequence[next].pos == kEndOfList) {
      DEBUG_MSG(F("Finished action"));
      // we reached end of list
      next = maxStrokes;
      if (defer) {
        DEBUG_MSG(F("Start deferred sequence"));
        // bell was enqueued while ringing, restart
        defer = false;
        startServo(); 
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    } else {
      DEBUG_MSG1(F("Next step "));
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
  DEBUG_MSG(F("Stop current action"));
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
      DEBUG_MSG(F("Reset frame"));
      pos = 0;
      break;
    case '\n':
    case '\r':
      DEBUG_MSG(F("Newline"));
      if (pos == max_buffer) {
        // buffer reached limit so it was truncated so we should ignore
        DEBUG_MSG(F("Buffer overflow"));
        pos = 0;
      }
      if (pos != 0) {
        switch(buffer[0]) {
          case '0':
            stopAction();
            break;
          case '1':
            if (pos==1) {
              startServo();
            } else {
              DEBUG_MSG(F("Start servo cmd has unrecognised payload"));
            }
            break;
          case '2':
            changeServo();
            break;
        }
        // reset command buffer
        pos=0;
      }
      break;
    default:
      DEBUG_MSG(F("Newchar"));
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
    DEBUG_MSG(F("Wrong message size"));
    return;
  }
  if (buffer[colon] != ':') {
    DEBUG_MSG(F("Failed to parse delimiters"));
    return;
  }
  int index;
  int angle;
  long delay;
  auto next = buffer + colon;
  auto end = buffer + pos;
  next = parseTo(next + 1, end, index);
  if (next == end || *next != ',') {
    DEBUG_MSG(F("Failed to parse: no first comma"));
    return;
  }
  next = parseTo(next + 1, end, angle);
  if (next == end || *next != ',') {
    DEBUG_MSG(F("Failed to parse: no second comma"));
    return;
  }
  next = parseTo(next + 1, end, delay);
  if (next != end) {
    DEBUG_MSG(F("Failed to parse: extra chars"));
    return;
  }
  // -1 because we reserve last entry as terminator
  if (index >= maxStrokes-1) {
    DEBUG_MSG(F("Validation: Index out of range"));
    return;
  }
  if (angle > 180) {
    DEBUG_MSG(F("Validation: Angle out of range"));
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
