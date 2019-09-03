/**
 * Startup: 
 *   position servo to the center.
 * Loop
 *   listen for char '1' on serial, and restart seq every time it is received
 *   sequence is defined as (pos, delay) tuples
 */
#include <Servo.h>

#define DEBUG_MSG(v) Serial.print(v)

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
    DEBUG_MSG("Defer action");
    // prev run still active
    defer = true;
  } else {
    DEBUG_MSG("Start action");
    next = 0;
    // set update 1 ms in the past to trigger first run
    nextUpdate = millis() - 1;
    refreshServo();
  }
}

void refreshServo() {
  if (next != maxStrokes && nextUpdate < millis()) {
    if (sequence[next].pos == kEndOfList) {
      DEBUG_MSG("Finished action");
      // we reached end of list
      next = maxStrokes;
      if (defer) {
        DEBUG_MSG("Start next deferred");
        // bell was enqueued while ringing, restart
        defer = false;
        startServo(); 
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
    } else {
      DEBUG_MSG("Next step");
      nextUpdate += sequence[next].delay;
      myservo.write(sequence[next].pos);
      next++;
      // signal software people here
      digitalWrite(LED_BUILTIN, (next + 1) % 2);
    }
  }
}

const int max_buffer = 13;
char buffer[max_buffer];
int pos = 0;
void processSerial() {
  int inByte = Serial.read();
  if (inByte == '\n') {
    if (pos != 0) {
      switch(buffer[0]) {
        case 0:
          break;
        case 1:
          if (pos==1) {
            startServo();
          }
          break;
        case 2:          
          changeServo();
          break;
      }
      // reset command buffer
      pos=0;
    }
  } else {
    if (pos >= max_buffer) {
      // this is error, we should start from the beginning
      pos = 0;
    } else {
      buffer[pos++] = inByte;
    }
  }
}

// max message size:
// 0123456789012
// 2:0,000,00000
// 2:<index>,<pos>,<delay>
// index - [0, 7] index of step to change
// pos - [0, 180] angle encoded as 3 digits always
// delay - [0, 60000] milliseconds (1 minute max) as 5 digits
const int colon = 1;
const int comma1 = 3;
const int comma2 = 7;
void changeServo() {
  if (pos != max_buffer) {
    return;
  }
  if (buffer[colon] != ':' 
      || buffer[comma1] != ','
      || buffer[comma2] != ',') {
    DEBUG_MSG("Failed to parse delimiters");
    return;
  }
  int index;
  int pos;
  long delay;
  if (!parseTo(buffer+colon+1, buffer+comma1, index)
      || !parseTo(buffer+comma1+1, buffer+comma2, pos)
      || !parseTo(buffer+comma2+1, buffer+max_buffer, delay)) {
    DEBUG_MSG("Failed to parse digits");
    return;
  }
  if (index > 7) {
    DEBUG_MSG("Index out of range");
    return;
  }
  if (pos > 180) {
    DEBUG_MSG("Position out of range");
    return;
  }
  sequence[index].pos = pos;
  sequence[index].delay = delay;
  
  DEBUG_MSG("New position");
  DEBUG_MSG(index);
  DEBUG_MSG(pos);
  DEBUG_MSG(delay);
}

template <typename T>
bool parseTo(char* begin, char* end, T& val) {
  long value = 0;
  while(begin != end) {
    value *= 10;
    auto val = *begin;
    if (val < '0' || val > '9') {
      return false;
    }
    value += (val - '0');
  }
  val = value;
  return true;
}

void loop() {
  if (Serial.available() > 0) {
    processSerial();
  }
  refreshServo();
}
