# Automated Water Treatment Sequencer (ESP32-S2)

An embedded firmware implementation for an automated, multi-stage water treatment sequencer designed for the Espressif ESP32-S2 microcontroller. The system features a modular architecture composed of an Application Layer, a Process API Layer, and a Hardware Abstraction Layer (HAL), executing a strict time-sequenced batch process with real-time safety interlocks.

---

## 1. Transparency & Tool Disclosure

> **Candidate Disclosure Note:**  
> In the following discloses the tools and assistance used during the completion of this task:
> 
> * **Core Engineering & Logic:**  
>   The functional state sequencing, hardware design abstractions (HAL), peripheral pin selections, PWM frequency derivations for the 80 RPM stepper motor, and FreeRTOS task concurrency structure were designed and verified based on the task requirements and technical specifications of the ESP32-S2.
> * **Use of Generative AI (LLMs / GPTs):**  
>   Per the prompt's explicit permission to utilize AI for documentation, an LLM was utilized as a documentation assistant to format, polish, and structure this `README.md` and review Markdown clarity. It was also used to double-check peripheral struct syntax compatibility across the ESP32 Arduino Core version differences.
> * **Integrity Statement:**  
>   All engineering trade-offs, architecture decisions, and code flows are fully understood and can be walked through, tested, or modified live during a technical interview.

---

## 2. Firmware Architecture

The firmware separates concerns into three distinct layers to ensure testability, portability, and safety:

```
+-------------------------------------------------------------+
|                     APPLICATION LAYER                       |
|           (Main Sequencer Task / Recipe Execution)           |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                     PROCESS API LAYER                       |
|    (Recipe Structs, Emergency Shutdown, Sequence Actions)   |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                HARDWARE ABSTRACTION LAYER (HAL)             |
|  - GPIO Control (Pumps & Actuators)                         |
|  - LEDC Peripheral Driver (Precision Stepper PWM)           |
|  - Digital Input Filtering (Tank Float Sensors)             |
+-------------------------------------------------------------+
```

### Layer Breakdown

1. **Hardware Abstraction Layer (`hal_water_system.h`, `hal_water_system.cpp`)**
   * Encapsulates low-level hardware interactions (GPIO register writes and LEDC timers).
   * Decouples application logic from physical pin allocations.
   * Provides non-blocking hardware control interfaces.

2. **API Layer (`water_treatment_api.h`, `water_treatment_api.cpp`)**
   * Exposes high-level interfaces (`api_init`, `api_emergency_stop_all`).
   * Defines sequence data structures (`treatment_step_t`) linking human-readable step names, execution times, and callback function pointers.

3. **Application Sequencer (`WaterTreatmentESP32.ino`)**
   * Executes as an independent, deterministic FreeRTOS task (`waterTreatmentSequencerTask`).
   * Steps through the 7-stage recipe using non-blocking periodic delays (`vTaskDelay`).
   * Concurrently samples tank level sensors during active stages to trigger immediate safety interlocks.

---

## 3. Process Sequence (Cycle 1 Specification)

The sequence strictly mirrors the engineering cycle specification:

| Step # | Process Description | Active Duration | Transition Delay | Controlled Hardware |
| :---: | :--- | :---: | :---: | :--- |
| **1** | Waste Water Pumping to 5000L Tank | 720 s (12 min) | 1 s | `PUMP_WASTE_TO_5000L` |
| **2** | Normal Water to Coagulation Tank | 264 s (4.4 min) | 1 s | `PUMP_NORMAL_TO_COAGULATION` |
| **3** | Normal Water to Flocculant Tank | 58 s | 1 s | `PUMP_NORMAL_TO_FLOCCULANT` |
| **4** | Waste Water Pumping to Mixing Tank | 264 s (4.4 min) | 1 s | `PUMP_WASTE_TO_MIXING` |
| **5** | Powder Dosing Start | 60 s (1 min) | 1 s | `ACTUATOR_POWDER_DOSER` |
| **6** | Coagulant Mixing (Stepper @ 80 RPM) | 600 s (10 min) | 3 s | Stepper Driver via LEDC PWM |
| **7** | DC Dosing Pump Start | 60 s (1 min) | 1 s | `PUMP_DC_DOSING` |

---

## 4. Hardware Pinout (ESP32-S2)

### Relay / Digital Outputs (Pumps & Actuators)
* `GPIO 4`: Waste Water Pump to 5000L Tank
* `GPIO 5`: Normal Water Pump to Coagulation Tank
* `GPIO 6`: Normal Water Pump to Flocculant Tank
* `GPIO 7`: Waste Water Pump to Mixing Tank
* `GPIO 8`: DC Dosing Pump
* `GPIO 9`: Powder Doser Actuator
* `GPIO 10`: Mixer Motor Auxiliary Actuator

### Stepper Motor Interface (Coagulant Mixer)
* `GPIO 11`: Stepper Pulse / Step Pin (`LEDC_CHANNEL_0`)
* `GPIO 12`: Stepper Direction Pin
* `GPIO 13`: Stepper Enable Pin (Active LOW)

> **RPM to Frequency Calculation:**  
> Standard 1.8° Hybrid Stepper = 200 full steps/rev.  
> Using 1/16 microstepping driver: $200 \times 16 = 3200 \text{ steps/rev}$.  
> Target speed: $80 \text{ RPM} = \frac{80}{60} \text{ rev/s} \approx 1.333 \text{ rev/s}$.  
> Target frequency: $1.333 \times 3200 \approx 4267 \text{ Hz}$.  
> The HAL initializes an ESP32-S2 LEDC hardware timer running at 4267 Hz with 50% duty cycle, ensuring jitter-free pulse generation without CPU cycle hogging.

### Sensor Inputs (Active-LOW Float Switches)
* `GPIO 14`: 5000L Tank High Level Sensor (Interlock)
* `GPIO 15`: 5000L Tank Low Level Sensor
* `GPIO 16`: Mixing Tank High Level Sensor

---

## 5. Build and Installation Instructions

### Prerequisites
1. [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. ESP32 Arduino Core ($\ge$ v2.0.x)
   * Add to Board Manager: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

### Compilation & Flashing
1. Clone or download the repository into your Arduino sketchbook directory:
   ```bash
   git clone <repo-url> WaterTreatmentESP32
   ```
2. Open `WaterTreatmentESP32.ino` in Arduino IDE.
3. Select board: **Tools > Board > esp32 > ESP32S2 Dev Module**.
4. Configure USB settings:
   * **USB CDC On Boot:** `Enabled` (if using onboard native USB for Serial monitoring)
   * **Upload Mode:** `Internal USB` or `UART0` (depending on your development board)
5. Connect your ESP32-S2 via USB and click **Upload**.
6. Open the Serial Monitor at **115200 baud** to view real-time state transitions and diagnostic logs.

---

## 6. Safety Features & Edge Handling

* **Non-Blocking Architecture:** Process delays run via FreeRTOS `vTaskDelay()`, allowing the system to monitor sensors and accept emergency commands concurrently.
* **Overflow Protection:** Step 1 actively monitors the `5000L_HIGH` float switch; if high liquid level is reached before the 12-minute timer elapses, the pump terminates immediately to prevent tank overflow.
* **Failsafe Default:** On reboot or unhandled assertion, all outputs initialize to `LOW` / Disabled states, ensuring no valves, pumps, or heaters turn on unintentionally.

-----------------------------------------------------------------------------------------------------------------------------------------------------------

Where I used AI assistance:
To keep the project clean and well-structured without spending hours writing repetitive syntax, I used AI (like ChatGPT/Gemini) as a helper tool. Specifically, I used it to:

Format the markdown documentation and structure the technical tables so they look professional.

Cross-check the exact ESP-IDF syntax for the LEDC hardware timer and GPIO driver configurations.

Set up the initial file boilerplates (.h and .c files) so I could focus directly on writing the sequencer logic.