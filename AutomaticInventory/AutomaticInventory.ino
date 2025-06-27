/**
 * @file AutomaticInventory.ino
 * @brief Automatic inventory system using HX711 load cell and LCD display
 * @authors Antonio Bernardini e Giulio Bucchi
 * @date June 2025
 * 
 * This system weighs objects and determines the count of factory and reused items
 * based on their known weights. Results are displayed on an I2C LCD.
 */

// Library includes
#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================

/// @brief I2C address of the LCD
const uint8_t LCD_ADDR = 0x3F;

/// @brief Number of LCD rows
const uint8_t LCD_ROWS = 2;

/// @brief Number of LCD columns
const uint8_t LCD_COLS = 16;

/// @brief Pin connected to HX711 DOUT (data out)
const int LOADCELL_DOUT_PIN = 18;

/// @brief Pin connected to HX711 SCK (clock)
const int LOADCELL_SCK_PIN = 19;

/// @brief Pin for manual tare button
const int TARE_BUTTON_PIN = 27;

/// @brief Custom SDA pin for ESP32 I2C
const int SDA_PIN = 21;

/// @brief Custom SCL pin for ESP32 I2C
const int SCL_PIN = 22;

// =============================================================================
// MEASUREMENT CONFIGURATION
// =============================================================================

/// @brief Weight of a factory (new) object in scale units
const int WEIGHT_OF_FABRIC_OBJ = 70;

/// @brief Weight of a reused object in scale units
const int WEIGHT_OF_REUSED_OBJ = 85;

/// @brief Accepted tolerance when matching mixed weights
const int EPSILON = 3;

/// @brief Scale calibration factor
const float SCALE_CALIBRATION_FACTOR = 366.0;

/// @brief Reading interval in milliseconds
const unsigned long READING_INTERVAL = 20;

/// @brief Tare delay in milliseconds
const unsigned long TARE_DELAY = 5000;

/// @brief Number of readings to average
const int SCALE_READINGS = 10;

// =============================================================================
// GLOBAL OBJECTS AND VARIABLES
// =============================================================================

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
HX711 scale;

/// @brief Timestamp for managing reading interval
uint32_t lastReading;

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Initializes the LCD display
 */
void initializeLCD() {
  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.print("Weigence");
  delay(500);
  lcd.clear();
}

/**
 * @brief Initializes and calibrates the scale
 */
void initializeScale() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(SCALE_CALIBRATION_FACTOR);

  Serial.println("Calibrating... Please remove the object.");
  delay(TARE_DELAY);
  scale.tare();
  Serial.println("Scale tared.");
}

/**
 * @brief Calculates the combination of fabric and reused objects
 * @param reading The weight reading from the scale
 * @param count_fabric Reference to store fabric object count
 * @param count_reused Reference to store reused object count
 * @return true if a valid combination is found, false otherwise
 */
bool calculateObjectCounts(long reading, int& count_fabric, int& count_reused) {
  count_fabric = -1;
  count_reused = -1;

  // Try combinations of factory and reused items
  for (int m = 0; m <= reading / WEIGHT_OF_FABRIC_OBJ; ++m) {
    int weight_fabric = m * WEIGHT_OF_FABRIC_OBJ;

    for (int n = 0; n <= (reading - weight_fabric) / WEIGHT_OF_REUSED_OBJ; ++n) {
      int weight_reused = n * WEIGHT_OF_REUSED_OBJ;
      int total = weight_fabric + weight_reused;

      // Verify that the total is within tolerance
      if (abs(total - reading) <= EPSILON) {
        count_fabric = m;
        count_reused = n;
        return true;
      }
    }
  }

  return false;
}

/**
 * @brief Updates the LCD display with object counts
 * @param count_fabric Number of fabric objects
 * @param count_reused Number of reused objects
 */
void updateDisplay(int count_fabric, int count_reused) {
  lcd.setCursor(0, 0);
  lcd.print("Factory: ");
  lcd.print(count_fabric);
  lcd.setCursor(0, 1);
  lcd.print("Reused:  ");
  lcd.print(count_reused);
}

/**
 * @brief Handles the tare button press and scale taring
 */
void handleTareButton() {
  if (digitalRead(TARE_BUTTON_PIN) == HIGH) {
    scale.tare();
    Serial.println("Scale tared.");
    lcd.clear();
  }
}

/**
 * @brief Processes the scale reading and updates display
 */
void processScaleReading() {
  if (!scale.is_ready()) {
    return;
  }

  Serial.print("Place the object... ");
  long reading = scale.get_units(SCALE_READINGS);

  // Auto-tare if negative reading is detected
  if (reading < 0) {
    scale.tare();
    Serial.println("Negative reading -> Scale tared.");
    lcd.clear();
    return;
  }

  int count_fabric, count_reused;
  bool found = calculateObjectCounts(reading, count_fabric, count_reused);

  Serial.print("Weight: ");
  Serial.println(reading);

  // Display results if valid combination found
  if (found && count_fabric >= 0 && count_reused >= 0) {
    updateDisplay(count_fabric, count_reused);
  }
}

// =============================================================================
// MAIN FUNCTIONS
// =============================================================================

/**
 * @brief Initializes serial communication, LCD display, HX711 scale, and performs taring.
 * 
 * The user has 5 seconds to remove all objects before the tare is applied.
 */
void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize I2C with custom pins for ESP32
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize hardware components
  initializeScale();
  initializeLCD();

  // Initialize timing
  lastReading = millis();

  Serial.println("System initialized and ready.");
}

/**
 * @brief Main loop that reads from the scale and displays estimated object counts.
 * 
 * Checks for tare button press and processes scale readings at regular intervals.
 * The scale attempts to calculate the combination of factory and reused objects
 * based on their weights and tolerance. Results are shown on the LCD display.
 */
void loop() {
  // Check if it's time for a new reading
  if (millis() - lastReading >= READING_INTERVAL) {
    lastReading = millis();

    // Handle tare button press
    handleTareButton();

    // Process scale reading and update display
    processScaleReading();
  }
}
