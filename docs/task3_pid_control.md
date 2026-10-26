# Task 3 — PID Control for Line Following

PID stands for Proportional, Integral and Derivative. It is a feedback control technique that helps a robot adjust its movement based on the difference between the desired position and its actual position.

For a line-following robot, the desired position is the centre of the line sensor array. The IR sensors measure where the line is, and the controller adjusts the left and right motor speeds to steer the robot back towards the line.

The combined control equation is:

```math
u = K_p e + K_i \int e \, dt + K_d \frac{de}{dt}
```

Here, $e$ is the error and $u$ is the motor-speed correction. The constants $K_p$, $K_i$ and $K_d$ determine how strongly each term affects the correction.

## The three terms

**Proportional** — responds to the current error

The greater the distance between the line and the centre, the larger the correction. A small error produces a small steering adjustment; a large error produces a stronger turn.

If Kp is too low, the robot may turn too weakly. If it is too high, the robot may repeatedly overshoot the line and oscillate.

**Integral** — responds to accumulated error

The controller adds up the error over time. This can correct a persistent bias, such as the robot consistently drifting to one side because its motors behave differently.

Too much integral action can cause overshooting or integral windup, where the accumulated error stays large even after the robot needs a different correction.

**Derivative** — responds to how quickly the error changes

The controller considers whether the robot is moving towards or away from the centre and how rapidly the error is changing. This helps dampen overshooting and reduce oscillation. Too much derivative action makes the controller sensitive to noisy sensor readings.

## Chosen configuration: PD

I would start with PD control (Proportional and Derivative).

A line follower needs to react quickly when the line moves away from the centre. The proportional term provides the immediate steering correction, while the derivative term helps reduce oscillation. For a basic robot using five IR sensors and an L298N driver, the integral term may not be necessary at the start.

## From sensor readings to motor commands

**Step 1: Calculate the line position.** The five calibrated sensor readings use weights [-2, -1, 0, 1, 2]. We calculate the weighted mean.

**Step 2: Calculate the error.** The target position is zero: e = Target – Position

**Step 3: Calculate the steering correction.** correction = Kp × e + Kd × (de/dt)

**Step 4: Adjust the two motor speeds.** Left PWM = Base − correction, Right PWM = Base + correction

With the values in the [Task 3 sketch](../src/Task3_PD/Task3_PD.ino) (Base = 130, Kp = 45):

| Sensor readings | Estimated position | Left PWM | Right PWM |
|---|---|---|---|
| {0, 500, 1000, 500, 0} | 0.00 | 130 | 130 |
| {0, 0, 0, 500, 1000} | +1.67 | 205 | 55 |
| {1000, 500, 0, 0, 0} | −1.67 | 55 | 205 |
| {0, 0, 0, 0, 0} | Line lost | Recovery behaviour | Recovery behaviour |

## Oscillation and lost line

If the robot oscillates from side to side: reduce Kp first, or reduce the base speed. If the robot still overshoots while the sensors are reading cleanly, gradually increase Kd. If the readings are noisy, excessive derivative action may worsen the behaviour.

If the line is lost, we turn in the direction in which the line was last read by the sensors.

The tuned values used on the robot are in [Task 9's notes](task9_pid_tuning.md).
