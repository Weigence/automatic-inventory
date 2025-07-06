/**
 * @file AutomaticInventory.ino
 * @brief Automatic inventory system using HX711 load cell and LCD display
 * @authors Antonio Bernardini and Giulio Bucchi
 * @date June 2025
 * 
 * This system weighs objects and determines the count of factory items
 * based on their known weight. Results are displayed on an I2C LCD with
 * error detection and buzzer notification for unrecognized weights.
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
const uint8_t LCD_ROWS = 4;

/// @brief Number of LCD columns
const uint8_t LCD_COLS = 20;

/// @brief Pin connected to HX711 DOUT (data out)
const int LOADCELL_DOUT_PIN = 18;

/// @brief Pin connected to HX711 SCK (clock)
const int LOADCELL_SCK_PIN = 19;

/// @brief Pin for manual tare button
const int TARE_BUTTON_PIN = 27;

/// @brief Pin for buzzer output
const int BUZZER_BUTTON_PIN = 26;

/// @brief Custom SDA pin for ESP32 I2C
const int SDA_PIN = 21;

/// @brief Custom SCL pin for ESP32 I2C
const int SCL_PIN = 22;

// =============================================================================
// MEASUREMENT CONFIGURATION
// =============================================================================

/// @brief Weight of a factory (new) object in scale units
const int WEIGHT_OF_FABRIC_OBJ = 70;

/// @brief Accepted tolerance when matching weights
const int EPSILON = 3;

/// @brief Scale calibration factor
const float SCALE_CALIBRATION_FACTOR = 366.0;

/// @brief Reading interval in milliseconds
const unsigned long READING_INTERVAL = 200;

/// @brief Tare delay in milliseconds
const unsigned long TARE_DELAY = 5000;

/// @brief Number of readings to average
const int SCALE_READINGS = 15;

/// @brief Buzzer frequency for error notification
const int BUZZER_FREQUENCY = 4000;

// =============================================================================
// GLOBAL OBJECTS AND VARIABLES
// =============================================================================

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
HX711 scale;

/// @brief Timestamp for managing reading interval
uint32_t lastReading;

/// @brief Flag to track buzzer state
bool isBuzzerOn = false;

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
 * @brief Initializes the buzzer pin
 */
void initializeBuzzer() {
  pinMode(BUZZER_BUTTON_PIN, OUTPUT);
  noTone(BUZZER_BUTTON_PIN);
  isBuzzerOn = false;
}

/**
 * @brief Initializes the tare button pin
 */
void initializeTareButton() {
  pinMode(TARE_BUTTON_PIN, INPUT);
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
 * @brief Calculates the number of fabric objects based on weight reading
 * @param reading The weight reading from the scale
 * @param count_fabric Reference to store fabric object count
 * @return true if a valid count is found within tolerance, false otherwise
 */
bool calculateFabricObjectCount(long reading, int& count_fabric) {
  count_fabric = round((float)reading / WEIGHT_OF_FABRIC_OBJ);
  long expectedWeight = count_fabric * WEIGHT_OF_FABRIC_OBJ;
  long delta = abs(reading - expectedWeight);
  
  // Check if the reading is within tolerance
  return (delta <= EPSILON * count_fabric);
}

/**
 * @brief Activates the buzzer for error notification
 */
void activateBuzzer() {
  if (!isBuzzerOn) {
    tone(BUZZER_BUTTON_PIN, BUZZER_FREQUENCY);
    isBuzzerOn = true;
  }
}

/**
 * @brief Deactivates the buzzer
 */
void deactivateBuzzer() {
  if (isBuzzerOn) {
    noTone(BUZZER_BUTTON_PIN);
    isBuzzerOn = false;
  }
}

/**
 * @brief Displays fabric object count on LCD
 * @param count_fabric Number of fabric objects
 */
void displayFabricCount(int count_fabric) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fabbrica: ");
  lcd.print(count_fabric);
}

/**
 * @brief Displays error message on LCD
 */
void displayError() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERRORE PESO");
  lcd.setCursor(0, 1);
  lcd.print("Non riconosciuto");
}

/**
 * @brief Displays tare completion message on LCD
 */
void displayTareComplete() {
  lcd.clear();
  lcd.print("TARA ESEGUITA");
  delay(500);
  lcd.clear();
}

/**
 * @brief Displays automatic tare message on LCD
 */
void displayAutoTare() {
  lcd.clear();
  lcd.print("TARA AUTOMATICA");
  delay(500);
  lcd.clear();
}

/**
 * @brief Handles the tare button press and scale taring
 */
void handleTareButton() {
  if (digitalRead(TARE_BUTTON_PIN) == HIGH) {
    scale.tare();
    Serial.println("Tara eseguita.");
    displayTareComplete();
    deactivateBuzzer();
  }
}

/**
 * @brief Processes the scale reading and updates display
 */
void processScaleReading() {
  if (!scale.is_ready()) {
    return;
  }

  Serial.print("Peso letto: ");
  long reading = scale.get_units(SCALE_READINGS);
  Serial.println(reading);

  // Auto-tare if negative reading is detected
  if (reading < 0) {
    scale.tare();
    Serial.println("Peso negativo -> tara automatica.");
    displayAutoTare();
    deactivateBuzzer();
    return;
  }

  int count_fabric;
  bool isValidWeight = calculateFabricObjectCount(reading, count_fabric);

  // First check - if weight is within tolerance
  if (isValidWeight) {
    Serial.print("Numero stimato: ");
    Serial.println(count_fabric);
    displayFabricCount(count_fabric);
    deactivateBuzzer();
  } else {
    reading = scale.get_units(SCALE_READINGS);
    Serial.print("Peso di verifica: ");
    Serial.println(reading);
    
    isValidWeight = calculateFabricObjectCount(reading, count_fabric);
    
    if (isValidWeight) {
      Serial.print("Numero stimato dopo verifica: ");
      Serial.println(count_fabric);
      displayFabricCount(count_fabric);
      deactivateBuzzer();
    } else {
      Serial.println("Errore confermato.");
      displayError();
      activateBuzzer();
    }
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
  initializeBuzzer();
  initializeTareButton();

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
