# Task 9 — PID Line Follower: Tuning Notes

## In short

[`Task9_LineFollower.ino`](../src/Task9_LineFollower/Task9_LineFollower.ino) runs a 10 ms loop:

1. Read the 5 IR sensors and scale each to 0–1000 using its own min and max from calibration.
2. Find the line position with a weighted centroid, from −2 (far left) to +2 (far right).
3. PID: error = 0 − position. One wheel speeds up and the other slows down by the same amount.
4. If no sensor sees the line, spin towards where it was last seen. If it is still lost after 1.5 s, stop.
5. Print the sensor values, position, error and both motor commands over Serial (115200).

**To test without the robot:** leave `RUN_TESTS = true`, upload to a bare ESP32 and open the Serial Monitor. It runs the sample inputs below and never drives the motors. Set it to `false` to drive.

## PID tuning choices

**I use PD (Ki = 0) as chosen in Task 3, with starting values worked out from the robot's numbers rather than guessed.**

| Setting | Value | Why |
|---|---|---|
| `Kp` | 60 | At the largest error (±2, line under an edge sensor) P gives 120 PWM. The inner wheel drops to 30 and the outer rises to 255 (capped), sharp enough for a 90° corner. At a small error of 0.25, P gives only 15 PWM, a gentle nudge. |
| `Kd` | 240 (per 10 ms loop) | Kd ÷ Kp × 10 ms = 40 ms, so D steers as if the error were already 40 ms further on. This damps the overshoot P alone causes. In test 2, D adds 24 PWM while P adds only 6, so the robot reacts before the error grows. |
| `Ki` | 0 | The integral builds up on every curve and makes the robot overshoot coming out of it. It only helps a steady offset, like always running slightly left on a straight. If that shows up, try Ki = 0.5. The integral is capped at ±100, so it can add at most 50 PWM. |
| `BASE_SPEED` | 150 of 255 | At 9 V the motors do about 150 RPM, about 0.33 m/s at full PWM on 42 mm wheels. 150 PWM gives roughly 0.19 m/s, just above the 0.16 m/s Task 8 needs for the dry run. It also leaves 105 PWM of room for the outer wheel to speed up. |
| `LOOP_MS` | 10 ms | 100 corrections a second; at 0.2 m/s the robot moves 2 mm per loop. One telemetry line is 68 bytes, about 6 ms at 115200 baud, so printing fits in the loop. |
| `LINE_SEEN` | 200 of 1000 | After calibration the floor reads near 0 and the line near 1000. 200 is well above floor noise and well below a sensor on the line. |
| `SEARCH_SPEED` | 110 | Slower than cruising, so the robot does not spin past the line when it finds it. |
| `MAX_LOST_LOOPS` | 150 (1.5 s) | About half a turn at search speed, assuming roughly 12 cm between the wheels. If the line is not back by then, a safe stop beats wandering off. |

## How to tune on the real robot

The values above are starting points; the real robot settles them.

1. Set Ki = 0, Kd = 0 and `BASE_SPEED` = 120. Raise Kp until the robot follows a straight line with a small, steady wobble.
2. Add Kd in steps of about 50 until the wobble dies out. If the motors start to buzz or twitch, Kd is amplifying sensor noise, so step back.
3. Raise `BASE_SPEED` by 10–20 and repeat steps 1–2. Stop when 90° corners start to fail.
4. Only if it keeps running to one side on straights, add a small Ki.
5. Watch the error in the telemetry (over Bluetooth, Task 7): an error swinging between + and − means too much Kp or too little Kd.

If `LOOP_MS` changes, Kd must change by the same factor, because D is measured per loop.

## Test results

**All 13 sample inputs give the values worked out by hand.** Test calibration: floor reads 300 and line reads 3800 on every sensor, so 0–1000 after scaling. Raw inputs are in the code; the table shows scaled values.

| Test | Sensors S1–S5 (scaled) | Position | Error | Left PWM | Right PWM | Check |
|---|---|---|---|---|---|---|
| Centred | 0, 0, 1000, 0, 0 | 0.00 | 0.00 | 150 | 150 | Straight on |
| Line a bit right | 0, 0, 500, 500, 0 | 0.50 | −0.50 | 180 | 120 | 150 ± 60 × 0.5 |
| Line far right | 0, 0, 0, 500, 1000 | 1.67 | −1.67 | 250 | 50 | 150 ± 60 × 1.67 |
| Line far left | 1000, 500, 0, 0, 0 | −1.67 | 1.67 | 50 | 250 | Mirror of far right |
| Drift, loop 1 | 0, 0, 1000, 0, 0 | 0.00 | 0.00 | 150 | 150 | Start |
| Drift, loop 2 | 0, 0, 900, 100, 0 | 0.10 | −0.10 | 180 | 120 | P = −6, D = −24 |
| Drift, loop 3 | 0, 0, 800, 200, 0 | 0.20 | −0.20 | 186 | 114 | P = −12, D = −24 |
| Drift, loop 4 | 0, 0, 700, 300, 0 | 0.30 | −0.30 | 192 | 108 | P = −18, D = −24 |
| Line far right | 0, 0, 0, 500, 1000 | 1.67 | −1.67 | 250 | 50 | Last seen right |
| All sensors on floor | 0, 0, 0, 0, 0 | lost | — | 110 | −110 | Spins right to search |
| Still lost after 151 loops | 0, 0, 0, 0, 0 | lost | — | 0 | 0 | Safe stop after 1.5 s |
| Line found again | 0, 0, 1000, 0, 0 | 0.00 | 0.00 | 150 | 150 | No derivative kick |
| All readings 0 | 0, 0, 0, 0, 0 | lost | — | 0 | 0 | All-zero case; last seen centred, so stop |

The drift test shows why D matters: at an error of only 0.3, P alone would give 168/132, but with D the robot is already turning at 192/108.

## Notes

- **IN4 (BIN2) on GPIO 19, STBY on GPIO 23.** Appendix A puts IN4 on GPIO 11, which is wired to the ESP32's flash chip. GPIO 23 drives the TB6612FNG's STBY pin HIGH only at the end of `setup()`, so the motors stay off while the ESP32 boots. This matches the wiring diagram.
- **Line colour:** Appendix A says higher voltage = darker = line, but Meshmerize uses a white line on a black floor. Check which way your sensors read during calibration; if the line reads lower than the floor, set `WHITE_LINE = true`.
- **Motor driver:** the same six control pins drive the L298N in Appendix A or the TB6612FNG in my BOM. The TB6612FNG also uses STBY (GPIO 23); an L298N has no such pin.
- **Wi-Fi off:** sensor S5 is on GPIO 25, which is ADC2, and ADC2 stops working when Wi-Fi is on (Task 7).
- **Calibration:** for the first 5 s, slide the sensors across the line and the floor by hand. A sensor whose min–max range is under 500 is flagged on Serial.
- **Validation:** the tests above were run by compiling the sketch with g++ and a small stand-in for the Arduino functions; it has not yet run on an ESP32 or the robot.
