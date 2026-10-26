# Arya_ProjX

Project X submission of Arya Jain: a line-following, maze-solving robot for LMS / Meshmerize (Techfest 2026-27), built on an ESP32-WROOM-32.

## Repository layout

| Folder | Contents |
|---|---|
| [`BOM/`](BOM) | BOM table ([BOM.md](BOM/BOM.md), [BOM.xlsx](BOM/BOM.xlsx)), [power-block diagram](BOM/power_block_diagram.jpeg), [power budget and battery comparison](BOM/power_budget.md) |
| [`docs/`](docs) | Notes on datasheets, control theory, power budget, wiring, wireless diagnostics, maze analysis and PID tuning |
| [`src/`](src) | Source code and test cases, one folder per program |

## Where each task is

| Task | Files |
|---|---|
| 1. Basic coding | [src/Task1/Task1.cpp](src/Task1/Task1.cpp) |
| 2. Datasheet literacy | [docs/task2_datasheet.md](docs/task2_datasheet.md) |
| 3. PID control | [docs/task3_pid_control.md](docs/task3_pid_control.md), [src/Task3_PD/Task3_PD.ino](src/Task3_PD/Task3_PD.ino) |
| 4. BOM preparation | [BOM/BOM.md](BOM/BOM.md), [BOM/BOM.xlsx](BOM/BOM.xlsx), [BOM/power_block_diagram.jpeg](BOM/power_block_diagram.jpeg) |
| 5. Power budget & battery selection | [BOM/power_budget.md](BOM/power_budget.md) (tables), [docs/task5_power_budget.md](docs/task5_power_budget.md) (notes) |
| 6. Electronics & hardware | [docs/task6_wiring.md](docs/task6_wiring.md) |
| 7. Wireless sensor diagnostics | [docs/task7_wireless_diagnostics.md](docs/task7_wireless_diagnostics.md) |
| 8. Maze-solving algorithms | [docs/task8_maze_solving.md](docs/task8_maze_solving.md), [src/Task8_MazeSolver/Task8_MazeSolver.ino](src/Task8_MazeSolver/Task8_MazeSolver.ino) |
| 9. LMS implementation | [src/Task9_LineFollower/Task9_LineFollower.ino](src/Task9_LineFollower/Task9_LineFollower.ino), [docs/task9_pid_tuning.md](docs/task9_pid_tuning.md) |

## How to run the code

### Task 1 (C++ on a computer)

```bash
g++ src/Task1/Task1.cpp -o task1
./task1
```

- `linePosition()` takes five readings at positions −2 to +2 and returns their weighted centroid. If all five are zero, it sets `lost` and returns ±2 on the side where the line was last seen (0 if never seen).
- `limit()` clamps a motor command to ±maximum while keeping its sign, so a reverse command stays reverse.

Each call in `main()` prints its result next to the expected value:

```text
centred       -> +0.000  lost=0   (expect  0.000)
slightly left -> -0.375  lost=0   (expect -0.375 = -600/1600)
far right     -> +1.667  lost=0   (expect +1.667 = 2500/1500)
no line       -> +2.000  lost=1   (expect +2, last seen on the right)
limit(300, 255)  = 255   (expect 255)
limit(-400, 255) = -255  (expect -255, still reverse)
limit(120, 255)  = 120   (expect 120, unchanged)
```

### ESP32 sketches (Tasks 3, 8 and 9)

1. Install the Arduino IDE and, in Boards Manager, the **esp32 by Espressif Systems** package.
2. Select the board **DOIT ESP32 DEVKIT V1**.
3. Open the `.ino` file. Each sketch already sits in a folder of the same name, as the Arduino IDE requires.
4. Upload, then open the Serial Monitor at **115200 baud**.

| Sketch | With no robot attached | Expected output |
|---|---|---|
| [Task9_LineFollower](src/Task9_LineFollower/Task9_LineFollower.ino) | `RUN_TESTS = true` (default) runs 13 sample sensor inputs with the motors off | The table in [docs/task9_pid_tuning.md](docs/task9_pid_tuning.md#test-results), e.g. centred line → L 150, R 150; line far right → L 250, R 50 |
| [Task8_MazeSolver](src/Task8_MazeSolver/Task8_MazeSolver.ino) | A simulated robot explores the example maze, drawn as text at the top of the sketch | Dry run 450 cm; route S → A → G → E = 150 cm; replay STRAIGHT, STRAIGHT, RIGHT |
| [Task3_PD](src/Task3_PD/Task3_PD.ino) | Needs the robot: the Task 3 PD line follower with fixed calibration levels | Streams sensors, position, error and PWM every loop |

**To drive the robot with Task 9:** set `RUN_TESTS = false` and upload. For the first 5 s, slide the sensor bar across the line and the floor (calibration); the robot starts 2 s later.

## Assumptions

- **Motor driver:** Appendix A specifies an L298N; the BOM uses a TB6612FNG ([why](BOM/BOM.md#component-choice-motor-driver)). Both use the same six control pins; the TB6612FNG also uses STBY.
- **Pins:** as in Appendix A, except IN4 (BIN2). Appendix A puts it on GPIO 11, which is wired to the ESP32's flash chip, so it uses GPIO 19. GPIO 23 drives STBY. Full pin table: [docs/task6_wiring.md](docs/task6_wiring.md#pin-connections).
- **Line polarity:** Appendix A says a higher voltage means a darker surface (the line). If the sensors read the line lower than the floor, set `WHITE_LINE = true` in Task 9.
- **Sensor S5** stays on GPIO 25 as in Appendix A. GPIO 25 is ADC2, which does not work while Wi-Fi is on, so Wi-Fi stays off.
- **Power figures** come from datasheets and supplier pages, plus stated estimates where none exist (encoder current, motor running current, converter efficiency). Each is marked in [BOM/power_budget.md](BOM/power_budget.md).
- **Task 8 example maze** is drawn on a 30 cm grid.

## Limitations

- All code was compiled and run with g++ against a minimal stand-in for the Arduino functions. It has not yet been built with the ESP32 toolchain or run on the robot.
- In Task 8, the robot functions (turning, line following to the next junction, odometry) drive a simulated maze. On the robot they would use the Task 9 line follower and the wheel encoders.
- The Task 8 dry run ends at the END box, so the replayed route is the shortest among the paths explored by then.
- The PID gains in Task 9 are starting values to be tuned on the robot.
- Task 7 (Bluetooth diagnostics) is a design only; the task asked for no code.

## References

- Meshmerize problem statement, Techfest 2026-27, and the selection task sheet with Appendix A (mock hardware specification)
- [ESP32-WROOM-32 datasheet, Espressif](https://documentation.espressif.com/esp32-wroom-32_datasheet_en.pdf)
- [ESP32 current figures from the Espressif datasheet, with DevKit V1 measurements, Mischianti](https://mischianti.org/2021/03/06/esp32-practical-power-saving-manage-wifi-and-cpu-1)
- [ESP32 GPIO summary, ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/peripherals/gpio.html)
- [ESP32 ADC FAQ (ADC2 with Wi-Fi and Bluetooth), Espressif](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/adc.html)
- [BLE connection guide, ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-guides/ble/get-started/ble-connection.html)
- [TB6612FNG datasheet, Toshiba (via Digi-Key)](https://www.digikey.at/en/htmldatasheets/production/1857561/0/0/1/2927)
- [TCRT5000 datasheet, Vishay](https://www.epfl.ch/campus/associations/robopoly/wp-content/uploads/2018/09/TCRT5000.pdf)
- [AMS1117 datasheet, Advanced Monolithic Systems](https://hirokun.jp/AMS1117.pdf)
- [N20 12 V 200 RPM encoder motor, Robu](https://st.robu.in/?p=475779) and [Waveshare](https://www.waveshare.com/product/dcgm-n20-12v-en-200rpm.htm)
- [XL4015 buck module, Components101](https://components101.com/modules/xl4015-dc-dc-converter-module)
- [Mini MP1584 buck module, Robu](https://st.robu.in/product/mini-mp1584-dc-dc-3a-adjustable-buck-module-india)
- [3S LiPo packs, Robu](https://aws.robu.in/product-category/batteries/batteries-batteries/lithium-polymer-battery-packs/orange-lipo/3-cell-lipo-11-1v-12-6v) and [Pro-Range NMC 18650 3S1P 2500 mAh pack, Robu](https://stgaws.robu.in/product/pro-range-18650-li-ion-2500mah-11-1v-3s1p-protected-battery-pack-3c)
- [Bluetooth profiles that iOS supports, Apple](https://support.apple.com/HT204387)
- [ESP32 Bluetooth Classic with Arduino, Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-bluetooth-classic-arduino-ide/)
- [ESP32 Bluetooth Classic guide, DeepBlue Embedded](https://deepbluembedded.com/esp32-bluetooth-classic-with-arduino-complete-guide/)
- Supplier links for every BOM item are in [BOM/BOM.md](BOM/BOM.md)
