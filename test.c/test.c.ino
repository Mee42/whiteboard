#include <limits.h>

void setup() {
  pinMode(2, OUTPUT); // digital out
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  Serial.begin(9600);
  while(!Serial); // wait until we connect to 
  zero();
}

const int sets[8][4] = { 
        {0, 0, 0, 1},
        {0, 0, 1, 1},
        {0, 0, 1, 0},
        {0, 1, 1, 0},
        {0, 1, 0, 0},
        {1, 1, 0, 0},
        {1, 0, 0, 0},
        {1, 0, 0, 1}
};

/*

m
r 50,50
r -50,50
r -50,-50
r 50,-50
r 50,50
m

m
r -100, -70
r -85, 0
r -60, -70
r -45, 0
r -30, -70

r -20, -70
r 0, -50
r -20, -30
r -20, -70
r -20, 0

r 10, 0
r 35, 0
r 60, 0
r 35, 0
r 35, -70

r 10, -70
r 35, -70
r 60, -70
f 0, -10

*/


// where we want to be
// where 0 = the marker is touching the corner
// and 512*8(1 rot) = the motor is is (2)(pi)(r) units away from the corner
long setpointA = 0; // this is in ticks from the A corner to the marker
long setpointB = 0; // this is in ticks from the B corner to the marker

// where we are. Same unit as the above
long positionA = 0;
long positionB = 0;

const long xMax = 492;    // this is the width of the whiteboard in mm, 19 and 3/8in
const long yMax = 390;    // this is the height of the whtiteboard in mm

long string_length_at_center; // in mm
long lastX, lastY;

void zero() {
  xy_to_absolute_string_length(xMax/2, yMax/2, &string_length_at_center, &string_length_at_center);
  setpointA = 0;
  setpointB = 0;
  positionA = 0;
  positionB = 0;
  lastX = xMax/2;
  lastY = yMax/2;
  Serial.println("Zeroing");
}

// returns motor tick delta
long string_len_to_ticks(long mm) {
  return (mm * 1000) / 30;
}

void xy_to_absolute_string_length(long xTarget, long yTarget, long *out_a_mm, long *out_b_mm) {
  long bLength = (long)sqrt(xTarget * xTarget + yTarget * yTarget); // mm from B corner
  long aLength = (long)sqrt((xMax - xTarget)*(xMax - xTarget) + yTarget * yTarget); // mm from A corner
  *out_a_mm = aLength;
  *out_b_mm = bLength;
}

void xy_to_motor_setpoints(long xTarget, long yTarget) {
  long a_mm, b_mm;
  xy_to_absolute_string_length(xTarget, yTarget, &a_mm, &b_mm);
  a_mm -= string_length_at_center;
  b_mm -= string_length_at_center;
  setpointA = -string_len_to_ticks(a_mm);
  setpointB = -string_len_to_ticks(b_mm);
}



void buffer() {

}

void loop() {
  if(Serial.available() > 0){
    char nextByte = Serial.read();
    if(nextByte == 'a') setpointA += 100;
    if(nextByte == 'A') setpointA -= 100;
    if(nextByte == 'b') setpointB += 100;
    if(nextByte == 'B') setpointB -= 100;
    if(nextByte == 'z') {
      zero();
    }

    if(nextByte == 'o') {
      Serial.print("setpointA: ");
      Serial.print(setpointA);
      Serial.print(" positionA:");
      Serial.print(positionA);

      Serial.print("    setpointB: ");
      Serial.print(setpointB);
      Serial.print(" positionB:");
      Serial.print(positionB);
      Serial.print(" lastX:");
      Serial.print(lastX);
      Serial.print(" lastY:");
      Serial.println(lastY);
    }
    if(nextByte == 'm'){
      xy_to_motor_setpoints(xMax/2, yMax/2);
      lastX = xMax / 2;
      lastY = yMax / 2; 
    }
    if(nextByte == 'f') {
      Serial.read(); // the space
      long x = Serial.parseInt();
      lastX += x;
      Serial.read(); // the ,
      lastY += Serial.parseInt();
      xy_to_motor_setpoints(lastX, lastY);
    }
    if(nextByte == 'r'){
      lastX = Serial.parseInt() + xMax/2;
      Serial.read(); // the ,
      lastY = Serial.parseInt() + yMax/2;
      xy_to_motor_setpoints(lastX, lastY);
    }
    if(nextByte == 's'){
      Serial.read(); // the space
      lastX = Serial.parseInt();
      Serial.read(); // the ,
      lastY = Serial.parseInt();
      xy_to_motor_setpoints(lastX, lastY);
    }
    if(nextByte == 's' || nextByte == 'f' || nextByte == 'm' || nextByte == 'r') {
      Serial.print("Going to ");
      Serial.print(lastX);
      Serial.print(", ");
      Serial.println(lastY);
    }
  }
  while((setpointA != positionA) || (setpointB != positionB)){
    if(setpointA != positionA) {
      if(setpointA > positionA) positionA++;
      else positionA--;
      const int* motorSetpoints = sets[7 - positionA & 7]; 
      for(int pin = 0; pin < 4; pin++){
        digitalWrite(6 + pin, motorSetpoints[pin] ? HIGH : LOW);
      }
    }
    if(setpointB != positionB) {
      if(setpointB > positionB) positionB++;
      else positionB--;
      const int* motorSetpoints = sets[7 - positionB & 7]; 
      for(int pin = 0; pin < 4; pin++){
        digitalWrite(2 + pin, motorSetpoints[pin] ? HIGH : LOW);
      }
    }
    delayMicroseconds(1500);
  }
}