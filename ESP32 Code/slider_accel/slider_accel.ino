// Include the AccelStepper library:
#include <AccelStepper.h>

#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

void setup() {
  // Set the maximum speed and acceleration:
  stepper.setMaxSpeed(10000);
  stepper.setAcceleration(1000);
}

void loop() {
  // Set the target position:
  stepper.moveTo(40000);
  // Run to target position with set speed and acceleration/deceleration:
  stepper.runToPosition();

  //delay(100);

  // Move back to zero:
  stepper.moveTo(0);
  stepper.runToPosition();

  //delay(1000);
}
