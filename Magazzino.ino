#include "HX711.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 2;
const int LOADCELL_SCK_PIN = 3;

#define WEIGHT_OF_OBJ 70

HX711 scale;

void setup() {
  Serial.begin(57600);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  scale.set_scale(366);    
  Serial.println("Sto tarando... rimuovi l oggetto.");
  delay(5000);
  scale.tare();
  Serial.println("Tarato");
}

void loop() {

  if (scale.is_ready()) {
    
    Serial.print("Metti l oggetto. ");
    delay(1000);
    long reading = scale.get_units(10);
    Serial.print("Peso: ");
    //fare l if per aumentare la calibrazione per gli oggetti che si ossono riutilizzare
    Serial.println(reading);
    long how_many = reading / WEIGHT_OF_OBJ;
    Serial.print("Ci sono : ");
    Serial.println(how_many);
  } 
  else {
    Serial.println("HX711 not found.");
  }
  delay(1000);
}
