# Arduino Sentry: Radar & Auto-Launcher System 🎯

An automated, multitasking Arduino sentry system that sweeps a radar to detect targets using an ultrasonic sensor. Once a target is locked, it triggers an alarm and automatically aims and fires a launcher mechanism at the target's exact angle.

Unlike basic Arduino scripts, this project utilizes a **Non-Blocking State Machine Architecture**. By completely eliminating `delay()` calls and utilizing the high-speed `NewPing` library, the Arduino can multitask seamlessly. The radar scanning, target locking, visual/audio alerts, and servo aiming sequences all happen independently and concurrently without the system ever freezing.

---

## 🚀 Features

- **Non-Blocking State Machine**: Handles multiple simultaneous tasks (sweeping, aiming, alerting) using `millis()` instead of `delay()`.
- **Fast & Accurate Tracking**: Uses the `NewPing` library and hardware interrupts for lightning-fast ultrasonic target detection.
- **Auto-Aiming Launcher**: Smoothly aims a secondary servo directly at the angle of detection before executing a simulated firing sequence.
- **Audio-Visual Alarms**: Triggers a customizable LED and Buzzer sequence upon target lock.
- **Easy Configuration**: All critical variables (sweep speed, detection range, limits, angles) are conveniently located at the top of the code for quick tuning.

---

## 🛠️ Hardware Requirements

*   1x Arduino (Uno, Nano, or Mega)
*   2x Standard/Micro Servos (e.g., SG90)
    *   *One for the radar sweep, one for the launcher pitch.*
*   1x HC-SR04 Ultrasonic Distance Sensor
*   1x LED (with appropriate ~220Ω resistor)
*   1x Active/Passive Buzzer
*   **External 5V Power Supply** (Highly recommended to prevent Arduino brownouts when the dual servos draw current).

---

## 🔌 Wiring Diagram / Pinout

| Component               | Arduino Pin | Notes                                      |
| ----------------------- | :---------- | :----------------------------------------- |
| **Ultrasonic Trig**     | Pin 7       |                                            |
| **Ultrasonic Echo**     | Pin 6       |                                            |
| **Launcher Servo**      | Pin 9       | Ensure PWM compatibility                   |
| **Radar Servo**         | Pin 10      | Ensure PWM compatibility                   |
| **Status LED**          | Pin 3       | Requires resistor                          |
| **Alarm Buzzer**        | Pin 2       |                                            |

> **Hardware Note:** Connect the Ground (GND) of the external 5V power supply to the Ground (GND) of the Arduino so they share a common ground reference.

---

## 💻 Software Setup

1.  **Install the Arduino IDE**: Ensure you have the latest version of the [Arduino IDE](https://www.arduino.cc/en/software) installed.
2.  **Install the NewPing Library**: This code requires the `NewPing` library for the ultrasonic sensor.
    *   Open the Arduino IDE.
    *   Go to `Sketch` -> `Include Library` -> `Manage Libraries...`
    *   Search for **"NewPing"** (by Tim Eckel).
    *   Click **Install**.
3.  **Upload**: Open the `missile_detector.ino` file in the IDE, select your board and COM port, and click **Upload**.

---

## ⚙️ Configuration

You can easily adjust how the sentry behaves by modifying the variables at the top of the `missile_detector.ino` file:

```cpp
// --- Configuration Parameters ---
const int detectionDistance = 15; // Max detection range (cm)
const int minDistance = 5;        // Min detection range (cm)
const int radarLeftLimit = 15;    // Radar sweep left boundary
const int radarRightLimit = 165;  // Radar sweep right boundary
const int readyAngle = 90;        // Default facing forward angle
const int sweepDelay = 30;        // Delay between radar steps (ms)
```

---

## 🧠 How it Works (Under the Hood)

The main `loop()` contains four separate sub-routines that are constantly being polled:

1.  **`updateRadar()`**: Steps the radar servo back and forth by 1 degree. It will halt scanning to focus when a target is locked.
2.  **`checkSonar()`**: Pings the ultrasonic sensor every 40ms. If a target is registered between `minDistance` and `detectionDistance` twice consecutively, a "target lock" is declared.
3.  **`updateAlert()`**: An independent timer that flashes the LED and sounds the buzzer 3 times to warn the intruder.
4.  **`updateLauncher()`**: A 5-stage state machine (`IDLE` -> `AIMING` -> `FIRING_DOWN` -> `FIRING_UP` -> `RETURNING`). It smoothly rotates the launcher servo to the recorded `lockedAngle`, dips down to "fire", and returns to the `readyAngle`.

## 📜 License
DO not feel free to use, modify, and distribute this project. 
