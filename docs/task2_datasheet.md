# Task 2 — Datasheet Literacy: TB6612FNG Motor Driver

Component: the TB6612FNG dual motor driver module (Probots PRO984) from the [BOM](../BOM/BOM.md).

## Parameters that constrain the design

- **Motor-supply voltage:** the selected PRO984 module is advertised for 4.5–10 V. The 9.0 V regulator keeps its supply within that specified range, even though the battery is a 3S pack.
- **Logic voltage:** the chip's logic-supply range is 2.7–5.5 V, so the ESP32's 3.3 V logic is appropriate.
- **Motor current:** treat continuous current and short-duration peak current as different limits. The 3.2 A peak figure is not a continuous operating rating; the motor's exact stall current must be checked before you finalize the driver choice.

## Condition that would damage it

**Damage-prevention condition:** applying more than the module's specified 10 V motor supply could damage it. The regulated 9.0 V rail avoids that condition under normal operation; bulk capacitance and motor-noise suppression provide additional protection against transients.

Sources: [TB6612FNG datasheet, Toshiba (via Digi-Key)](https://www.digikey.at/en/htmldatasheets/production/1857561/0/0/1/2927) · [PRO984 module listing, Probots](https://probots.co.in/tb6612fng-dual-channel-dc-motor-driver-module.html)
