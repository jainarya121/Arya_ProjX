/*
  Task 9 - Line follower with PID control (ESP32, Arduino)
  ========================================================
  Every 10 ms the robot:
    1. reads the 5 IR sensors and scales each one to 0..1000 (min-max calibration)
    2. finds the line position with a weighted centroid: -2 (far left) .. +2 (far right)
    3. PID: error = 0 - position, then speeds up one wheel and slows the other
    4. if no sensor sees the line: spins towards where the line was last seen,
       and stops safely if it is not found within 1.5 s
    5. prints sensor values, position, error and motor commands (115200 baud)

  RUN_TESTS = true  -> runs the hard-coded sample inputs at the bottom, prints the
                       results and never drives the motors. Set false to drive.

  Pins are from Appendix A, except IN4: GPIO 11 is wired to the ESP32's flash
  chip, so IN4 (BIN2) uses GPIO 19. GPIO 23 drives the TB6612FNG's STBY pin
  (see docs/task6_wiring.md). With an L298N, leave STBY unconnected.
*/

// ================== SETTINGS ==================
const bool RUN_TESTS  = true;   // true = test with sample inputs, motors stay off
const bool WHITE_LINE = false;  // Appendix A: higher voltage = darker = line.
                                // If your line reads LOWER than the floor, set true.

// PID gains (the Task 9 notes explain each choice)
const float Kp = 60.0;    // PWM per unit of error
const float Ki = 0.0;     // off: PD is enough for a line follower
const float Kd = 240.0;   // PWM per unit of error change per loop

const int BASE_SPEED     = 150;  // PWM (0-255) of both wheels when the line is centred
const int MAX_PWM        = 255;
const int SEARCH_SPEED   = 110;  // PWM while spinning to find a lost line
const int LINE_SEEN      = 200;  // a sensor above this (out of 1000) can see the line
const int MAX_LOST_LOOPS = 150;  // 150 loops x 10 ms = 1.5 s of searching, then stop
const int LOOP_MS        = 10;   // control loop period

// ================== PINS (Appendix A) ==================
const int SENSOR_PIN[5] = { 34, 35, 32, 33, 25 };   // S1 (leftmost) .. S5 (rightmost)
const int WEIGHT[5]     = { -2, -1, 0, 1, 2 };

const int ENA = 14, IN1 = 27, IN2 = 26;   // left motor
const int ENB = 13, IN3 = 12, IN4 = 19;   // right motor (IN4 moved from GPIO 11)
const int STBY = 23;                      // TB6612FNG standby: LOW = motors off

// ================== VARIABLES ==================
int minRaw[5], maxRaw[5];   // from calibration
int raw[5];                 // readings, 0..4095
int value[5];               // scaled, 0..1000 (1000 = right on the line)

float position = 0;         // -2 .. +2, 0 = line under the centre sensor
float lastPosition = 0;
float error = 0;
float lastError = 0;
float integral = 0;
int   leftPWM = 0;          // -255 .. +255, minus = backwards
int   rightPWM = 0;

bool lineLost = true;       // true until the line is seen
int  lostLoops = 0;         // how many loops in a row the line has been lost
unsigned long lastLoopTime = 0;

// ================== 1. SENSORS ==================
void readSensors() {
  for (int i = 0; i < 5; i++) {
    raw[i] = analogRead(SENSOR_PIN[i]);
  }
}

// Scale each reading to 0..1000 using that sensor's own min and max.
void scaleSensors() {
  for (int i = 0; i < 5; i++) {
    int range = maxRaw[i] - minRaw[i];
    if (range <= 0) {
      value[i] = 0;
      continue;
    }
    long v = (long)(raw[i] - minRaw[i]) * 1000 / range;
    value[i] = constrain(v, 0, 1000);
    if (WHITE_LINE) value[i] = 1000 - value[i];
  }
}

// For 5 seconds, slide the sensors across the line and the floor by hand.
void calibrate() {
  for (int i = 0; i < 5; i++) {
    minRaw[i] = 4095;
    maxRaw[i] = 0;
  }
  Serial.println("Calibrating for 5 s: slide the sensors across the line and the floor.");
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    readSensors();
    for (int i = 0; i < 5; i++) {
      if (raw[i] < minRaw[i]) minRaw[i] = raw[i];
      if (raw[i] > maxRaw[i]) maxRaw[i] = raw[i];
    }
    delay(5);
  }
  for (int i = 0; i < 5; i++) {
    Serial.printf("S%d: min %d, max %d", i + 1, minRaw[i], maxRaw[i]);
    if (maxRaw[i] - minRaw[i] < 500) Serial.print("  <- small range, check this sensor");
    Serial.println();
  }
}

// ================== 2. LINE POSITION (weighted centroid) ==================
// Returns false if no sensor can see the line (this also covers all zeros).
bool findLine() {
  long sum = 0;
  long weightedSum = 0;
  int strongest = 0;
  for (int i = 0; i < 5; i++) {
    sum += value[i];
    weightedSum += (long)WEIGHT[i] * value[i];
    if (value[i] > strongest) strongest = value[i];
  }
  if (strongest < LINE_SEEN) return false;
  position = (float)weightedSum / sum;
  return true;
}

// ================== 3. PID ==================
// Keep a motor command between -MAX_PWM and +MAX_PWM, keeping its sign (Task 1).
int limit(int speed) {
  if (speed > MAX_PWM)  return MAX_PWM;
  if (speed < -MAX_PWM) return -MAX_PWM;
  return speed;
}

void followLine() {
  error = 0 - position;          // target is the centre (position 0)

  if (lineLost) {                // line just found: start D and I fresh (no sudden kick)
    lastError = error;
    integral = 0;
    lineLost = false;
  }

  integral = constrain(integral + error, -100, 100);   // clamp so I can't wind up
  float derivative = error - lastError;                 // change since the last loop
  float correction = Kp * error + Ki * integral + Kd * derivative;
  lastError = error;

  // Line to the right -> error negative -> correction negative
  // -> left wheel faster, right wheel slower -> robot turns right.
  leftPWM  = limit((int)round(BASE_SPEED - correction));
  rightPWM = limit((int)round(BASE_SPEED + correction));

  lastPosition = position;
  lostLoops = 0;
}

// ================== 4. LOST LINE ==================
void searchForLine() {
  lineLost = true;
  lostLoops++;

  if (lostLoops > MAX_LOST_LOOPS) {        // searched too long: safe stop
    leftPWM = 0;
    rightPWM = 0;
  } else if (lastPosition > 0.5) {         // line was last on the right: spin right
    leftPWM = SEARCH_SPEED;
    rightPWM = -SEARCH_SPEED;
  } else if (lastPosition < -0.5) {        // line was last on the left: spin left
    leftPWM = -SEARCH_SPEED;
    rightPWM = SEARCH_SPEED;
  } else {                                 // lost while centred: the line ended, stop
    leftPWM = 0;
    rightPWM = 0;
  }
}

// One control step on the current readings.
void controlStep() {
  if (findLine()) followLine();
  else searchForLine();
}

// ================== MOTORS ==================
// speed: -255..+255. Plus = forward, minus = backward, 0 = stop.
void setMotor(int pwmPin, int pinA, int pinB, int speed) {
  if (speed > 0) {
    digitalWrite(pinA, HIGH);
    digitalWrite(pinB, LOW);
  } else if (speed < 0) {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, HIGH);
  } else {
    digitalWrite(pinA, LOW);
    digitalWrite(pinB, LOW);
  }
  analogWrite(pwmPin, abs(speed));
}

void driveMotors() {
  setMotor(ENA, IN1, IN2, leftPWM);
  setMotor(ENB, IN3, IN4, rightPWM);
}

// ================== 5. TELEMETRY ==================
void printStatus() {
  Serial.printf("S: %4d %4d %4d %4d %4d | ", value[0], value[1], value[2], value[3], value[4]);
  if (lineLost) {
    Serial.printf("line lost (loop %3d)  | ", lostLoops);
  } else {
    Serial.printf("pos %5.2f  err %5.2f | ", position, error);
  }
  Serial.printf("L %4d  R %4d\n", leftPWM, rightPWM);
}

// ================== TESTS WITH SAMPLE INPUTS ==================
// Raw readings (0..4095) as the ADC would give them. For the tests,
// calibration says: floor reads 300 and line reads 3800 on every sensor.
void runTest(const char *label, int s1, int s2, int s3, int s4, int s5) {
  raw[0] = s1;
  raw[1] = s2;
  raw[2] = s3;
  raw[3] = s4;
  raw[4] = s5;
  scaleSensors();
  controlStep();
  Serial.printf("%-24s", label);
  printStatus();
}

void runTests() {
  for (int i = 0; i < 5; i++) {
    minRaw[i] = 300;
    maxRaw[i] = 3800;
  }

  Serial.println("\n--- 1. Single readings (each starts fresh, so D = 0) ---");
  lineLost = true;  runTest("Centred",          300,  300, 3800,  300,  300);  // L 150 R 150
  lineLost = true;  runTest("Line a bit right", 300,  300, 2050, 2050,  300);  // pos 0.5  -> L 180 R 120
  lineLost = true;  runTest("Line far right",   300,  300,  300, 2050, 3800);  // pos 1.67 -> L 250 R 50
  lineLost = true;  runTest("Line far left",   3800, 2050,  300,  300,  300);  // pos -1.67 -> L 50 R 250

  Serial.println("\n--- 2. Line drifting right, one loop at a time (D reacts early) ---");
  lineLost = true;
  runTest("Loop 1", 300, 300, 3800,  300, 300);   // pos 0.0 -> L 150 R 150
  runTest("Loop 2", 300, 300, 3450,  650, 300);   // pos 0.1 -> P -6,  D -24 -> L 180 R 120
  runTest("Loop 3", 300, 300, 3100, 1000, 300);   // pos 0.2 -> P -12, D -24 -> L 186 R 114
  runTest("Loop 4", 300, 300, 2750, 1350, 300);   // pos 0.3 -> P -18, D -24 -> L 192 R 108

  Serial.println("\n--- 3. Lost line ---");
  lineLost = true;
  runTest("Line far right",       300, 300, 300, 2050, 3800); // last seen on the right
  runTest("All sensors on floor", 300, 300, 300,  300,  300); // spin right: L 110 R -110
  for (int i = 0; i < MAX_LOST_LOOPS - 1; i++) {              // 1.49 s more with no line
    controlStep();
  }
  runTest("Still lost at 1.5 s", 300, 300, 300, 300, 300);   // safe stop: L 0 R 0
  runTest("Line found again",    300, 300, 3800, 300, 300);  // back to L 150 R 150
  runTest("All readings 0",        0,   0,    0,   0,   0);  // all-zero case: lost, centred -> stop

  Serial.println("\nTests done. Set RUN_TESTS = false to drive.");
}

// ================== setup / loop ==================
void setup() {
  Serial.begin(115200);
  delay(500);
  analogReadResolution(12);   // readings 0..4095

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);    // driver off while setting up and during tests
  driveMotors();              // both 0 = stopped

  if (RUN_TESTS) {
    runTests();
    return;
  }

  calibrate();
  Serial.println("Starting in 2 s...");
  delay(2000);
  digitalWrite(STBY, HIGH);   // enable the motor driver only now
}

void loop() {
  if (RUN_TESTS) return;      // tests already ran in setup()

  if (millis() - lastLoopTime < LOOP_MS) return;   // run every 10 ms
  lastLoopTime = millis();

  readSensors();
  scaleSensors();
  controlStep();
  driveMotors();
  printStatus();
}
