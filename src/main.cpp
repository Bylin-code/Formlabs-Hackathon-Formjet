#include <Arduino.h>
#include <AccelStepper.h>

//////////////////////////
////// DEFINITIONS ///////
//////////////////////////
// Constants
uint32_t Z_RANGE = 10000; // The range between the tallest and lowest Z position
uint32_t Z_LOW_BOUND = 4000; // the lowest position Z axis goes (0 is at the top)
uint32_t Z_HIGH_BOUND = 9400; // the highest position Z goes

int32_t Z_HEIGHT = -17000;

// Vars for speed stuff
int maxStepsPerSec = 3000; // max speed for all motors
int BPAcceleration = 200; // max acceleration for spinning build plat
int ZAcceleration = 10000; // default acceleratuion for z axis

int zDefaultSpeed = 1500;

int z_dir = -1;

// Wash & Cure Parameters (adjustable)
int soapWipeCycles = 4;      // Number of wipe cycles with soap
int waterWipeCycles = 0;      // Number of wipe cycles with water
int cureWipeCycles = 2;       // Number of wipe cycles during curing
int washSpinSpeed = 1000;     // BP spin speed during wash

// State variables
bool cycleRunning = false;
enum CycleState { IDLE, SOAP_WASH, WATER_WASH, CURE, COMPLETE };
CycleState currentState = IDLE;
int currentWipeCycle = 0;

//////////////////////////
////////// PINS //////////
//////////////////////////

// Relays
uint8_t PUMP = 2;
uint8_t VALVE = 3;
uint8_t UV_LED = 4;

// Z-Axis Motor
uint8_t Z_ST_DIR = 54;
uint8_t Z_ST_PUL = 55;
uint8_t Z_ST_EN = 56;

// Build plate spin motor
uint8_t BP_ST_DIR = 57;
uint8_t BP_ST_PUL = 58;
uint8_t BP_ST_EN = 59;

// Motor defintions and configs
AccelStepper z_stepper(1, Z_ST_PUL, Z_ST_DIR);
AccelStepper bp_stepper(1, BP_ST_PUL, BP_ST_DIR);

// define functions
void spinBP(int speed);
void zMove(int pos, int speed);
void zZero();
void zWipe(int low_bound, int high_bound, int speed);
void zStopWipe();
void startWashCureCycle();
void handleWashCureCycle();
void printMenu();
void processUserInput();

//////////////////////////
////////// CODE //////////
//////////////////////////

void setup() {
  // Initialize Serial communication
  Serial.begin(115200);
  while (!Serial) { ; } // Wait for serial port to connect
  
  // enable both motors
  digitalWrite(Z_ST_EN, HIGH);
  digitalWrite(BP_ST_EN, HIGH);

  // set relays as output pins
  pinMode(PUMP, OUTPUT);
  pinMode(VALVE, OUTPUT);
  pinMode(UV_LED, OUTPUT);
  
  // Make sure all relays start OFF
  digitalWrite(PUMP, LOW);
  digitalWrite(VALVE, LOW);
  digitalWrite(UV_LED, LOW);

  // set accelerations
  z_stepper.setMaxSpeed(maxStepsPerSec);
  bp_stepper.setMaxSpeed(maxStepsPerSec);

  z_stepper.setAcceleration(ZAcceleration);
  bp_stepper.setAcceleration(BPAcceleration);

  // Zero the Z
  Serial.println("\n=== FormJet Wash & Cure System ===");
  Serial.println("Zeroing Z-axis...");
  zZero();
  Serial.println("System ready!\n");
  
  // Print menu
  printMenu();
}

void loop() {
  // run steppers with non blocking
  bp_stepper.run();
  z_stepper.run();

  // Process user input
  processUserInput();
  
  // Handle wash & cure cycle if running
  if (cycleRunning) {
    handleWashCureCycle();
  }
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
  z_stepper.setCurrentPosition(0); // Set bottom position as 0
}

// wipes z axis with a high and low bound at some speed
// non blocking function
void zWipe(int low_bound, int high_bound, int speed) {
  if (z_stepper.distanceToGo() == 0) {
    z_dir = z_dir * -1;
    if (z_dir == 1) {
      zMove(high_bound, speed);
    }
    else {
      zMove(low_bound, speed);
    }
  }
  else {
    ;
  }
}

// stops wipe in Z axis and goes to zero
void zStopWipe() {
  zMove(0, 2500);
}

// Print menu options
void printMenu() {
  Serial.println("\n===== MENU =====");
  Serial.println("Commands:");
  Serial.println("  s - Start Wash & Cure Cycle");
  Serial.println("  1 - Set soap wipe cycles (current: " + String(soapWipeCycles) + ")");
  Serial.println("  2 - Set water wipe cycles (current: " + String(waterWipeCycles) + ")");
  Serial.println("  3 - Set cure wipe cycles (current: " + String(cureWipeCycles) + ")");
  Serial.println("  m - Show this menu");
  Serial.println("================");
  Serial.println();
}

// Process user input from Serial
void processUserInput() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    
    // Clear any extra input
    while (Serial.available() > 0) {
      Serial.read();
    }
    
    switch (input) {
      case 's':
      case 'S':
        if (!cycleRunning) {
          Serial.println("\nStarting Wash & Cure Cycle...");
          startWashCureCycle();
        } else {
          Serial.println("Cycle already running!");
        }
        break;
        
      case '1':
        Serial.println("\nEnter number of soap wipe cycles (1-10):");
        while (Serial.available() == 0) { ; }
        soapWipeCycles = Serial.parseInt();
        soapWipeCycles = constrain(soapWipeCycles, 1, 10);
        Serial.println("Soap wipe cycles set to: " + String(soapWipeCycles));
        break;
        
      case '2':
        Serial.println("\nEnter number of water wipe cycles (1-10):");
        while (Serial.available() == 0) { ; }
        waterWipeCycles = Serial.parseInt();
        waterWipeCycles = constrain(waterWipeCycles, 1, 10);
        Serial.println("Water wipe cycles set to: " + String(waterWipeCycles));
        break;
        
      case '3':
        Serial.println("\nEnter number of cure wipe cycles (1-10):");
        while (Serial.available() == 0) { ; }
        cureWipeCycles = Serial.parseInt();
        cureWipeCycles = constrain(cureWipeCycles, 1, 10);
        Serial.println("Cure wipe cycles set to: " + String(cureWipeCycles));
        break;
        
      case 'm':
      case 'M':
        printMenu();
        break;
        
      default:
        // Ignore other input
        break;
    }
  }
}

// Start wash & cure cycle
void startWashCureCycle() {
  cycleRunning = true;
  currentState = SOAP_WASH;
  currentWipeCycle = 0;
  z_dir = -1; // Reset wipe direction
  
  // Start pump and valve (soap)
  digitalWrite(PUMP, HIGH);
  digitalWrite(VALVE, HIGH);
  Serial.println("Phase 1: Washing with soap...");
  Serial.println("  Pump: ON, Valve: ON (soap), BP Speed: " + String(washSpinSpeed));
  
  // Start BP spinning
  spinBP(washSpinSpeed);
}

// Handle wash & cure cycle states
void handleWashCureCycle() {
  static bool waitingForWipeComplete = false;
  
  switch (currentState) {
    case SOAP_WASH:
      // Wipe with soap and valve ON
      if (!waitingForWipeComplete) {
        waitingForWipeComplete = true;
        zWipe(Z_LOW_BOUND, Z_HIGH_BOUND, zDefaultSpeed);
      }
      
      // Check if wipe cycle is complete
      if (z_stepper.distanceToGo() == 0 && waitingForWipeComplete) {
        currentWipeCycle++;
        Serial.println("  Soap wipe cycle " + String(currentWipeCycle) + "/" + String(soapWipeCycles) + " complete");
        
        if (currentWipeCycle >= soapWipeCycles) {
          // Move to water wash
          currentState = WATER_WASH;
          currentWipeCycle = 0;
          digitalWrite(VALVE, LOW); // Switch valve to water
          Serial.println("\nPhase 2: Washing with water...");
          Serial.println("  Pump: ON, Valve: OFF (water), BP Speed: " + String(washSpinSpeed));
        }
        waitingForWipeComplete = false;
      }
      break;
      
    case WATER_WASH:
      // Wipe with water (valve OFF)
      if (!waitingForWipeComplete) {
        waitingForWipeComplete = true;
        zWipe(Z_LOW_BOUND, Z_HIGH_BOUND, zDefaultSpeed);
      }
      
      if (z_stepper.distanceToGo() == 0 && waitingForWipeComplete) {
        currentWipeCycle++;
        Serial.println("  Water wipe cycle " + String(currentWipeCycle) + "/" + String(waterWipeCycles) + " complete");
        
        if (currentWipeCycle >= waterWipeCycles) {
          // Move to cure phase
          currentState = CURE;
          currentWipeCycle = 0;
          digitalWrite(PUMP, LOW);   // Turn off pump
          digitalWrite(UV_LED, HIGH); // Turn on UV LED
          Serial.println("\nPhase 3: Curing...");
          Serial.println("  Pump: OFF, UV LED: ON, BP Speed: " + String(washSpinSpeed));
        }
        waitingForWipeComplete = false;
      }
      break;
      
    case CURE:
      // Wipe during cure with LED on
      if (!waitingForWipeComplete) {
        waitingForWipeComplete = true;
        zWipe(Z_LOW_BOUND, Z_HIGH_BOUND, zDefaultSpeed);
      }
      
      if (z_stepper.distanceToGo() == 0 && waitingForWipeComplete) {
        currentWipeCycle++;
        Serial.println("  Cure wipe cycle " + String(currentWipeCycle) + "/" + String(cureWipeCycles) + " complete");
        
        if (currentWipeCycle >= cureWipeCycles) {
          // Complete cycle
          currentState = COMPLETE;
        }
        waitingForWipeComplete = false;
      }
      break;
      
    case COMPLETE:
      // Turn everything off
      digitalWrite(PUMP, LOW);
      digitalWrite(VALVE, LOW);
      digitalWrite(UV_LED, LOW);
      spinBP(0); // Stop BP
      bp_stepper.stop();
      
      Serial.println("\n=== Wash & Cure Cycle COMPLETE! ===");
      Serial.println();
      
      // Ask if user wants to run again
      Serial.println("Do you want to start another cycle? (s = yes, any other key = no)");
      cycleRunning = false;
      currentState = IDLE;
      break;
      
    default:
      break;
  }
}






