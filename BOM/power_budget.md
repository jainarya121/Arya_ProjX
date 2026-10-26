# Power Budget (Task 5)

Tables for the robot in [BOM.md](BOM.md). The reasoning, runtime calculation and assumptions are in [docs/task5_power_budget.md](../docs/task5_power_budget.md).

**Worst case 1.67 A (18.5 W) from the battery; typical running 0.45 A; about 107 minutes per charge.**

## Current draw per component

Each current is measured at the part's own rail. **Idle** = switched on at the start line, motors off. **Typical** = normal line following, radio off. **Peak** = worst case: both motors stalled at full PWM, Wi-Fi transmitting, END LED on.

| Component | Rail | Idle (mA) | Typical (mA) | Peak / stall (mA) | Where the number comes from |
|---|---|---|---|---|---|
| [ESP32-WROOM-32](https://mischianti.org/2021/03/06/esp32-practical-power-saving-manage-wifi-and-cpu-1) on DOIT DevKit V1 | 3.3 V | 68 | 68 | 240 | Espressif figures: CPU on, radio off = 30–68 mA (I use the top); Wi-Fi transmitting at full power = 240 mA. A DevKit V1 measured about 72 mA with Wi-Fi off, which agrees |
| DevKit power LED | 3.3 V | 2 | 2 | 2 | Estimate |
| 5 × [TCRT5000](https://www.epfl.ch/campus/associations/robopoly/wp-content/uploads/2018/09/TCRT5000.pdf) IR sensors | 3.3 V | 48 | 48 | 48 | IR LED: (3.3 − 1.25 V) ÷ 220 Ω = 9.3 mA. Phototransistor: at most 3.3 V ÷ 10 kΩ = 0.33 mA. Times five. The LEDs are always on, so idle = peak |
| 2 × motor encoders | 3.3 V | 20 | 20 | 20 | Seller gives no figure; assumed 10 mA per encoder (two Hall sensors). Measure to confirm |
| [TB6612FNG](https://www.digikey.at/en/htmldatasheets/production/1857561/0/0/1/2927) logic (VCC) | 3.3 V | 2 | 2 | 2 | Datasheet: 1.1 mA typical, 1.8 mA max |
| Red END LED | 3.3 V | 0 | 0 | 4 | (3.3 − 2.0 V) ÷ 330 Ω = 3.9 mA, lit only at the finish |
| AMS1117 regulator on the DevKit | 5 V | 5 | 5 | 5 | Its own standby (quiescent) current, typical 5 mA |
| Left [N20 motor](https://www.waveshare.com/product/dcgm-n20-12v-en-200rpm.htm) | 9 V | 0 | 200 | 825 | Stall is 1.1 A at 12 V ([Robu](https://st.robu.in/?p=475779) lists 1 A; I use the higher). At 9 V: 1.1 × 9 ÷ 12 = 0.825 A. Typical is an assumption, see Runtime |
| Right N20 motor | 9 V | 0 | 200 | 825 | Same as left |
| TB6612FNG motor side (VM) | 9 V | 0 | 0 | 0 | Standby under 1 µA; no operating figure in the datasheet, so treated as zero |
| Converter standby losses + battery divider | 11.1 V | 10 | 10 | 10 | Sellers give no figure; assumed 10 mA. The 100 kΩ + 22 kΩ divider adds only 0.1 mA |

## Worst-case total at the battery

Each rail's power is divided by its converter's efficiency, then the total is divided by 11.1 V.

| Step | Idle | Typical | Peak |
|---|---|---|---|
| 9 V rail current (motors) | 0 mA | 400 mA | 1,650 mA |
| 5 V rail current (ESP32 board, including all 3.3 V loads) | 145 mA | 145 mA | 321 mA |
| Battery power for 9 V rail (9 V × I ÷ 0.90 XL4015 efficiency) | 0 W | 4.00 W | 16.50 W |
| Battery power for 5 V rail (5 V × I ÷ 0.85 MP1584 efficiency) | 0.85 W | 0.85 W | 1.89 W |
| Standby losses + divider (10 mA × 11.1 V) | 0.11 W | 0.11 W | 0.11 W |
| **Total power from battery** | **0.97 W** | **4.97 W** | **18.50 W** |
| **Battery current (total ÷ 11.1 V)** | **87 mA** | **447 mA** | **1,667 mA** |

## Runtime if the assumptions are wrong

| Scenario | Battery current | Runtime |
|---|---|---|
| Waiting at the start line (idle) | 87 mA | ~9 h |
| Normal running (assumed) | 447 mA | ~107 min |
| Motors draw double (0.4 A each) | 808 mA | ~59 min |
| Everything at peak non-stop (not possible in practice) | 1,667 mA | ~29 min |

## Comparison: LiPo vs Li-ion

Both packs are 3-cell, 11.1 V, from the same supplier, so the comparison is like for like. The choice (LiPo) and the reasons are in the [notes](../docs/task5_power_budget.md#battery-chemistry-why-lipo).

|  | LiPo: Pro-Range 3S 1000 mAh 30C (in BOM) | Li-ion: [Pro-Range NMC 18650 3S1P 2500 mAh 3C](https://stgaws.robu.in/product/pro-range-18650-li-ion-2500mah-11-1v-3s1p-protected-battery-pack-3c) |
|---|---|---|
| Energy stored | 11.1 Wh | 27.75 Wh |
| Weight | ~85 g ([typical for this pack size](https://aws.robu.in/product-category/batteries/batteries-batteries/lithium-polymer-battery-packs/orange-lipo/3-cell-lipo-11-1v-12-6v)) | 172 g |
| Energy density | ~131 Wh/kg | ~161 Wh/kg |
| Max continuous current (C-rating × capacity) | 30 × 1 Ah = 30 A | 3 × 2.5 Ah = 7.5 A |
| Headroom over our 1.67 A peak | ~18× | ~4.5× |
| Size | ~75 × 34 × 17 mm, flat | 70 × 60 × 25 mm |
| Price | ₹1,049 | ₹799 |
| Cost per Wh | ~₹95 | ~₹29 |
| Runtime at 447 mA (80% usable) | ~107 min | ~268 min |
| Built-in protection | None; needs the balance charger and low-voltage alarm (both in BOM) | BMS built in |
