/* Perfect Settings:
 * 1/32 microstepping (6400 pulses/rev)
 * 0.5A is more than enough for moving an iPhone
 * vertically but more current may be needed for
 * a larger camera. Motors are stone-cold @ 0.5A
 * 
 * Full travel is rougly 100,000 pulses.
 * Maximum speed is about 15 microseconds between
 * pulses.
 */

#define enPin 4
#define dirPin 3
#define stepPin 2
long setpoint = 0;
long current = 0;
int speed = 50;
bool kill = true;

void setup() {
  // Declare pins as output:
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(dirPin, LOW);
  Serial.begin(9600);
}

void loop() {
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
    digitalWrite(enPin, LOW);
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
  if(kill)
    digitalWrite(4, HIGH);
}

void flush() {
  while (Serial.available() > 0) {
    char t = Serial.read();
  }
}