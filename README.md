

# Design & Implementation of an MPPT Solar Charge Controller

[cite_start]This repository contains the hardware design, schematic details, and embedded C/C++ firmware for an Arduino Nano-based Maximum Power Point Tracking (MPPT) Solar Charge Controller[cite: 44, 45]. [cite_start]It was developed as a project for the EEE-316 Power Electronics Laboratory[cite: 7].

##  Background and Motivation
[cite_start]A solar panel's output changes continuously with irradiance, temperature, and load[cite: 37]. [cite_start]Connecting a 100 W solar panel directly to a 12 V battery can result in over 57% of the available power being wasted[cite: 69, 74]. [cite_start]This project implements an MPPT charge controller to continuously adjust the operating point of the panel, ensuring the maximum possible power is extracted at every instant[cite: 39]. 

##  Key Features
* [cite_start]**MPPT Algorithm:** Implements the Perturb & Observe (P&O) method to dynamically track the maximum power point by perturbing the PWM duty cycle[cite: 45, 82, 85].
* [cite_start]**DC-DC Buck Converter:** Utilizes a synchronous buck converter stage for efficient voltage step-down conversion[cite: 46, 105].
* [cite_start]**Multi-Stage Battery Charging:** Incorporates a finite state machine to manage three lead-acid charging stages: Off, Bulk, and Float[cite: 48, 269].
* [cite_start]**Real-Time Monitoring:** Integrates voltage, current, and power measurements, displaying real-time system status on a 20x4 I2C LCD[cite: 47].
* [cite_start]**Load Protection:** Includes a Low Voltage Disconnect (LVD) threshold (< 11.5 V) to prevent deep discharge of the battery[cite: 121].

## 🛠️ Hardware Specifications
[cite_start]The system is designed for a 50 W (12-21 V) solar panel and a 12 V sealed lead-acid battery[cite: 44, 1082, 1083]. Key components include:
* [cite_start]**Microcontroller:** Arduino Nano (ATmega328P) operating at 16 MHz[cite: 176, 178].
* [cite_start]**Power Switching:** Three IRFZ44Z N-channel MOSFETs driven by an IR2104 half-bridge gate driver[cite: 185, 194].
* [cite_start]**Energy Storage (Converter):** 33 µH toroidal power inductor and 220 µF electrolytic output filter capacitor[cite: 210, 212].
* [cite_start]**Sensing:** * ACS712-05B Hall-effect current sensor for solar current (0-5 A range)[cite: 199].
  * [cite_start]Resistive voltage dividers to scale solar and battery voltages for the 10-bit ADC[cite: 179, 202].
* [cite_start]**Protection:** P6KE36CA TVS diodes for transient voltage suppression and an isolated load control circuit using a 2N2222A transistor[cite: 216, 223, 224].

##  Software Architecture
[cite_start]The firmware operates on a cooperative multitasking pattern, executing the following loop[cite: 238]:
1. [cite_start]**ADC Averaging:** Reads and averages eight samples per cycle to minimize sensor noise[cite: 247].
2. [cite_start]**Charger State Machine:** Updates the charging state between Off, On, Bulk, and Float based on voltage and power thresholds[cite: 240, 269].
3. [cite_start]**Serial & Display Update:** Refreshes debug outputs, LED indicators, and the LCD[cite: 241, 243, 244].

[cite_start]A Timer1 hardware interrupt generates a steady 50 kHz PWM signal to drive the buck converter[cite: 182, 245].

##  System Performance
* [cite_start]**Efficiency:** The system achieves a theoretical buck converter efficiency of approximately 99.5% due to low MOSFET conduction and switching losses[cite: 1157, 1159].
* [cite_start]**Accuracy:** Calibrated sensor measurements maintain an error rate of less than 2% for both voltage and current[cite: 1142].
* [cite_start]**Stability:** The P&O algorithm produces minimal steady-state oscillation (±0.2 W) by using a fine 1% PWM perturbation step[cite: 90, 1150].

##  Project Team
* [cite_start]**Abid Abdullah** (Roll: 1906107) [cite: 12]
* [cite_start]**Liakot Omor Rihan** (Roll: 1906120) [cite: 12]
* [cite_start]**Shahriar Peal** (Roll: 1906124) [cite: 12]
* [cite_start]**Anik Biswas** (Roll: 1906125) [cite: 12]

[cite_start]**Institution:** Department of Electrical and Electronic Engineering, Bangladesh University of Engineering and Technology (BUET)[cite: 14, 15].
