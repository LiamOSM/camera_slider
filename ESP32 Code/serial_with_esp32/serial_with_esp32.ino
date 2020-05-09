/* Perfect Settings:
   1/32 microstepping (6400 pulses/rev)
   0.5A is more than enough for moving an iPhone
   vertically but more current may be needed for
   a larger camera. Motors are stone-cold @ 0.5A

   Full travel is rougly 100,000 pulses.
   Maximum speed is about 15 microseconds between
   pulses.
*/
#define enPin 19
#define dirPin 18
#define stepPin 5

#define rLim 22
#define lLim 23

long length = 0;
long setpoint = 0;
long current = 0;
int speed = 50;
int calSpeed = 25;
bool kill = true;

void IRAM_ATTR rLimISR() { // zero position (home)
  current = 0;
  kill = true;
  //Serial.println(current);
}
void IRAM_ATTR lLimISR() { // positive limit
  kill = true;
  Serial.println(current);
}

void setup() {
  // Declare pins as output:
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  pinMode(rLim, INPUT_PULLUP);
  pinMode(lLim, INPUT_PULLUP);
  attachInterrupt(rLim, rLimISR, FALLING);
  attachInterrupt(lLim, lLimISR, FALLING);
  digitalWrite(dirPin, LOW);
  Serial.begin(9600);
  while (1) {
    calibrate();
  }
  current = length;
}

void loop() {
  digitalWrite(enPin, kill);
  while (Serial.available() > 0) {
    if (Serial.peek() == 'x') {
      kill = !kill;
      Serial.println(kill ? "Stopping" : "Resuming");
    } else if (Serial.peek() == 'a') {
      Serial.read();
      setpoint = Serial.parseInt();
      Serial.print("Setpoint = ");
      Serial.println(setpoint);
    } else if (Serial.peek() == 'm') {
      Serial.read();
      setpoint += Serial.parseInt();
      Serial.print("Setpoint = ");
      Serial.println(setpoint);
    } else if (Serial.peek() == 's') {
      Serial.read();
      speed = Serial.parseInt();
      Serial.print("Speed = ");
      Serial.println(speed);
    } else if (Serial.peek() == 'c') {
      setpoint = 0;
      current = 0;
      Serial.println("Home");
    }
    flush();
  }

  if (!kill) {
    if (setpoint > current) {
      digitalWrite(dirPin, LOW);
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(speed);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(speed);
      current++;
    }
    else if (setpoint < current) {
      digitalWrite(dirPin, HIGH);
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(speed);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(speed);
      current--;
    }
  }
}

void flush() {
  while (Serial.available() > 0) {
    char t = Serial.read();
  }
}

void calibrate() {
  detachInterrupt(rLim);
  detachInterrupt(lLim);
  digitalWrite(enPin, LOW);
  unsigned long temp = 0;

  // Move left (out)
  digitalWrite(dirPin, LOW);
  while (digitalRead(lLim) != 0) {
    // move left until the limit is reached
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(calSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(calSpeed);
  }
  delay(250);

  // Move right (home)
  digitalWrite(dirPin, HIGH);
  while (digitalRead(rLim) != 0) {
    // move right until the limit is reached, while counting steps
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(calSpeed);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(calSpeed);
    temp++;
  }
  delay(250);
  length = temp;
  Serial.print("Length: ");
  Serial.println(temp);
}
