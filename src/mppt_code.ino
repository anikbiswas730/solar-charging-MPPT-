#include "TimerOne.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define ENABLE_DATALOGGER 0
#define LOAD_ALGORITHM 1

// ADC Channel Assignments
#define SOL_AMPS_CHAN 1     // ACS712 current sensor on A1
#define SOL_VOLTS_CHAN 0    // Solar voltage divider on A0
#define BAT_VOLTS_CHAN 2    // Battery voltage divider on A2
#define AVG_NUM 8           // Number of ADC samples to average

// Voltage Scaling Factors
#define SOL_VOLTS_SCALE 0.029296875 // (5V/1023) * 6 (divider ratio)
#define BAT_VOLTS_SCALE 0.029296875

// PWM Configuration
#define PWM_PIN 6           // Timer1 PWM output
#define PWM_ENABLE_PIN 5    // IR2104 SD pin (HIGH enable)
#define PWM_FULL 1023       // 100% duty cycle raw value
#define PWM_MAX 100         // Maximum PWM percentage
#define PWM_MIN 60          // Minimum PWM percentage
#define PWM_START 90        // Initial PWM percentage
#define PWM_INC 1           // Perturbation step size (%)

// Boolean Aliases
#define TRUE 1
#define FALSE 0
#define ON TRUE
#define OFF FALSE

// MOSFET Control Macros
#define TURN_ON_MOSFETS digitalWrite(PWM_ENABLE_PIN, HIGH)
#define TURN_OFF_MOSFETS digitalWrite(PWM_ENABLE_PIN, LOW)

// Timer Constants
#define ONE_SECOND 50000 // Timer1 ticks per second at 50 kHz

// Voltage Power Thresholds
#define LOW_SOL_WATTS 5.00   // Below this: stay in ON state, max PWM
#define MIN_SOL_WATTS 0.10   // Below this: turn off charging
#define MIN_BAT_VOLTS 11.00  // Minimum battery voltage to start charging
#define MAX_BAT_VOLTS 14.10  // Over-voltage threshold
#define BATT_FLOAT 13.60     // Float charge voltage setpoint
#define HIGH_BAT_VOLTS 13.00 // Threshold to go from off to float
#define LVD 11.5             // Low Voltage Disconnect threshold
#define OFF_NUM 9            // Startup delay count cycles

// I/O Pin Assignments
#define LED_YELLOW 12
#define LED_GREEN 11
#define LED_RED 10
#define LOAD_PIN 4           // Load MOSFET gate (via Q1)
#define BACK_LIGHT_PIN 3     // Backlight timeout input

// Global Variables
float AcsValue, ss = 0.0;

// Custom LCD battery level characters (0% to 100%)
byte battery_icons[6][8] = {
  {0b01110, 0b11011, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111},
  {0b01110, 0b11011, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111, 0b11111},
  {0b01110, 0b11011, 0b10001, 0b10001, 0b10001, 0b11111, 0b11111, 0b11111},
  {0b01110, 0b11011, 0b10001, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111},
  {0b01110, 0b11011, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111},
  {0b01110, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111}
};

// Custom solar panel icon
#define SOLAR_ICON 6
byte solar_icon[8] = {
  0b11111, 0b10101, 0b11111, 0b10101,
  0b11111, 0b10101, 0b11111, 0b00000
};

// Custom PWM icon
#define PWM_ICON 7
byte PWM_icon[8] = {
  0b11101, 0b10101, 0b10101, 0b10101,
  0b10101, 0b10101, 0b10101, 0b10111
};

byte backslash_char[8] = {
  0b10000, 0b10000, 0b01000, 0b01000,
  0b00100, 0b00100, 0b00010, 0b00010
};

// Measurement Variables
float sol_amps;
float sol_volts;
float bat_volts;
float sol_watts;
float old_sol_watts = 0;

unsigned int seconds = 0;
unsigned int prev_seconds = 0;
unsigned int interrupt_counter = 0;
unsigned long time = 0;

int delta = PWM_INC;  // Current perturbation direction
int pwm = 0;          // Current PWM duty cycle (%)
int back_light_pin_State = 0;
boolean load_status = false;

// Charger state machine states
enum charger_mode { off, on, bulk, bat_float } charger_state;

// LCD: I2C address 0x27, 20 columns, 4 rows
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Setup
void setup() {
  pinMode(PWM_ENABLE_PIN, OUTPUT);
  TURN_OFF_MOSFETS; // Safe startup: MOSFETs off
  charger_state = off;

  // Initialize LCD and load custom characters
  lcd.init();
  lcd.backlight();
  for (int batchar = 0; batchar < 6; ++batchar) {
    lcd.createChar(batchar, battery_icons[batchar]);
  }
  lcd.createChar(PWM_ICON, PWM_icon);
  lcd.createChar(SOLAR_ICON, solar_icon);
  lcd.createChar('\\', backslash_char);

  // LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  // Timer1: 50 kHz PWM, attach 1-second interrupt
  Timer1.initialize(20); // 20us period = 50 kHz
  Timer1.pwm(PWM_PIN, 0);
  Timer1.attachInterrupt(callback);

  Serial.begin(9600);
  pwm = PWM_START;

  pinMode(BACK_LIGHT_PIN, INPUT);
  pinMode(LOAD_PIN, OUTPUT);
  digitalWrite(LOAD_PIN, LOW);

  // LCD header row
  lcd.setCursor(0, 0); lcd.print("SOL");
  lcd.setCursor(4, 0); lcd.write(SOLAR_ICON);
  lcd.setCursor(8, 0); lcd.print("BAT");
}

// Main Loop
void loop() {
  read_data();     // Sample all sensors
  run_charger();   // Execute MPPT state machine
  print_data();    // Serial output for debugging
  load_control();  // Switch load on/off
  led_output();    // Update status LEDs
  lcd_display();   // Refresh LCD
}

// ADC Averaging
int read_adc(int channel) {
  int sum = 0;
  for (int i = 0; i < AVG_NUM; i++) {
    sum += analogRead(channel);
    delayMicroseconds(50);
  }
  return (sum / AVG_NUM);
}

// ACS712 Current Reading
// Maps ADC value (510=0A, 580=5A) to current in amperes
float read_adccc() {
  float AcsValue, ss = 0.0;
  for (int i = 0; i < AVG_NUM; i++) {
    AcsValue = analogRead(A1);
    ss += mapf(AcsValue, 510, 580, 0.0, 5.0);
    delayMicroseconds(50);
  }
  // Optional Serial prints from original code
  // Serial.println(AcsValue);
  // Serial.println(ss / AVG_NUM);
  return (ss / AVG_NUM);
}

// Read All Sensor Data
void read_data(void) {
  sol_amps = read_adccc();
  sol_volts = read_adc(SOL_VOLTS_CHAN) * SOL_VOLTS_SCALE;
  bat_volts = read_adc(BAT_VOLTS_CHAN) * BAT_VOLTS_SCALE;
  sol_watts = sol_amps * sol_volts;
}

// Timer1 ISR: increment seconds counter
void callback() {
  if (interrupt_counter++ > ONE_SECOND) {
    interrupt_counter = 0;
    seconds++;
  }
}

// Set PWM Duty Cycle with Clamping
void set_pwm_duty(void) {
  if (pwm > PWM_MAX) pwm = PWM_MAX;
  else if (pwm < PWM_MIN) pwm = PWM_MIN;

  if (pwm < PWM_MAX) {
    Timer1.pwm(PWM_PIN, (PWM_FULL * (long)pwm / 100), 20);
  } else {
    Timer1.pwm(PWM_PIN, (PWM_FULL - 1), 20); // Avoid 100% (keep bootstrap alive)
  }
}

// MPPT Charger State Machine
void run_charger(void) {
  static int off_count = OFF_NUM;

  switch (charger_state) {
    case on:
      if (sol_watts < MIN_SOL_WATTS) {
        charger_state = off; off_count = OFF_NUM; TURN_OFF_MOSFETS;
      } else if (bat_volts > (BATT_FLOAT - 0.1)) {
        charger_state = bat_float;
      } else if (sol_watts < LOW_SOL_WATTS) {
        pwm = PWM_MAX; set_pwm_duty(); // Low light: max duty
      } else {
        // Transition to bulk with initial PWM estimate
        pwm = ((bat_volts + 10) / (sol_volts + 10)) + 5; 
        charger_state = bulk;
      }
      break;

    case bulk:
      if (sol_watts < MIN_SOL_WATTS) {
        charger_state = off; off_count = OFF_NUM; TURN_OFF_MOSFETS;
      } else if (bat_volts > BATT_FLOAT) {
        charger_state = bat_float;
      } else if (sol_watts < LOW_SOL_WATTS) {
        charger_state = on; TURN_ON_MOSFETS;
      } else {
        // P&O MPPT: compare current power with previous power
        if (old_sol_watts >= sol_watts) {
          delta = -delta; // Power decreased: reverse direction
        }
        pwm += delta;
        old_sol_watts = sol_watts;
        set_pwm_duty();
      }
      break;

    case bat_float:
      if (sol_watts < MIN_SOL_WATTS) {
        charger_state = off; off_count = OFF_NUM;
        TURN_OFF_MOSFETS; set_pwm_duty();
      } else if (bat_volts > BATT_FLOAT) {
        TURN_OFF_MOSFETS; pwm = PWM_MAX; set_pwm_duty();
      } else if (bat_volts < BATT_FLOAT) {
        pwm = PWM_MAX; set_pwm_duty(); TURN_ON_MOSFETS;
        if (bat_volts < (BATT_FLOAT - 0.1)) {
          charger_state = bulk;
        }
      }
      break;

    case off:
      TURN_OFF_MOSFETS;
      if (off_count > 0) {
        off_count--;
      } else if ((bat_volts > BATT_FLOAT) && (sol_volts > bat_volts)) {
        charger_state = bat_float; TURN_ON_MOSFETS;
      } else if ((bat_volts > MIN_BAT_VOLTS) && (bat_volts < BATT_FLOAT) && (sol_volts > bat_volts)) {
        charger_state = bulk; TURN_ON_MOSFETS;
      }
      break;

    default:
      TURN_OFF_MOSFETS;
      break;
  }
}

// Load Control
void load_control() {
#if LOAD_ALGORITHM == 0
  // Night mode: load on only if battery above LVD
  load_on(sol_watts < MIN_SOL_WATTS && bat_volts > LVD);
#else
  // Day mode: load on when solar available and battery full
  load_on(sol_watts > MIN_SOL_WATTS && bat_volts > BATT_FLOAT);
#endif
}

void load_on(boolean new_status) {
  if (load_status != new_status) {
    load_status = new_status;
    digitalWrite(LOAD_PIN, new_status ? HIGH : LOW);
  }
}

// Serial Debug Output
void print_data(void) {
  Serial.print(seconds, DEC); Serial.print(" ");
  Serial.print("Charging = ");
  
  if (charger_state == on) Serial.print("on ");
  else if (charger_state == off) Serial.print("off ");
  else if (charger_state == bulk) Serial.print("bulk ");
  else if (charger_state == bat_float) Serial.print("float");
  
  Serial.print(" ");
  Serial.print("pwm = ");
  
  if (charger_state == off) Serial.print(0, DEC);
  else Serial.print(pwm, DEC);
  
  Serial.print(" ");
  Serial.print("Current (panel) = "); Serial.print(sol_amps); Serial.print(" ");
  Serial.print("Voltage (panel) = "); Serial.print(sol_volts); Serial.print(" ");
  Serial.print("Power (panel) = "); Serial.print(sol_watts); Serial.print(" ");
  Serial.print("Battery Voltage = "); Serial.print(bat_volts); Serial.print("\n\r");
}

// LED Status Indicator
void light_led(char pin) {
  static char last_lit;
  if (last_lit == pin) return;
  if (last_lit != 0) digitalWrite(last_lit, LOW);
  digitalWrite(pin, HIGH);
  last_lit = pin;
}

void led_output(void) {
  if (bat_volts > 14.1) light_led(LED_YELLOW);
  else if (bat_volts > 11.9) light_led(LED_GREEN);
  else light_led(LED_RED);
}

// LCD Display Refresh
void lcd_display() {
  static bool current_backlight_state = -1;
  back_light_pin_State = digitalRead(BACK_LIGHT_PIN);
  
  if (current_backlight_state != back_light_pin_State) {
    current_backlight_state = back_light_pin_State;
    if (back_light_pin_State == HIGH) lcd.backlight();
    else lcd.noBacklight();
  }
  
  if (back_light_pin_State == HIGH) time = millis();

  // Column 0: Solar panel measurements
  lcd.setCursor(0, 1); lcd.print(sol_volts); lcd.print("V");
  lcd.setCursor(0, 2); lcd.print(sol_amps); lcd.print("A");
  lcd.setCursor(0, 3); lcd.print(sol_watts); lcd.print("W ");

  // Column 8: Battery info
  lcd.setCursor(8, 1); lcd.print(bat_volts);
  lcd.setCursor(8, 2);
  if (charger_state == on) lcd.print("on   ");
  else if (charger_state == off) lcd.print("off  ");
  else if (charger_state == bulk) lcd.print("bulk ");
  else if (charger_state == bat_float) { lcd.print("float"); }

  // Battery icon based on SOC estimate
  int pct = 100.0 * (bat_volts - 11.3) / (12.7 - 11.3);
  if (pct < 0) pct = 0;
  else if (pct > 100) pct = 100;

  lcd.setCursor(12, 0); lcd.print((char) (pct * 5 / 100));
  
  // Battery percentage
  lcd.setCursor(8, 3);
  pct = pct - (pct % 10);
  lcd.print(pct); lcd.print("% ");

  // PWM display
  lcd.setCursor(15, 0); lcd.print("PWM");
  lcd.setCursor(19, 0); lcd.write(PWM_ICON);
  lcd.setCursor(15, 1); lcd.print("    ");
  lcd.setCursor(15, 1);
  if (charger_state == off) lcd.print(0);
  else lcd.print(pwm);
  lcd.print("%");

  // Load status
  lcd.setCursor(15, 2); lcd.print("Load");
  lcd.setCursor(15, 3);
  if (load_status) lcd.print("On ");
  else lcd.print("Off");

  spinner();
  backLight_timer();
}

// Backlight Auto-Off (15 s)
void backLight_timer() {
  if ((millis() - time) <= 15000) lcd.backlight();
  else lcd.noBacklight();
}

// Spinner Animation
void spinner(void) {
  static int cspinner;
  static char spinner_chars[] = { '*', '*', '*', ' ', ' ' };
  lcd.print(spinner_chars[cspinner % sizeof(spinner_chars)]);
  cspinner++;
}

// Float-Point Map Utility
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
