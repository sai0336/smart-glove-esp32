# Smart Glove — ESP32 BLE Wireless Mouse

A wearable glove that replaces a physical mouse. You move your hand, the cursor moves. No surface needed.

Built this because I wanted to see if hand motion could be used as a proper input device — not just a demo, but something that actually works for daily use.

---

## How it works

An ESP32 reads orientation and motion data from an MPU-6050 gyroscope over I2C. That data gets processed into cursor movement and sent wirelessly to a PC or Android phone over BLE, where it shows up as a standard HID mouse — no drivers needed.

For clicks, copper tape strips on the fingers act as capacitive contacts. Touch two fingers together and it registers as a click.

Runs on a 2S lithium battery pack tucked into the glove.

---

## Modes

| Mode | Use |
|---|---|
| Air Mouse | General desktop cursor control |
| 3D Lab | Navigating 3D software viewports |
| Map Explorer | Map panning and zooming |

---

## Components

| Part | Purpose |
|---|---|
| ESP32 | Main controller, BLE transmission |
| MPU-6050 | 6-axis gyroscope and accelerometer |
| Copper tape | Capacitive click contacts |
| 2S Li-ion battery | Power supply |

---

## Pin Connections

| MPU-6050 | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

---

## Libraries

```
MPU6050 by Electronic Cats
ESP32 BLE Arduino (built into ESP32 board package)
```

---

## How to flash

1. Install ESP32 board package in Arduino IDE
2. Install MPU6050 library
3. Open `smart_glove.ino`
4. Select board: ESP32 Dev Module
5. Upload and pair via Bluetooth — shows up as a mouse

---

## Demonstrated at

Tech-Mania 2K26 — inter-college technical competition, Patkar-Varde College, Mumbai

---

Built by [Sai Chavan](https://github.com/sai0336)
