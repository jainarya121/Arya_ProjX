# Task 5 — Power Budget & Battery Selection: Notes

The power budget table, worst-case total and battery comparison table are in [BOM/power_budget.md](../BOM/power_budget.md).

## In short

- **Worst case:** if both motors stall at full power while Wi-Fi transmits, the robot pulls about **1.67 A (18.5 W)** from the battery.
- **Runtime:** normal line following pulls about **0.45 A**, so the 1000 mAh 3S LiPo lasts about **107 minutes** per charge. That is roughly 19 full matches of 5 min 30 s.
- **Chemistry:** **LiPo**. A Li-ion 18650 pack stores more energy per gram and costs less per Wh, but it weighs twice as much, and the robot does not need the extra energy.

All figures are for the Task 4 BOM: ESP32 DevKit V1, TB6612FNG driver on a 9 V rail, two N20 12 V 200 RPM encoder motors, five TCRT5000 sensors, 3S 1000 mAh 30C LiPo.

## How the power is shared

The battery feeds three rails at different voltages (wiring as in the [power-block diagram](../BOM/power_block_diagram.jpeg)). Because the voltages differ, the currents cannot simply be added.

| Rail | Made by | Feeds |
|---|---|---|
| 9 V | XL4015 buck converter | TB6612FNG motor supply (VM), so both N20 motors |
| 5 V | MP1584 buck converter | ESP32 DevKit VIN |
| 3.3 V | AMS1117 regulator on the DevKit (fed from 5 V) | ESP32 chip, 5 IR sensors, 2 encoders, TB6612FNG logic, red END LED |

The simple rule is **power taken from the battery = power delivered ÷ converter efficiency**. For each rail I find its power (P = V × I), divide by the efficiency, add the rails, then divide by 11.1 V to get battery current.

## Why stall current drops at 9 V

The motors are rated at 12 V but run on the 9 V rail. A stalled motor is not spinning, so it acts like a plain resistor. Current = voltage ÷ winding resistance, so 75% of the voltage gives 75% of the current.

## Worst-case total

The battery-side total is in the [worst-case table](../BOM/power_budget.md#worst-case-total-at-the-battery). Quick sanity check: adding every current straight up, ignoring voltages, gives 1.98 A at peak. That overestimates, because a buck converter takes less current in than it gives out. So 1.67 A is the real figure and 1.98 A is a safe upper bound.

### What the worst-case number decides

- **Fuse:** at a nearly flat pack (10 V) peak current rises to 18.5 W ÷ 10 V = 1.85 A. A **3 A slow-blow fuse** never blows in normal use but does on a short. Slow-blow lets it ride through motor start-up surges and capacitor charging. It sits well below the 10 A switch rating.
- **Converters:** XL4015 carries 1.65 A (about 4.5 A recommended continuous). MP1584 carries 0.32 A (3 A rated). The ESP32 datasheet asks for a supply able to give at least 500 mA, which the 5 V rail can.
- **DevKit 3.3 V regulator:** carries 140 mA typical and 316 mA peak, because the sensors and encoders hang off the 3V3 pin. That is inside an AMS1117's rating; it only warms up at peak.
- **Motor driver:** 0.83 A per channel at stall, under the TB6612FNG's 1.2 A continuous limit (detail in Task 2).

## Runtime

**About 107 minutes of normal running per charge, roughly 19 full matches.**

```math
t = \frac{\text{usable capacity}}{\text{typical current}} = \frac{1000\ \text{mAh} \times 0.8}{447\ \text{mA}} \approx 1.79\ \text{h} \approx 107\ \text{min}
```

Cross-check with energy: 800 mAh × 11.1 V = 8.88 Wh, and 8.88 Wh ÷ 4.97 W = 1.79 h. Same answer.

### Assumptions

1. Battery is 1000 mAh at 11.1 V nominal (11.1 Wh). I use nominal voltage throughout.
2. Only 80% is usable (800 mAh). Draining a LiPo flat damages it, so I stop at about 3.5 V per cell at rest. This also keeps the battery comfortably above the 9 V the motor converter must output.
3. Each motor averages 0.2 A while running, about a quarter of its stall current. Rough check: rolling resistance plus accelerating a ~0.35 kg robot at 1 m/s² needs about 0.05 kg·cm per motor. That is ~5% of stall torque at 9 V, or about 0.14 A. The extra margin covers junction turns and hard stops.
4. Radio is off during a run; it is only needed for Task 7 diagnostics while testing.
5. Converter efficiencies are 90% (XL4015, rated up to 96%) and 85% (MP1584), typical for these step-downs at these loads.

### What if the assumptions are wrong?

The [runtime table](../BOM/power_budget.md#runtime-if-the-assumptions-are-wrong) recalculates the runtime for idle, double motor current and non-stop peak. Even the impossible worst case outlasts a 5 min 30 s match five times over. **Runtime does not decide the battery. Weight, size, voltage and peak current do.**

## Battery chemistry: why LiPo

**I chose the 3S LiPo.** Both packs compared are 3-cell, 11.1 V, from the same supplier, so the comparison is like for like. The full side-by-side table is in [BOM/power_budget.md](../BOM/power_budget.md#comparison-lipo-vs-li-ion).

- **Energy density:** Li-ion wins (161 vs 131 Wh/kg). But a match needs about 6 minutes and the LiPo already gives 107. Extra energy here is just extra weight.
- **C-rating:** both are enough. The LiPo has far more headroom (30 A vs 7.5 A), so its voltage dips less when the motors surge.
- **Weight:** the LiPo is half the weight, 85 g vs 172 g. On a robot of roughly 0.3 kg, Li-ion would add close to 30% mass. A lighter robot brakes and turns faster at junctions and overshoots less. The rules also forbid switching to a lighter battery later, so the light pack must be chosen from the start.
- **Cost:** Li-ion is ₹250 cheaper and far cheaper per Wh. The LiPo still fits the ₹10,000 BOM, and the BOM's IMAX B3 charger is made for it.
- **Size:** the flat LiPo lies low on the 150 × 130 mm plate, which keeps the centre of gravity low.
- **Spec:** Appendix A names a 3S LiPo, so no design changes are needed.

**The LiPo's weak point is safety.** It has no BMS and can swell or catch fire if over-discharged, punctured or charged wrongly. So: charge only on the balance charger, check cells with the alarm and stop at ~3.5 V per cell, keep the 3 A fuse next to the battery, and store it at ~3.8 V per cell.
