#include <Arduino.h>
#include <math.h>

const int sensorPins[5] = {34, 35, 32, 33, 25};

const int whiteLevel[5] = {500, 500, 500, 500, 500};
const int blackLevel[5] = {2500, 2500, 2500, 2500, 2500};

const int weights[5] = {-2, -1, 0, 1, 2};

const int ENA = 14;
const int IN1 = 27;
const int IN2 = 26;

const int ENB = 13;
const int IN3 = 12;
const int IN4 = 19;
const int STBY = 23;   // TB6612FNG standby pin, HIGH = driver on

const bool LEFT_REVERSED  = false;
const bool RIGHT_REVERSED = false;

const float Kp = 45.0;
const float Kd = 0.12;

const int BASE_SPEED = 130;
const int RECOVERY_SPEED = 80;
const int MIN_LINE_SUM = 80;

float previousError = 0.0;
float lastPosition = 0.0;

bool havePreviousError = false;
bool haveLastPosition = false;

unsigned long previousTimeUs = 0;


int calibrateSensor(int raw, int index) {
  int white = whiteLevel[index];
  int black = blackLevel[index];

  if (black <= white) {
    return 0;
  }

  long value = (long)(raw - white) * 1000L
               / (black - white);

  return constrain((int)value, 0, 1000);
}


void readSensors(int readings[5]) {
  for (int i = 0; i < 5; i++) {
    int raw = analogRead(sensorPins[i]);
    readings[i] = calibrateSensor(raw, i);
  }
}


bool estimatePosition(const int readings[5],
                      float &position) {
  long weightedSum = 0;
  long total = 0;

  for (int i = 0; i < 5; i++) {
    weightedSum += (long)readings[i] * weights[i];
    total += readings[i];
  }

  // Handle the all-zero or very weak signal safely.
  if (total < MIN_LINE_SUM) {
    return false;
  }

  position = (float)weightedSum / total;
  return true;
}


void setMotor(int pwmPin, int inA, int inB,
              int command, bool reversed) {
  if (reversed) {
    command = -command;
  }

  command = constrain(command, -255, 255);

  if (command > 0) {
    digitalWrite(inA, HIGH);
    digitalWrite(inB, LOW);
  } else if (command < 0) {
    digitalWrite(inA, LOW);
    digitalWrite(inB, HIGH);
  } else {
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
  }

  analogWrite(pwmPin, abs(command));
}


void driveMotors(int left, int right) {
  setMotor(ENA, IN1, IN2, left, LEFT_REVERSED);
  setMotor(ENB, IN3, IN4, right, RIGHT_REVERSED);
}


void stopMotors() {
  driveMotors(0, 0);
}


void recoverLine() {
  if (!haveLastPosition) {
    stopMotors();
    return;
  }

  if (lastPosition > 0.15) {
    driveMotors(RECOVERY_SPEED, -RECOVERY_SPEED);
  }
  else if (lastPosition < -0.15) {
    driveMotors(-RECOVERY_SPEED, RECOVERY_SPEED);
  } else {
    stopMotors();
  }
}


void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT);
    analogSetPinAttenuation(sensorPins[i], ADC_11db);
  }

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(STBY, OUTPUT);

  analogReadResolution(12);
  analogWriteResolution(ENA, 8);
  analogWriteResolution(ENB, 8);

  stopMotors();
  digitalWrite(STBY, HIGH);
  previousTimeUs = micros();
}


void loop() {
  int readings[5];
  float position = 0.0;

  readSensors(readings);

  unsigned long nowUs = micros();
  float dt = (nowUs - previousTimeUs) / 1000000.0;
  previousTimeUs = nowUs;

  if (dt < 0.001) {
    dt = 0.001;
  }

  bool lineDetected = estimatePosition(readings, position);

  if (!lineDetected) {
    Serial.println("Line lost: attempting recovery");

    havePreviousError = false;

    recoverLine();
    delay(10);
    return;
  }

  lastPosition = position;
  haveLastPosition = true;

  float error = 0.0 - position;

  float derivative = 0.0;

  if (havePreviousError) {
    derivative = (error - previousError) / dt;
  }

  float correction = (Kp * error) + (Kd * derivative);

  int leftPWM  = constrain(
      (int)round(BASE_SPEED - correction), 0, 255);

  int rightPWM = constrain(
      (int)round(BASE_SPEED + correction), 0, 255);

  driveMotors(leftPWM, rightPWM);

  previousError = error;
  havePreviousError = true;

  // Diagnostic output for tuning and Task 9.
  Serial.print("Sensors: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(readings[i]);
    Serial.print(i < 4 ? ", " : " | ");
  }

  Serial.print("Position: ");
  Serial.print(position, 2);

  Serial.print(" Error: ");
  Serial.print(error, 2);

  Serial.print(" Correction: ");
  Serial.print(correction, 2);

  Serial.print(" Left PWM: ");
  Serial.print(leftPWM);

  Serial.print(" Right PWM: ");
  Serial.println(rightPWM);

  delay(10);
}