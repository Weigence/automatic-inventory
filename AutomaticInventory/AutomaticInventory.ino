#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

uint8_t lcd_addr = 0x3F;
uint8_t lcd_rows = 2;
uint8_t lcd_cols = 16;

LiquidCrystal_I2C lcd(lcd_addr, lcd_rows, lcd_cols);

/// @brief Pin connected to HX711 DOUT (data out)
const int LOADCELL_DOUT_PIN = 18; // 2;

/// @brief Pin connected to HX711 SCK (clock)
const int LOADCELL_SCK_PIN = 19; // 3;

#define TARE_BUTTON_PIN 27
uint32_t lastReading;

/// @brief Approximate weight (in units from the scale) of a single object
#define WEIGHT_OF_OBJ 70

HX711 scale;

/**
 * @brief Initializes serial communication and calibrates the HX711 scale.
 * 
 * The function sets the scale factor and tares the scale.
 * Users are prompted to remove the object before taring.
 */
void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Wire.begin(21, 22);

  scale.set_scale(366); ///< Adjust this value based on your calibration
  Serial.println("Calibrating... Please remove the object.");
  delay(5000); ///< Wait for user to remove the object
  scale.tare(); ///< Set the current weight as zero
  Serial.println("Scale tared.");

  lastReading = millis();

  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.print("Weigence");
  delay(5000);
  lcd.clear();
}

/**
 * @brief Continuously reads the weight from the HX711 and estimates the number of objects.
 * 
 * If the scale is ready, it takes 10 averaged readings and prints:
 * - The current weight
 * - The estimated number of objects based on WEIGHT_OF_OBJ
 * 
 * Otherwise, it prints a message if the HX711 is not detected.
 */
void loop() {
  if (millis() - lastReading >= 20) {
    lastReading = millis();

    int value = digitalRead(TARE_BUTTON_PIN);
    if (value == HIGH) {
      scale.tare(); ///< Set the current weight as zero
      Serial.println("Scale tared.");
    }
  
    if (scale.is_ready()) {
      Serial.print("Place the object... ");
      long reading = scale.get_units(10); ///< Take average of 10 readings
      if (reading < 0) {
        scale.tare(); ///< Set the current weight as zero
        Serial.println("Negative Reading -> Scale tared.");
      }
      Serial.print("Weight: ");
      Serial.println(reading);

      long how_many = reading / WEIGHT_OF_OBJ; ///< Estimate object count
      Serial.print("Estimated number of objects: ");
      Serial.println(how_many);
      lcd.print("Fabbrica: ");
      lcd.print(how_many);
      lcd.home();
    }
  }
}
