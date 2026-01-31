#include <Arduino.h>
#include <AccelStepper.h>

//////////////////////////
////// DEFINITIONS ///////
//////////////////////////
// Constants
uint32_t Z_RANGE = 10000; // The range between the tallest and lowest Z position
uint32_t Z_LOW_BOUND = -14000; // the lowest position Z axis goes (0 is at the top)
uint32_t Z_HIGH_BOUND = -2000; // the highest position Z goes

uint16_t Z_HEIGHT = 17500;

// Vars for speed stuff
int maxStepsPerSec = 3000; // max speed for all motors
int BPAcceleration = 200; // max acceleration for spinning build plat
int ZAcceleration = 10000; // default acceleratuion for z axis

int zDefaultSpeed = 2000;

int z_dir = -1;

//////////////////////////
////////// PINS //////////
//////////////////////////

// Relays
uint8_t PUMP = 59;
uint8_t VALVE = 60;
uint8_t UV_LED = 61;

// Z-Axis Motor
uint8_t Z_ST_DIR = 3;
uint8_t Z_ST_PUL = 4;
uint8_t Z_ST_EN = 5;

// Build plate spin motor
uint8_t BP_ST_DIR = 6;
uint8_t BP_ST_PUL = 7;
uint8_t BP_ST_EN = 8;

// Motor defintions and configs
AccelStepper z_stepper(1, Z_ST_PUL, Z_ST_DIR);
AccelStepper bp_stepper(1, BP_ST_PUL, BP_ST_DIR);

// define functions
void spinBP(int speed);
void zMove(int pos, int speed);
void zZero();
void zWipe(int low_bound, int high_bound, int speed);

//////////////////////////
////////// CODE //////////
//////////////////////////

void setup() {
  // enable both motors
  digitalWrite(Z_ST_EN, HIGH);
  digitalWrite(BP_ST_EN, HIGH);

  // set relays as output pins
  pinMode(PUMP, OUTPUT);
  pinMode(VALVE, OUTPUT);
  pinMode(UV_LED, OUTPUT);

  // set accelerations
  z_stepper.setMaxSpeed(maxStepsPerSec);
  bp_stepper.setMaxSpeed(maxStepsPerSec);

  z_stepper.setAcceleration(ZAcceleration);
  bp_stepper.setAcceleration(BPAcceleration);

  // Zero the Z
  // zZero();
}

void loop() {
  // run steppers with non blocking
  bp_stepper.run();
  z_stepper.run();

  // spinBP(4000);
  // zWipe(Z_LOW_BOUND, Z_HIGH_BOUND, zDefaultSpeed);

  
  
}

// spin build plate at a speed (positive or negative) with fixed acceleration (to protect build plate)
void spinBP(int speed) {
  bp_stepper.setMaxSpeed(speed);
  bp_stepper.setAcceleration(BPAcceleration);

  const long FAR = 1000000000L;
  bp_stepper.moveTo(speed >= 0 ? FAR : -FAR);
}

// Move Z to position at speed
void zMove(int pos, int speed) {
  z_stepper.setMaxSpeed(speed);
  z_stepper.moveTo(pos);
}

// zeros the build plate (0 is at the top of the screw drive)
// blocks everything
void zZero() {
  z_stepper.move(Z_HEIGHT);
  z_stepper.runToPosition();
}

// wipes z axis with a high and low bound at some speed
// non blocking function
void zWipe(int low_bound, int high_bound, int speed) {
  if (z_stepper.distanceToGo() == 0) {
    z_dir = z_dir * -1;
    if (z_dir == 1) {
      zMove(low_bound + Z_HEIGHT, speed);
    }
    else {
      zMove(high_bound + Z_HEIGHT, speed);
    }
  }
  else {
    ;
  }
}

// stops wipe in Z axis and geos to zero
void zStopWipe() {
  zMove(0, 2500);
}






