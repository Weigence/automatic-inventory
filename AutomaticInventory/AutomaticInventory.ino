#include "HX711.h"

/// @brief Pin connected to HX711 DOUT (data out)
const int LOADCELL_DOUT_PIN = 2;

/// @brief Pin connected to HX711 SCK (clock)
const int LOADCELL_SCK_PIN = 3;

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
  Serial.begin(57600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  scale.set_scale(366); ///< Adjust this value based on your calibration
  Serial.println("Calibrating... Please remove the object.");
  delay(5000); ///< Wait for user to remove the object
  scale.tare(); ///< Set the current weight as zero
  Serial.println("Scale tared.");
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
  if (scale.is_ready()) {
    Serial.print("Place the object... ");
    delay(1000);

    long reading = scale.get_units(10); ///< Take average of 10 readings
    Serial.print("Weight: ");
    Serial.println(reading);

    long how_many = reading / WEIGHT_OF_OBJ; ///< Estimate object count
    Serial.print("Estimated number of objects: ");
    Serial.println(how_many);
  } else {
    Serial.println("HX711 not found.");
  }

  delay(1000); ///< Delay between measurements
}
