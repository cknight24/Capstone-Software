#include <SPI.h>    // SPI library
#include <Wire.h>   // i2c library
#include <LiquidCrystal_I2C.h>  // LCD library

// SPI Pins
#define SPI_CLOCK 13  // Cannot be changed
#define SPI_DATA  11  // Cannot be changed

// Shift register pins
#define SHIFT_ENABLE  5 
#define SHIFT_LATCH   4
#define SHIFT_READ    3   // Interrupt pin

// 4:1 Analog MUX pins
#define MUX_S1 12
#define MUX_S0 8

// ADC pins
#define FINE_PIN A3 // Analog pin for precise duty cycle control
#define VOUT_PIN A2 // Analog pin on output of charge controller
#define ALT_CURR_PIN A1 // Analog pin for alternator field winding current

// Load selector pins
#define LS_BAT_ENABLE 7
#define LS_AUX_ENABLE 8

// PWM pins
// Using pins 9 and 10 for PWM since this does not affect the delay() and millis() functions
#define PWM_AC 10 // PWM pin for alternator controller
#define PWM_CC 9  // PWM pin for charge controller

// Settings button
#define SETTINGS_BUTTON 2;

// Shift register mapping
// ----------------------
// Byte 5
// ------
#define CC_OFF_BUTTON       23 // 48
#define CC_BAT_BUTTON       22 // 47
#define CC_HIGH_BUTTON      21 // 46
#define CC_LOW_BUTTON       20 // 45
#define CC_FINE_BUTTON      19 // 44
#define CC_CRSE_BUTTON      18 // 43
#define SS_OFF_BUTTON       17 // 42
// Bit #16 not used
// Byte 4
// ------
#define SS_AUTO_BUTTON      15 // 40
#define SS_MANU_BUTTON      14 // 39
#define SS_AC_BUTTON        13 // 38
#define SS_BIKE_BUTTON      12 // 37
#define SS_AUX_BUTTON       11 // 36
#define CC_UP_BUTTON        10 // 35
#define AC_EN_BUTTON        9 // 34
// Bit #8 not used
// Byte 3
// ------
#define CC_DOWN_BUTTON      7 // 32
#define LS_OFF_BUTTON       6 // 31
#define LS_BAT_BUTTON       5 // 30
#define LS_AUX_BUTTON       4 // 29
// Bit #3 not used
// Bit #2 not used
// Bit #1 not used
// Bit #0 not used
// Byte 2
// ------
#define CC_OFF_LED_RED      23 // 24
#define CC_BAT_LED_GREEN    22 // 23
#define CC_HIGH_LED_GREEN   21 // 22
#define CC_LOW_LED_GREEN    20 // 21
#define CC_FINE_LED_GREEN   19 // 20
#define CC_CRSE_LED_GREEN   18 // 19
#define SS_OFF_LED_RED      17 // 18
// Bit #16 not used
// Byte 1
// ------
#define SS_AUTO_LED_GREEN   15 // 16
#define SS_MANU_LED_GREEN   14 // 15
#define SS_AC_LED_GREEN     13 // 14
#define SS_AC_LED_YELLOW    12 // 13
#define SS_BIKE_LED_GREEN   11 // 12
#define SS_BIKE_LED_YELLOW  10 // 11
#define SS_AUX_LED_GREEN    9 // 10
// Bit #8 not used
// Byte 0
// ------
#define SS_AUX_LED_YELLOW   7 // 8
#define AC_EN_LED_GREEN     6 // 7
// Bit #5 not mapped
#define LS_OFF_LED_RED      4 // 5
#define LS_BAT_LED_GREEN    3 // 4
#define LS_AUX_LED_GREEN    2 // 3
// Bit #1 not used
// Bit #0 not used
  
// Operating mode and state holder constants
// -----------------------------------------
// Source selector
#define SS_OFF  "off "
#define SS_AUTO "auto"
#define SS_MANU "manu"
#define SS_NONE "none"
#define SS_AC   "ac  "
#define SS_BIKE "bike"
#define SS_AUX  "aux "
// Charge controller  
#define CC_OFF  "off "
#define CC_BAT  "bat "
#define CC_HIGH "high"
#define CC_LOW  "low "
#define CC_FINE "fine"
#define CC_CRSE "crse"
// Load selector
#define LS_OFF  "off "
#define LS_BAT  "bat "
#define LS_AUX  "aux "
// Alternator controller
#define AC_OFF  "off "
#define AC_ON   "on  "

// Operating mode and stator holder variables
String ss_mode = SS_OFF;     // Current source selector mode
String ss_source = SS_NONE;  // Current source selector active soruce
String cc_mode = CC_OFF;     // Current charge controller mode
String ls_mode = LS_OFF;     // Current load selector mode
String ac_mode = AC_OFF;     // Current charge controller mode

// Button press variables
// ----------------------
// 0 for not pressed, 1 for pressed
// Source selector
bool ss_off_button = 0;
bool ss_auto_button = 0;
bool ss_manu_button = 0;
bool ss_ac_button = 0;
bool ss_bike_button = 0;
bool ss_aux_button = 0;
// Charge controller
bool cc_off_button = 0;
bool cc_bat_button = 0;
bool cc_high_button = 0;
bool cc_low_button = 0;
bool cc_fine_button = 0;
bool cc_crse_button = 0;
bool cc_up_button = 0;
bool cc_down_button = 0;
// Load selector
bool ls_off_button = 0;
bool ls_bat_button = 0;
bool ls_aux_button = 0;
// Alternator controller
bool ac_en_button = 0;

float val_1 = 0;
float val_2 = 0;
float val_3 = 0;
float val_4 = 0;

// Analog measurement variables
// -----------------------------
const unsigned int num_samples = 2<<3; // == 16; Number of samples to take for ADC measurements, must be power of two when using a bitwise shift
const unsigned int measurement_delay_ms = 20;
// Source selector input voltages
float ADC_ss_ac = 0;
float ADC_ss_bike = 0;
float ADC_ss_aux = 0;
// Miscellaneous measurements
float ADC_cc_pot = 0; // Duty cycle potentiometer for charge controller
float ADC_cc_volt = 0; // Charge controller output voltage
float ADC_ac_curr = 0; // Alternator controller field winding current

// Power monitoring easurement variables
// -------------------------------------
// Voltage measurements
float P1_aux_volt = 0;
float P2_bus_volt = 0;
float P3_sys_volt = 0;
float P4_ac_volt = 0;
float P5_dc_volt = 0;
float P6_in_volt = 0;
float P7_bat_volt = 0;
// Current measurements
float P1_aux_curr = 0;
float P2_bus_curr = 0;
float P3_sys_curr = 0;
float P4_ac_curr = 0;
float P5_dc_curr = 0;
float P6_in_curr = 0;
float P7_bat_curr = 0;
// Power measurements
float P1_aux_pwr = 0;
float P2_bus_pwr = 0;
float P3_sys_pwr = 0;
float P4_ac_pwr = 0;
float P5_dc_pwr = 0;
float P6_in_pwr = 0;
float P7_bat_pwr = 0;

// 0x50 <-- EEPROM address
// LCD initialization variables
LiquidCrystal_I2C lcd_1(0x27, 20, 4);  // 20 chars and 4 line display @ address 0x27
LiquidCrystal_I2C lcd_2(0x3F, 20, 4);  // 20 chars and 4 line display @ address 0x3E
LiquidCrystal_I2C lcd_3(0x3E, 20, 4);  // 20 chars and 4 line display @ address 0x3F

// LCD position variables
uint8_t LCD_row_duty_cycle = 3;
uint8_t LCD_row_efficiency = 4;
uint8_t LCD_row_vout = 4;
uint8_t LCD_row_alt_curr = 4;

// Duty cycle variables
const uint8_t duty_cycle_max = 255*0.25; // Max duty cycle value of 25%, rounded down for byte data type
const uint8_t duty_cycle_crse_step = 1; // Duty cycle increment/decrement value for course mode
uint8_t duty_cycle = 0;

// Shift register variables
uint32_t shift_buttons = 0;
uint32_t shift_outputs = 0; 

// Miscellaneous variables
float efficiency = 75.8; // Charge controller efficiency
float vout = 0; // Charge controller output voltage
float alt_curr = 0; // Alternator field winding current
bool error = 0; // error flag

void setup() {
  // Serial Communitication Initialization
  Serial.begin(9600);
  
  // SPI Initializations
  // -------------------
  // Set SPI pin directions
  pinMode(SPI_CLOCK, OUTPUT);
  pinMode(SPI_DATA,  OUTPUT);
  // Set SPI outputs low
  digitalWrite(SPI_CLOCK, LOW);
  digitalWrite(SPI_DATA,  LOW);
  // Settings and communication startup
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();
  
  // Shift register initializations
  pinMode(SHIFT_ENABLE, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);
  pinMode(SHIFT_READ, INPUT);
  digitalWrite(SHIFT_ENABLE, LOW);
  digitalWrite(SHIFT_LATCH, LOW);

  // 4:1 Analog MUX initializations
  analogReference(EXTERNAL);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S0, OUTPUT);
  digitalWrite(MUX_S1, LOW);
  digitalWrite(MUX_S0, LOW);
  
  /*
  // ADC pins
  pinMode(FINE_PIN, INPUT);
  pinMode(VOUT_PIN, INPUT);

  // Load selector pins
  pinMode(LS_BAT_ENABLE, OUTPUT);
  pinMode(LS_AUX_ENABLE, OUTPUT);
  digitalWrite(LS_BAT_ENABLE, LOW);
  digitalWrite(LS_AUX_ENABLE, LOW);
  */

  // PWM pins initializations
  pinMode(PWM_AC, OUTPUT);
  pinMode(PWM_CC, OUTPUT);
  analogWrite(PWM_AC, 0);
  analogWrite(PWM_CC, 0);
  TCCR1B = TCCR1B & 0xF9;  // Set the PWM frequency on pin 9 and 10 to 31.250kHz

  // LCD Initializations
  // -------------------
  //lcd_1.noBacklight(); // turn off backlight
  lcd_1.init();  // initialize the lcd
  lcd_2.init();  // initialize the lcd
  lcd_3.init();  // initialize the lcd
  lcd_1.clear();
  lcd_2.clear();
  lcd_3.clear();
  lcd_1.backlight(); // turn on backlight
  lcd_2.backlight(); // turn on backlight
  lcd_3.backlight(); // turn on backlight
}

void loop() {
  // Clear previous shift register contents and enable output
  Update_shift_registers();
  digitalWrite(SHIFT_ENABLE, HIGH);

  // Startup LED animation
  //LED_anima
  
  // Intialize LEDs to "off" state
  bitSet(shift_outputs, SS_OFF_LED_RED);
  bitSet(shift_outputs, CC_OFF_LED_RED);
  bitSet(shift_outputs, LS_OFF_LED_RED);
  
  Turn_buttons_on(); // Turn on push buttons
  attachInterrupt(1, ISP_Button_Press, RISING);  // Set up interrupt for push buttons

  //analogWrite(PWM_AC, 2);

  

  while(1) {

    Get_Analog_Measurements();
    Calculate_Power_Measurements();
    LCD_print_power_measurements();
    
    /*
    LCD_clear_row(1); // Clear row
    lcd_1.setCursor(0, 0); // Reset cursor
    lcd_1.print(" Val 1 = "); lcd_1.print(val_1, 5);
    
    LCD_clear_row(2); // Clear row
    lcd_1.setCursor(0, 1); // Reset cursor
    lcd_1.print(" Val 2 = "); lcd_1.print(val_2, 5);
    
    LCD_clear_row(3); // Clear row
    lcd_1.setCursor(0, 2); // Reset cursor
    lcd_1.print(" Val 3 = "); lcd_1.print(val_3, 5);
    
    LCD_clear_row(4); // Clear row
    lcd_1.setCursor(0, 3); // Reset cursor
    lcd_1.print(" Val 4 = "); lcd_1.print(val_4, 5);
    */
    delay(2000);
  }
}




// Duty cycle functions
// --------------------

// Read and update duty cycle value based on the potentiometer measurement
void Update_duty_cycle_fine(void) {
  duty_cycle = analogRead(FINE_PIN)>>2; // Convert from 10-bit to 8-bit value
  if (duty_cycle > duty_cycle_max) // Clamp duty cycle to acceptable range
    duty_cycle = duty_cycle_max;
} //Update_duty_cycle_fine



// LCD functions
// -------------

/*
// Print the duty cycle to the LCD
void LCD_print_duty_cycle(void) {
  LCD_clear_row(LCD_row_duty_cycle); // Clear row
  lcd_1.setCursor(0, LCD_row_duty_cycle-1); // Reset cursor
  // If value is less than 10% add extra padding to string
  if (((float) duty_cycle/255 * 100) < 10) {
    lcd_1.print(" Duty Cycle =  "); lcd_1.print((float) duty_cycle/255 * 100, 1); lcd_1.print("%");
  }
  else {
    lcd_1.print(" Duty Cycle = "); lcd_1.print((float) duty_cycle/255 * 100, 1); lcd_1.print("%");
  }
} // LCD_print_duty_cycle

// Print the efficiency to the LCD
void LCD_print_efficiency(void) {
  LCD_clear_row(LCD_row_efficiency); // Clear row
  lcd_1.setCursor(0, LCD_row_efficiency-1); // Reset cursor
  if (efficiency < 10) {
    lcd_1.print(" Efficiency =  "); lcd_1.print(efficiency,1); lcd_1.print("%");
  }
  else {
    lcd_1.print(" Efficiency = "); lcd_1.print(efficiency,1); lcd_1.print("%");
  }
} // LCD_print_efficiency

// Print the output voltage to the LCD
void LCD_print_vout(void) {
  LCD_clear_row(LCD_row_vout); // Clear row
  lcd_1.setCursor(0, LCD_row_vout-1); // Reset cursor
  if (vout < 10) {
    lcd_1.print(" Vout =  "); lcd_1.print(vout,1); lcd_1.print("V");
  }
  else {
    lcd_1.print(" Vout = "); lcd_1.print(vout,1); lcd_1.print("V");
  }
} // LCD_print_vout

// Print the alternator current to the LCD
void LCD_print_alt_curr(void) {
  LCD_clear_row(LCD_row_alt_curr); // Clear row
  lcd_1.setCursor(0, LCD_row_alt_curr-1); // Reset cursor
  lcd_1.print(" Alt.Current= "); lcd_1.print(alt_curr,3); lcd_1.print("A");
} // LCD_print_alt_curr

// Clear specified row of the LCD
void LCD_clear_row(uint8_t row) {
  lcd_1.setCursor( 0, row-1); // 3rd row
  lcd_1.print("                    ");
} // LCD_clear_row
*/
void LCD_print_power_measurements(void) {
  // Voltage, current, and power measurements are displayed with 4 significant digits

  // LCD Display #2
  // --------------
  // --------------
  lcd_2.clear();
  lcd_2.print("Power Monitoring Sys");
  
  // P6: Input node measurements
  // ---------------------------
  lcd_2.setCursor(0, 1);
  // Print voltage measurement
  if (P6_in_volt < 10) lcd_2.print("0");
  lcd_2.print(P6_in_volt, 2); lcd_2.print("V ");
  // Print current measurement
  if (P6_in_curr > 10) lcd_2.print(P6_in_curr, 2);
  else lcd_2.print(P6_in_curr, 3);
  lcd_2.print("A ");
  // Print power measurement
  if (P6_in_pwr < 100) lcd_2.print("0");
  if (P6_in_pwr < 10) lcd_2.print("0");
  lcd_2.print(P6_in_pwr, 1); lcd_2.print("W");
  
  // P2: Bus node measurements
  // -------------------------
  lcd_2.setCursor(0, 2);
  // Print voltage measurement
  if (P2_bus_volt < 10) lcd_2.print("0");
  lcd_2.print(P2_bus_volt, 2); lcd_2.print("V ");
  // Print current measurement
  if (P2_bus_curr > 10) lcd_2.print(P2_bus_curr, 2);
  else lcd_2.print(P2_bus_curr, 3);
  lcd_2.print("A ");
  // Print power measurement
  if (P2_bus_pwr < 100) lcd_2.print("0");
  if (P2_bus_pwr < 10) lcd_2.print("0");
  lcd_2.print(P2_bus_pwr, 1); lcd_2.print("W");

  // P1: Auxiliary output node measurements
  // --------------------------------------
  lcd_2.setCursor(0, 3);
  // Print voltage measurement
  if (P1_aux_volt < 10) lcd_2.print("0");
  lcd_2.print(P1_aux_volt, 2); lcd_2.print("V ");
  // Print current measurement
  if (P1_aux_curr > 10) lcd_2.print(P1_aux_curr, 2);
  else lcd_2.print(P1_aux_curr, 3);
  lcd_2.print("A ");
  // Print power measurement
  if (P1_aux_pwr < 100) lcd_2.print("0");
  if (P1_aux_pwr < 10) lcd_2.print("0");
  lcd_2.print(P1_aux_pwr, 1); lcd_2.print("W");


  // LCD Display #3
  // --------------
  // --------------
  lcd_3.clear();
  
  // P5: 12VDC output node measurements
  // ----------------------------------
  lcd_3.setCursor(0, 0);
  // Print voltage measurement
  if (P5_dc_volt < 10) lcd_3.print("0");
  lcd_3.print(P5_dc_volt, 2); lcd_3.print("V ");
  // Print current measurement
  if (P5_dc_curr > 10) lcd_3.print(P5_dc_curr, 2);
  else lcd_3.print(P5_dc_curr, 3);
  lcd_3.print("A ");
  // Print power measurement
  if (P5_dc_pwr < 100) lcd_3.print("0");
  if (P5_dc_pwr < 10) lcd_3.print("0");
  lcd_3.print(P5_dc_pwr, 1); lcd_3.print("W");
  
  // P4: 120VAC node measurements (Actually the 12VDC output going to the inverter)
  // ------------------------------------------------------------------------------
  lcd_3.setCursor(0, 1);
  // Print voltage measurement
  if (P4_ac_volt < 10) lcd_3.print("0");
  lcd_3.print(P4_ac_volt, 2); lcd_3.print("V ");
  // Print current measurement
  if (P4_ac_curr > 10) lcd_3.print(P4_ac_curr, 2);
  else lcd_3.print(P4_ac_curr, 3);
  lcd_3.print("A ");
  // Print power measurement
  if (P4_ac_pwr < 100) lcd_3.print("0");
  if (P4_ac_pwr < 10) lcd_3.print("0");
  lcd_3.print(P4_ac_pwr, 1); lcd_3.print("W");
  
  // P3: Systems power node measurements
  // -----------------------------------
  lcd_3.setCursor(0, 2);
  // Print voltage measurement
  if (P3_sys_volt < 10) lcd_3.print("0");
  lcd_3.print(P3_sys_volt, 2); lcd_3.print("V ");
  // Print current measurement
  if (P3_sys_curr > 10) lcd_3.print(P3_sys_curr, 2);
  else lcd_3.print(P3_sys_curr, 3);
  lcd_3.print("A ");
  // Print power measurement
  if (P3_sys_pwr < 100) lcd_3.print("0");
  if (P3_sys_pwr < 10) lcd_3.print("0");
  lcd_3.print(P3_sys_pwr, 1); lcd_3.print("W");
  
  // P7: Battery output node measurements
  // ------------------------------------
  lcd_3.setCursor(0, 3);
  // Print voltage measurement
  if (P7_bat_volt < 10) lcd_3.print("0");
  lcd_3.print(P7_bat_volt, 2); lcd_3.print("V ");
  // Print current measurement
  if (P7_bat_curr >= 0) {
    lcd_3.print("+");
    if (P7_bat_curr > 10) lcd_3.print(P7_bat_curr, 1);
    else lcd_3.print(P7_bat_curr, 2);
  }
  else {
    lcd_3.print("-");
    if (P7_bat_curr < -10) lcd_3.print(-1*P7_bat_curr, 1);
    else lcd_3.print(-1*P7_bat_curr, 2);
  }
  lcd_3.print("A ");
  // Print power measurement
  if (P7_bat_pwr >= 0) {
    lcd_3.print("+");
    if (P7_bat_pwr >= 100) {
      lcd_3.print((int)P7_bat_pwr);
      lcd_3.print(".");
    }
    else {
      if (P7_bat_pwr < 10) lcd_3.print("0");
      lcd_3.print(P7_bat_pwr, 1);
    }
  }
  else {
    lcd_3.print("-");
    if (P7_bat_pwr <= -100) {
      lcd_3.print(-1*(int)P7_bat_pwr);
      lcd_3.print(".");
    }
    else {
      if (P7_bat_pwr > -10) lcd_3.print("0");
      lcd_3.print(-1*P7_bat_pwr, 1);
    }
  }
  lcd_3.print("W");
}



void LCD_clear_row(String lcd_number, uint8_t row) {
  if (lcd_number == "lcd_1") {
      lcd_1.setCursor(0, row);
      lcd_1.print("                    ");
  }
  else if (lcd_number == "lcd_2") {
      lcd_2.setCursor(0, row);
      lcd_2.print("                    ");
  }
  else if (lcd_number == "lcd_3") {
      lcd_3.setCursor(0, row);
      lcd_3.print("                    ");
  }
}

void Update_source_selector_mode(void) {
  if ((ss_off_button == HIGH) && (!(ss_mode == SS_OFF))) {
    ss_mode = SS_OFF;
    Turn_source_selector_mode_LEDs_off();
    bitSet(shift_outputs, SS_OFF_LED_RED);
    Update_shift_registers();
  }
  else if ((ss_auto_button == HIGH) && (!(ss_mode == SS_AUTO))) {
    ss_mode = SS_AUTO;
    Turn_source_selector_mode_LEDs_off();
    bitSet(shift_outputs, SS_AUTO_LED_GREEN);
    Update_shift_registers();
  }
  else if ((ss_manu_button == HIGH) && (!(ss_mode == SS_MANU))) {
    ss_mode = SS_MANU;
    Turn_source_selector_mode_LEDs_off();
    bitSet(shift_outputs, SS_MANU_LED_GREEN);
    Update_shift_registers();
  }
  else if ((ss_ac_button == HIGH) && (!(ss_mode == SS_AC))) {
    ss_mode = SS_AC;
    Turn_source_selector_mode_LEDs_off();
    bitSet(shift_outputs, SS_AC_LED_GREEN);
    Update_shift_registers();
  }
  else if ((ss_bike_button == HIGH) && (!(ss_mode == SS_BIKE))) {
    ss_mode = SS_BIKE;
    Turn_source_selector_mode_LEDs_off();
    bitSet(shift_outputs, SS_BIKE_LED_GREEN);
    Update_shift_registers();
  }
  else if ((ss_aux_button == HIGH) && (!(ss_mode == SS_AUX))) {
    ss_mode = SS_AUX;
    Turn_source_selector_mode_LEDs_off();
    bitSet(shift_outputs, SS_AUX_LED_GREEN);
    Update_shift_registers();
  }
}

void Update_charge_controller_mode(void) {
  if ((cc_off_button == HIGH) && (!(cc_mode == CC_OFF))) {
    cc_mode = CC_OFF;
    Turn_charge_controller_mode_LEDs_off();
    bitSet(shift_outputs, CC_OFF_LED_RED);
    Update_shift_registers();
  }
  else if ((cc_bat_button == HIGH) && (!(cc_mode == CC_BAT))) {
    cc_mode = CC_BAT;
    Turn_charge_controller_mode_LEDs_off();
    bitSet(shift_outputs, CC_BAT_LED_GREEN);
    Update_shift_registers();
  }
 else if ((cc_high_button == HIGH) && (!(cc_mode == CC_HIGH))) {
    cc_mode = CC_HIGH;
    Turn_charge_controller_mode_LEDs_off();
    bitSet(shift_outputs, CC_HIGH_LED_GREEN);
    Update_shift_registers();
  }
  else if ((cc_low_button == HIGH) && (!(cc_mode == CC_LOW))) {
    cc_mode = CC_LOW;
    Turn_charge_controller_mode_LEDs_off();
    bitSet(shift_outputs, CC_LOW_LED_GREEN);
    Update_shift_registers();
  }
  else if ((cc_fine_button == HIGH) && (!(cc_mode == CC_FINE))) {
    cc_mode = CC_FINE;
    Turn_charge_controller_mode_LEDs_off();
    bitSet(shift_outputs, CC_FINE_LED_GREEN);
    Update_shift_registers();
  }
  else if ((cc_crse_button == HIGH) && (!(cc_mode == CC_CRSE))) {
    cc_mode = CC_CRSE;
    Turn_charge_controller_mode_LEDs_off();
    bitSet(shift_outputs, CC_CRSE_LED_GREEN);
    Update_shift_registers();
  }
}

void Update_load_selector_mode(void) {
  if ((ls_off_button == HIGH) && (!(ls_mode == LS_OFF))) {
    ls_mode = LS_OFF;
    Turn_load_selector_mode_LEDs_off();
    bitSet(shift_outputs, LS_OFF_LED_RED);
    Update_shift_registers();
    //digitalWrite(LS_BAT_ENABLE, LOW);
    //digitalWrite(LS_AUX_ENABLE, LOW);
  }
  else if ((ls_bat_button == HIGH) && (!(ls_mode == LS_BAT))) {
    ls_mode = LS_BAT;
    Turn_load_selector_mode_LEDs_off();
    bitSet(shift_outputs, LS_BAT_LED_GREEN);
    Update_shift_registers();
    //digitalWrite(LS_BAT_ENABLE, LOW);
    //digitalWrite(LS_AUX_ENABLE, LOW);
    //delayMicroseconds(5000);
    //digitalWrite(LS_BAT_ENABLE, HIGH);
  }
  else if ((ls_aux_button == HIGH) && (!(ls_mode == LS_AUX))) {
    ls_mode = LS_AUX;
    Turn_load_selector_mode_LEDs_off();
    bitSet(shift_outputs, LS_AUX_LED_GREEN);
    Update_shift_registers();
    //digitalWrite(LS_BAT_ENABLE, LOW);
    //digitalWrite(LS_AUX_ENABLE, LOW);
    //delayMicroseconds(5000); 
    //digitalWrite(LS_AUX_ENABLE, HIGH);
  }
}

void Update_alternator_controller_mode(void) {
  if ((ac_en_button == HIGH) && (!(ac_mode == AC_OFF))) {
    ac_mode = AC_OFF;
    Turn_alternator_controller_mode_LEDs_off();
  }
  else if ((ac_en_button == HIGH) && (!(ac_mode == AC_ON))) {
    ac_mode = AC_ON;
    Turn_alternator_controller_mode_LEDs_off();
    bitSet(shift_outputs, AC_EN_LED_GREEN);
    Update_shift_registers();
  }
}

void Turn_source_selector_mode_LEDs_off(void) {
  bitClear(shift_outputs, SS_OFF_LED_RED);
  bitClear(shift_outputs, SS_AUTO_LED_GREEN);
  bitClear(shift_outputs, SS_MANU_LED_GREEN);
  bitClear(shift_outputs, SS_AC_LED_GREEN);
  bitClear(shift_outputs, SS_BIKE_LED_GREEN);
  bitClear(shift_outputs, SS_AUX_LED_GREEN);
  Update_shift_registers();
}

void Turn_charge_controller_mode_LEDs_off(void) {
  bitClear(shift_outputs, CC_OFF_LED_RED);
  bitClear(shift_outputs, CC_BAT_LED_GREEN);
  bitClear(shift_outputs, CC_HIGH_LED_GREEN);
  bitClear(shift_outputs, CC_LOW_LED_GREEN);
  bitClear(shift_outputs, CC_FINE_LED_GREEN);
  bitClear(shift_outputs, CC_CRSE_LED_GREEN);
  Update_shift_registers();
}

void Turn_load_selector_mode_LEDs_off(void) {
  bitClear(shift_outputs, LS_OFF_LED_RED);
  bitClear(shift_outputs, LS_BAT_LED_GREEN);
  bitClear(shift_outputs, LS_AUX_LED_GREEN);
  Update_shift_registers();
}

void Turn_alternator_controller_mode_LEDs_off(void) {
  bitClear(shift_outputs, AC_EN_LED_GREEN);
  Update_shift_registers();
}

void Locate_button_presses(void) {
  int button_press_delay = 0;
  
  // Source selector auto button
  bitSet(shift_buttons, SS_OFF_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ss_off_button = 1;
  }
  else {
    ss_off_button = 0;
  }
  bitClear(shift_buttons, SS_OFF_BUTTON);
  
  // Source selector auto button
  bitSet(shift_buttons, SS_AUTO_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ss_auto_button = 1;
  }
  else {
    ss_auto_button = 0;
  }
  bitClear(shift_buttons, SS_AUTO_BUTTON);
  
  // Source selector manual button
  bitSet(shift_buttons, SS_MANU_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
   if (digitalRead(SHIFT_READ) == HIGH) {
    ss_manu_button = 1;
  }
  else {
    ss_manu_button = 0;
  }
  bitClear(shift_buttons, SS_MANU_BUTTON);
  
  // Source selector ac button
  bitSet(shift_buttons, SS_AC_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ss_ac_button = 1;
  }
  else {
    ss_ac_button = 0;
  }
  bitClear(shift_buttons, SS_AC_BUTTON);
  
  // Source selector bike button
  bitSet(shift_buttons, SS_BIKE_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ss_bike_button = 1;
  }
  else {
    ss_bike_button = 0;
  }
  bitClear(shift_buttons, SS_BIKE_BUTTON); 
  
  // Source selector aux button
  bitSet(shift_buttons, SS_AUX_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ss_aux_button = 1;
  }
  else {
    ss_aux_button = 0;
  }
  bitClear(shift_buttons, SS_AUX_BUTTON);
  
  // Charge controller off button
  bitSet(shift_buttons, CC_OFF_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_off_button = 1;
  }
  else {
    cc_off_button = 0;
  }
  bitClear(shift_buttons, CC_OFF_BUTTON);
  
  // Charge controller battery button
  bitSet(shift_buttons, CC_BAT_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_bat_button = 1;
  }
  else {
    cc_bat_button = 0;
  }  
  bitClear(shift_buttons, CC_BAT_BUTTON);

  // Charge contoller high button
  bitSet(shift_buttons, CC_HIGH_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_high_button = 1;
  }
  else {
    cc_high_button = 0;
  }  
  bitClear(shift_buttons, CC_HIGH_BUTTON);
  
  // Charge controller low button
  bitSet(shift_buttons, CC_LOW_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_low_button = 1;
  }
  else {
    cc_low_button = 0;
  }  
  bitClear(shift_buttons, CC_LOW_BUTTON);

  // Charge controller fine button
  bitSet(shift_buttons, CC_FINE_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_fine_button = 1;
  }
  else {
    cc_fine_button = 0;
  }  
  bitClear(shift_buttons, CC_FINE_BUTTON);
  
  // Charge controller course button
  bitSet(shift_buttons, CC_CRSE_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_crse_button = 1;
  }
  else {
    cc_crse_button = 0;
  }  
  bitClear(shift_buttons, CC_CRSE_BUTTON);

  // Charge controller up button
  bitSet(shift_buttons, CC_UP_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_up_button = 1;
  }
  else {
    cc_up_button = 0;
  }  
  bitClear(shift_buttons, CC_UP_BUTTON);

  // Charge controller down button
  bitSet(shift_buttons, CC_DOWN_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    cc_down_button = 1;
  }
  else {
    cc_down_button = 0;
  }  
  bitClear(shift_buttons, CC_DOWN_BUTTON);
   
  // Load selector off button
  bitSet(shift_buttons, LS_OFF_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
   if (digitalRead(SHIFT_READ) == HIGH) {
    ls_off_button = 1;
  }
  else {
    ls_off_button = 0;
  } 
  bitClear(shift_buttons, LS_OFF_BUTTON);

  // Load selector battery button
  bitSet(shift_buttons, LS_BAT_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ls_bat_button = 1;
  }
  else {
    ls_bat_button = 0;
  }  
  bitClear(shift_buttons, LS_BAT_BUTTON);
  
  // Load selector aux button
  bitSet(shift_buttons, LS_AUX_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ls_aux_button = 1;
  }
  else {
    ls_aux_button = 0;
  }  
  bitClear(shift_buttons, LS_AUX_BUTTON);

  // Alternator controller enable button
  bitSet(shift_buttons, AC_EN_BUTTON);
  Update_shift_registers();
  delayMicroseconds(button_press_delay);
  if (digitalRead(SHIFT_READ) == HIGH) {
    ac_en_button = 1;
  }
  else {
    ac_en_button = 0;
  }  
  bitClear(shift_buttons, AC_EN_BUTTON);
}

void Turn_buttons_off(void) {
  bitClear(shift_buttons, SS_OFF_BUTTON);
  bitClear(shift_buttons, SS_AUTO_BUTTON);
  bitClear(shift_buttons, SS_MANU_BUTTON);
  bitClear(shift_buttons, SS_AC_BUTTON);
  bitClear(shift_buttons, SS_BIKE_BUTTON);
  bitClear(shift_buttons, SS_AUX_BUTTON);
  
  bitClear(shift_buttons, CC_OFF_BUTTON);
  bitClear(shift_buttons, CC_BAT_BUTTON);
  bitClear(shift_buttons, CC_HIGH_BUTTON);
  bitClear(shift_buttons, CC_LOW_BUTTON);
  bitClear(shift_buttons, CC_FINE_BUTTON);
  bitClear(shift_buttons, CC_CRSE_BUTTON);
  bitClear(shift_buttons, CC_UP_BUTTON);
  bitClear(shift_buttons, CC_DOWN_BUTTON);
  
  bitClear(shift_buttons, LS_OFF_BUTTON);
  bitClear(shift_buttons, LS_BAT_BUTTON);
  bitClear(shift_buttons, LS_AUX_BUTTON);

  bitClear(shift_buttons, AC_EN_BUTTON);

  Update_shift_registers();
}

void Turn_buttons_on(void) {
  bitSet(shift_buttons, SS_OFF_BUTTON);
  bitSet(shift_buttons, SS_AUTO_BUTTON);
  bitSet(shift_buttons, SS_MANU_BUTTON);
  bitSet(shift_buttons, SS_AC_BUTTON);
  bitSet(shift_buttons, SS_BIKE_BUTTON);
  bitSet(shift_buttons, SS_AUX_BUTTON);
  
  bitSet(shift_buttons, CC_OFF_BUTTON);
  bitSet(shift_buttons, CC_BAT_BUTTON);
  bitSet(shift_buttons, CC_HIGH_BUTTON);
  bitSet(shift_buttons, CC_LOW_BUTTON);
  bitSet(shift_buttons, CC_FINE_BUTTON);
  bitSet(shift_buttons, CC_CRSE_BUTTON);
  bitSet(shift_buttons, CC_UP_BUTTON);
  bitSet(shift_buttons, CC_DOWN_BUTTON);
  
  bitSet(shift_buttons, LS_OFF_BUTTON);
  bitSet(shift_buttons, LS_BAT_BUTTON);
  bitSet(shift_buttons, LS_AUX_BUTTON);

  bitSet(shift_buttons, AC_EN_BUTTON);

  Update_shift_registers();
}

void ISP_Button_Press(void) {
  // Interrupt debouncing variables
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200) {
    Turn_buttons_off();
    Locate_button_presses();
    
    // Update modes
    Update_source_selector_mode();
    Update_charge_controller_mode();
    Update_load_selector_mode();
    Update_alternator_controller_mode();
        
    // Update charge controller duty cycle
    //Update_duty_cycle();

    Clear_button_presses();
    Turn_buttons_on();
    //while(digitalRead(SHIFT_READ)); // Wait while button pressed
  }
  last_interrupt_time = interrupt_time;
}

void Update_shift_registers() {
  // Pull latch low
  digitalWrite(SHIFT_LATCH, LOW);
  delayMicroseconds(5000);

 
  // Transfer data  
  SPI.transfer(shift_buttons>>16 & 0xFF);
  SPI.transfer(shift_buttons>>8  & 0xFF);
  SPI.transfer(shift_buttons     & 0xFF);
  SPI.transfer(shift_outputs>>16 & 0xFF);
  SPI.transfer(shift_outputs>>8  & 0xFF);
  SPI.transfer(shift_outputs     & 0xFF);
 

  /*
  // Transfer data - Used if not usual designated SPI pins, be sure to remove unnecessary SPI code
  shiftOut(SPI_DATA, SPI_CLOCK, MSBFIRST, shift_buttons>>16 & 0xFF);
  shiftOut(SPI_DATA, SPI_CLOCK, MSBFIRST, shift_buttons>>8 & 0xFF);
  shiftOut(SPI_DATA, SPI_CLOCK, MSBFIRST, shift_buttons & 0xFF);
  shiftOut(SPI_DATA, SPI_CLOCK, MSBFIRST, shift_outputs>>16 & 0xFF);
  shiftOut(SPI_DATA, SPI_CLOCK, MSBFIRST, shift_outputs>>8 & 0xFF);
  shiftOut(SPI_DATA, SPI_CLOCK, MSBFIRST, shift_outputs & 0xFF);
  */
  
  // Pull latch high
  digitalWrite(SHIFT_LATCH, HIGH);
  delayMicroseconds(5000); 
}

void Update_duty_cycle(void) {
  if (cc_mode == CC_OFF)
    duty_cycle = 0;
  
  else if (cc_mode == CC_FINE)
    duty_cycle = analogRead(FINE_PIN)>>2; // Convert 10-bit ADC value to 8-bit PWM value
  
  else if (cc_mode == CC_CRSE) {
    // Increment/decrement if up/down buttons are pressed
    if (cc_up_button == HIGH)
      duty_cycle += duty_cycle_crse_step;
    if (cc_down_button == HIGH)
      duty_cycle -= duty_cycle_crse_step;
  }

  // Clamp duty cycle between limits
  if (duty_cycle > duty_cycle_max)
    duty_cycle = duty_cycle_max;
  else if (duty_cycle < 0)
    duty_cycle = 0;

  analogWrite(PWM_CC, duty_cycle);
}

void Clear_button_presses(void) {
  // Source selector
  ss_off_button = 0;
  ss_auto_button = 0;
  ss_manu_button = 0;
  ss_ac_button = 0;
  ss_bike_button = 0;
  ss_aux_button = 0;
  
  // Charge controller
  cc_off_button = 0;
  cc_bat_button = 0;
  cc_high_button = 0;
  cc_low_button = 0;
  cc_fine_button = 0;
  cc_crse_button = 0;
  cc_up_button = 0;
  cc_down_button = 0;
  
  // Load selector
  ls_off_button = 0;
  ls_bat_button = 0;
  ls_aux_button = 0;
  
  // Alternator controller
  ac_en_button = 0;
}

void Get_Analog_Measurements(void) {
  // Variables
  // ---------
  unsigned int i;
  float ADC_sum;
  const float ADC_scale = 5/1023.0/num_samples;
  // Calibration variables
  const float P1_aux_volt_scale = 1;
  const float P2_bus_curr_scale = 1;
  const float P1_aux_curr_scale = 1;
  const float ADC_ac_curr_scale = 1;
  const float P2_bus_volt_scale = 1;
  const float P3_sys_curr_scale = 1;
  const float ADC_ss_ac_scale = 1;
  const float ADC_cc_volt_scale = 1;
  const float P5_dc_volt_scale = 1;
  const float P4_ac_curr_scale = 1;
  const float ADC_ss_bike_scale = 1;
  const float P6_in_curr_scale = 1;
  const float P6_in_volt_scale = 1;
  const float P5_dc_curr_scale = 2.041;
  const float ADC_ss_aux_scale = 1;
  const float ADC_cc_pot_scale = 1;



  // S0 = 0, S1 = 0
  // --------------
  digitalWrite(MUX_S0, LOW);
  digitalWrite(MUX_S1, LOW);
  delay(measurement_delay_ms);
    
  // Get channel 1 measurement
  ADC_sum = 0;
  analogRead(A0); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A0);
  }
  P1_aux_volt = P1_aux_volt_scale*ADC_scale*ADC_sum;
  Serial.print("P1_aux_volt = "); Serial.print(P1_aux_volt,5); Serial.print("\n");
  
  // Get channel 2 measurement
  ADC_sum = 0;
  analogRead(A1); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A1);
  }
  P2_bus_curr = P2_bus_curr_scale*ADC_scale*ADC_sum;
  Serial.print("P2_bus_curr = "); Serial.print(P2_bus_curr,5); Serial.print("\n");
  
  // Get channel 3 measurement
  ADC_sum = 0;
  analogRead(A2); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A2);
  }
  P1_aux_curr = P1_aux_curr_scale*ADC_scale*ADC_sum;
  Serial.print("P1_aux_curr = "); Serial.print(P1_aux_curr,5); Serial.print("\n");
  
  // Get channel 4 measurement
  ADC_sum = 0;
  analogRead(A3); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A3);
  }
  ADC_ac_curr = ADC_ac_curr_scale*ADC_scale*ADC_sum;
  Serial.print("ADC_ac_curr = "); Serial.print(ADC_ac_curr,5); Serial.print("\n");
  
    
  // S0 = 0, S1 = 1
  // --------------
  digitalWrite(MUX_S0, LOW);
  digitalWrite(MUX_S1, HIGH);
  delay(measurement_delay_ms);
    
  // Get channel 1 measurement
  ADC_sum = 0;
  analogRead(A0); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A0);
  }
  P2_bus_volt = P2_bus_volt_scale*ADC_scale*ADC_sum;
  Serial.print("P2_bus_volt = "); Serial.print(P2_bus_volt,5); Serial.print("\n");
  
  // Get channel 2 measurement
  ADC_sum = 0;
  analogRead(A1); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A1);
  }
  P3_sys_curr = P3_sys_curr_scale*ADC_scale*ADC_sum;
  Serial.print("P3_sys_curr = "); Serial.print(P3_sys_curr,5); Serial.print("\n");
  
  // Get channel 3 measurement
  ADC_sum = 0;
  analogRead(A2); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A2);
  }
  ADC_ss_ac = ADC_ss_ac_scale*ADC_scale*ADC_sum;
  Serial.print("ADC_ss_ac = "); Serial.print(ADC_ss_ac,5); Serial.print("\n");
  
  // Get channel 4 measurement
  ADC_sum = 0;
  analogRead(A3); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A3);
  }
  ADC_cc_volt = ADC_cc_volt_scale*ADC_scale*ADC_sum;
  Serial.print("ADC_cc_volt = "); Serial.print(ADC_cc_volt,5); Serial.print("\n");


  // S0 = 1, S1 = 0
  // --------------
  digitalWrite(MUX_S0, HIGH);
  digitalWrite(MUX_S1, LOW);
  delay(measurement_delay_ms);
    
  // Get channel 1 measurement
  ADC_sum = 0;
  analogRead(A0); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A0);
  }
  P5_dc_volt = P5_dc_volt_scale*ADC_scale*ADC_sum;
  Serial.print("P5_dc_volt = "); Serial.print(P5_dc_volt,5); Serial.print("\n");
  
  // Get channel 2 measurement
  ADC_sum = 0;
  analogRead(A1); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A1);
  }
  P4_ac_curr = P4_ac_curr_scale*ADC_scale*ADC_sum;
  Serial.print("P4_ac_curr = "); Serial.print(P4_ac_curr,5); Serial.print("\n");
  
  // Get channel 3 measurement
  ADC_sum = 0;
  analogRead(A2); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A2);
  }
  ADC_ss_bike = ADC_ss_bike_scale*ADC_scale*ADC_sum;
  Serial.print("ADC_ss_bike = "); Serial.print(ADC_ss_bike,5); Serial.print("\n");
  
  // Get channel 4 measurement
  ADC_sum = 0;
  analogRead(A3); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A3);
  }
  P6_in_curr = P6_in_curr_scale*ADC_scale*ADC_sum;
  Serial.print("P6_in_curr = "); Serial.print(P6_in_curr,5); Serial.print("\n");


  // S0 = 1, S1 = 1
  // --------------
  digitalWrite(MUX_S0, HIGH);
  digitalWrite(MUX_S1, HIGH);
  delay(measurement_delay_ms);
    
  // Get channel 1 measurement
  ADC_sum = 0;
  analogRead(A0); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A0);
  }
  P6_in_volt = P6_in_volt_scale*ADC_scale*ADC_sum;
  Serial.print("P6_in_volt = "); Serial.print(P6_in_volt,5); Serial.print("\n");
  
  // Get channel 2 measurement
  ADC_sum = 0;
  analogRead(A1); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A1);
  }
  P5_dc_curr = P5_dc_curr_scale*ADC_scale*ADC_sum;
  Serial.print("P5_dc_curr = "); Serial.print(P5_dc_curr,5); Serial.print("\n");
  
  // Get channel 3 measurement
  ADC_sum = 0;
  analogRead(A2); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A2);
  }
  ADC_ss_aux = ADC_ss_aux_scale*ADC_scale*ADC_sum;
  Serial.print("ADC_ss_aux = "); Serial.print(ADC_ss_aux,5); Serial.print("\n");
  
  // Get channel 4 measurement
  ADC_sum = 0;
  analogRead(A3); // Throw away first sample
  for (i = 0; i < num_samples; i++) {
    ADC_sum += analogRead(A3);
  }
  ADC_cc_pot = ADC_cc_pot_scale*ADC_scale*ADC_sum;
  Serial.print("ADC_cc_pot = "); Serial.print(ADC_cc_pot,5); Serial.print("\n");

  Serial.print("----------------\n");
}


void Calculate_Power_Measurements(void) {
  // Copy voltage values for shared bus node
  P3_sys_volt = P2_bus_volt;
  P4_ac_volt = P2_bus_volt;
  P7_bat_volt = P2_bus_volt;

  // Calculate battery net current based on KCL
  P7_bat_curr = P2_bus_curr - P3_sys_curr - P4_ac_curr - P5_dc_curr;

  // Calculate power measurements from voltage and current measurements
  P1_aux_pwr = P1_aux_volt * P1_aux_curr;
  P2_bus_pwr = P2_bus_volt * P2_bus_curr;
  P3_sys_pwr = P3_sys_volt * P3_sys_curr;
  P4_ac_pwr  = P4_ac_volt  * P4_ac_curr;
  P5_dc_pwr  = P5_dc_volt  * P5_dc_curr;
  P6_in_pwr  = P6_in_volt  * P6_in_curr;
  P7_bat_pwr = P7_bat_volt * P7_bat_curr;
}
