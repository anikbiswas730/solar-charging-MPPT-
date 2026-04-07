# Design & Implementation of an MPPT Solar Charge Controller

This repository contains the hardware design, schematic details, and embedded C/C++ firmware for an Arduino Nano-based Maximum Power Point Tracking (MPPT) Solar Charge Controller. It was developed as a project for the EEE-316 Power Electronics Laboratory.

## Background and Motivation
A solar panel's output changes continuously with irradiance, temperature, and load. Connecting a 100 W solar panel directly to a 12 V battery can result in over 57% of the available power being wasted. This project implements an MPPT charge controller to continuously adjust the operating point of the panel, ensuring the maximum possible power is extracted at every instant. 

## ✨ Key Features
* **MPPT Algorithm:** Implements the Perturb & Observe (P&O) method to dynamically track the maximum power point by perturbing the PWM duty cycle.
* **DC-DC Buck Converter:** Utilizes a synchronous buck converter stage for efficient voltage step-down conversion.
* **Multi-Stage Battery Charging:** Incorporates a finite state machine to manage three lead-acid charging stages: Off, Bulk, and Float.
* **Real-Time Monitoring:** Integrates voltage, current, and power measurements, displaying real-time system status on a 20x4 I2C LCD.
* **Load Protection:** Includes a Low Voltage Disconnect (LVD) threshold (< 11.5 V) to prevent deep discharge of the battery.

##  Hardware Specifications
The system is designed for a 50 W (12-21 V) solar panel and a 12 V sealed lead-acid battery. Key components include:
* **Microcontroller:** Arduino Nano (ATmega328P) operating at 16 MHz.
* **Power Switching:** Three IRFZ44Z N-channel MOSFETs driven by an IR2104 half-bridge gate driver.
* **Energy Storage (Converter):** 33 µH toroidal power inductor and 220 µF electrolytic output filter capacitor.
* **Sensing:** * ACS712-05B Hall-effect current sensor for solar current (0-5 A range).
  * Resistive voltage dividers to scale solar and battery voltages for the 10-bit ADC.
* **Protection:** P6KE36CA TVS diodes for transient voltage suppression and an isolated load control circuit using a 2N2222A transistor.

##  Software Architecture
The firmware operates on a cooperative multitasking pattern, executing the following loop:
1. **ADC Averaging:** Reads and averages eight samples per cycle to minimize sensor noise.
2. **Charger State Machine:** Updates the charging state between Off, On, Bulk, and Float based on voltage and power thresholds.
3. **Serial & Display Update:** Refreshes debug outputs, LED indicators, and the LCD.

A Timer1 hardware interrupt generates a steady 50 kHz PWM signal to drive the buck converter.

##  System Performance
* **Efficiency:** The system achieves a theoretical buck converter efficiency of approximately 99.5% due to low MOSFET conduction and switching losses.
* **Accuracy:** Calibrated sensor measurements maintain an error rate of less than 2% for both voltage and current.
* **Stability:** The P&O algorithm produces minimal steady-state oscillation (±0.2 W) by using a fine 1% PWM perturbation step.

## Project Team
* **Abid Abdullah** (Roll: 1906107)
* **Liakot Omor Rihan** (Roll: 1906120)
* **Shahriar Peal** (Roll: 1906124)
* **Anik Biswas** (Roll: 1906125)

**Institution:** Department of Electrical and Electronic Engineering, Bangladesh University of Engineering and Technology (BUET).
