#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/// @brief I2C address of the LCD
uint8_t lcd_addr = 0x3F;

/// @brief Number of LCD rows
uint8_t lcd_rows = 2;

/// @brief Number of LCD columns
uint8_t lcd_cols = 16;

LiquidCrystal_I2C lcd(lcd_addr, lcd_cols, lcd_rows);

/// @brief Pin connected to HX711 DOUT (data out)
const int LOADCELL_DOUT_PIN = 18;

/// @brief Pin connected to HX711 SCK (clock)
const int LOADCELL_SCK_PIN = 19;

/// @brief Pin for manual tare button
#define TARE_BUTTON_PIN 27

/// @brief Timestamp for managing reading interval
uint32_t lastReading;

/// @brief Weight of a factory (new) object in scale units
#define WEIGHT_OF_FABRIC_OBJ 70

/// @brief Weight of a reused object in scale units
#define WEIGHT_OF_REUSED_OBJ 85

/// @brief Accepted tolerance when matching mixed weights
#define EPSILON 3

HX711 scale;

/**
 * @brief Initializes serial communication, LCD display, HX711 scale, and performs taring.
 * 
 * The user has 5 seconds to remove all objects before the tare is applied.
 */
void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Wire.begin(21, 22);  ///< Custom SDA/SCL pins for ESP32

  scale.set_scale(366); ///< Calibration factor for the load cell
  Serial.println("Calibrating... Please remove the object.");
  delay(5000);          ///< Allow time to remove objects
  scale.tare();         ///< Zero the scale
  Serial.println("Scale tared.");

  lastReading = millis();

  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.print("Weigence");
  delay(500);
  lcd.clear();
}

/**
 * @brief Reads from the scale every 20ms and displays estimated object counts.
 * 
 * If the tare button is pressed, the scale is re-tared. The scale attempts to calculate
 * the combination of factory and reused objects based on their weights and a tolerance (`EPSILON`).
 * Results are shown on the LCD display.
 */
void loop() {
  if (millis() - lastReading >= 20) {
    lastReading = millis();

    // Check for tare button press
    if (digitalRead(TARE_BUTTON_PIN) == HIGH) {
      scale.tare();
      Serial.println("Scale tared.");
      lcd.clear();
    }

    // Proceed if scale is ready
    if (scale.is_ready()) {
      Serial.print("Place the object... ");
      long reading = scale.get_units(10); ///< Average of 10 readings

      // Auto-tare if negative reading is detected
      if (reading < 0) {
        scale.tare();
        Serial.println("Negative reading -> Scale tared.");
        lcd.clear();
      }

      int count_fabric = -1;
      int count_reused = -1;
      bool found = false;

      // Try combinations of factory and reused items
      for (int m = 0; m <= reading / WEIGHT_OF_FABRIC_OBJ; m++) {
        int remainder = reading - m * WEIGHT_OF_FABRIC_OBJ;

        // Check within ±EPSILON range to handle noise or inaccuracy
        for (int delta = -EPSILON; delta <= EPSILON; delta++) {
          int adjusted_remainder = remainder + delta;
          if (adjusted_remainder >= 0 && adjusted_remainder % WEIGHT_OF_REUSED_OBJ == 0) {
            count_fabric = m;
            count_reused = adjusted_remainder / WEIGHT_OF_REUSED_OBJ;
            found = true;
            break;
          }
        }

        if (found) break;
      }

      Serial.print("Weight: ");
      Serial.println(reading);

      // Display results if valid
      if (count_fabric >= 0 && count_reused >= 0) {
        lcd.setCursor(0, 0);
        lcd.print("Factory: ");
        lcd.print(count_fabric);
        lcd.setCursor(0, 1);
        lcd.print("Reused:  ");
        lcd.print(count_reused);
      }
    }
  }
}
