#define dirPin 2
#define stepPin 3
int setpoint = 0;
int current = 0;
int speed = 50;
bool kill = false;

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