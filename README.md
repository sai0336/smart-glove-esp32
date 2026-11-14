# Smart Glove — ESP32 BLE Mouse

An ESP32 and MPU-6050 based wireless mouse controlled by hand motion. The firmware implements three modes: **Air Mouse**, **3D Lab**, and **Map Explorer**.

The project was presented at Tech-Mania 2K26, Patkar-Varde College, Mumbai.

## How it works

The ESP32 reads motion data from the MPU-6050 over I2C and sends standard mouse actions to a paired computer using Bluetooth Low Energy (BLE) HID.

The current sketch reads four **active-low digital button inputs** with `INPUT_PULLUP`. A pressed/contact-closed input must connect its GPIO to ground. This sketch does **not** use the ESP32 capacitive touch peripheral; do not treat copper contacts as capacitive sensors without updating the firmware.

## Modes

| Mode | Behavior implemented in firmware |
| --- | --- |
| 1 — Air Mouse | Motion-controlled cursor, left-click/drag, right-click and scroll |
| 2 — 3D Lab | Active/idle toggle, movement/drag control and scroll input |
| 3 — Map Explorer | Motion cursor, panning/dragging, middle-click orbit and scrolling/zoom-style input |

Hold the mode button for longer than one second to cycle through the modes. Mode selection is stored in ESP32 Preferences.

The actual effect of mouse gestures in 3D or map applications depends on each application's mouse bindings.

## Hardware

- ESP32 development board with BLE support
- MPU-6050 accelerometer/gyroscope module
- Four active-low button/contact inputs
- Status/indicator LEDs and suitable current-limiting resistors
- A properly regulated power supply suitable for the exact ESP32 board

### MPU-6050 connections

| MPU-6050 | ESP32 |
| --- | --- |
| VCC | 3.3 V (only if the module supports this voltage) |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Button connections

| Firmware input | ESP32 pin |
| --- | --- |
| Button 1 | GPIO 19 |
| Button 2 | GPIO 18 |
| Button 3 | GPIO 5 |
| Mode button | GPIO 23 |

Each active-low button should close its corresponding GPIO to **GND**.

### Indicator LEDs

| Indicator | GPIO |
| --- | --- |
| Mode 1 | 13 |
| Mode 2 | 12 |
| Mode 3 | 27 |
| Error/idle | 26 |
| Feedback/active | 4 |
| BLE status | 2 |

Check the boot-strapping constraints of your specific ESP32 board when wiring GPIO 2, 5 and 12.

**Power safety:** A 2-cell Li-ion pack is not a regulated 3.3 V supply. Do not connect it directly to an ESP32 3.3 V pin. Select and verify a regulator/board power input for your hardware, and include appropriate cell protection.

## Software requirements

- Arduino IDE with a compatible ESP32 board package
- **ESP32 BLE Mouse** library providing `BleMouse.h` (for example, the T-vK ESP32-BLE-Mouse project)
- Arduino ESP32 core libraries `Wire.h` and `Preferences.h`

The provided sketch talks to the MPU-6050 through `Wire` directly; it does not include or require an MPU6050 wrapper library.

## Upload and pair

1. Open `smart_glove/smart_glove.ino` in Arduino IDE.
2. Select your compatible ESP32 board and port.
3. Install a compatible library that supplies `BleMouse.h` if it is not already installed.
4. Compile and upload the firmware.
5. Pair the device named **Smart Glove Master** over Bluetooth.
6. Confirm each button and movement mode works as intended.

## Repository layout

```text
smart-glove-esp32/
├── smart_glove/
│   └── smart_glove.ino
├── circuit_image.png
├── Screenshot 2026-05-20 095219.png
└── README.md
```

## Verification note

This cleanup reorganizes the original sketch and corrects its documentation; it does **not** independently verify compilation on the currently installed ESP32 core or physical behavior on the glove. Validate the code against your specific board and BLE Mouse library version.

Built by [Sai Chavan](https://github.com/sai0336).
