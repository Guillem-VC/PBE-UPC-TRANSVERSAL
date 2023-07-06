
// LLIBRERIES

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>
#include <RF24_config.h>

#include <ColorConverterLib.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"

//================

#define CE_PIN   9
#define CSN_PIN 10

const byte slaveAddress[5] = {'R','x','A','A','A'};
const byte masterAddress[5] = {'T','X','a','a','a'};

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

char data[16];
char dataToSend[16];
// char txNum = '0';
char dataReceived[16]; // to hold the data from the slave - must match replyData[] in the slave
bool newData = false;

unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 500; // send once per second

//================

//SETUP

void setup() {
  Serial.begin(9600);
  setupSensorColor(); 
  setupSensorComunicacions(); // ha de ser l'ultima funció de  totes
}

void setupSensorColor() {
  if (!tcs.begin()) {
    Serial.println("Error al iniciar TCS34725");
    while (1) delay(500);
  }
}

    void setupSensorComunicacions() {
      Serial.println("Iniciant Programa"); //    Serial.println("MasterSwapRoles Starting");
    
      radio.begin();
      radio.setDataRate( RF24_250KBPS );
    
      radio.openWritingPipe(slaveAddress);
      radio.openReadingPipe(1, masterAddress);
    
      radio.setRetries(3,5); // delay, count
      send(); // to get things started
      prevMillis = millis(); // set clock
    }

  
// LOOP

void loop() {
  loopSensorComunicacions();
//  loopSensorColor();
}

void loopSensorComunicacions() {
  currentMillis = millis();
  if (currentMillis - prevMillis >= txIntervalMillis) {
    send();
    prevMillis = millis();
  }
  getData();
  showData();
}


// ENVIAR

void send() {

        radio.stopListening();
            bool rslt;
            rslt = radio.write( &dataToSend, sizeof(dataToSend) );
        radio.startListening();
        Serial.print("Dades enviades ");
        Serial.print(dataToSend);
        if (rslt) {
            Serial.println("  Acknowledge rebut");
            updateMessage();
        }
        else {
            Serial.println("  Tx fallida");
        }
}


// OBTENIR DADES

void getData() {

    if ( radio.available() ) {
        radio.read( &dataReceived, sizeof(dataReceived) );
        newData = true;
    }
}


// ENSENYAR DADES

void showData() {
    if (newData == true) {
        Serial.print("Dades rebudes ");
        Serial.println(dataReceived);
//        Serial.print(", ");
//        Serial.println(dataReceived[1]);
//        Serial.println();
          newData = false;
    }
}

// ACTUALITZAR MISSATGE

void updateMessage() { // DATA A ENVIAR PER PART DELS SENSORS
        // so you can see that new data is being sent
//    txNum += 1;
//    if (txNum > '9') {
//       txNum = '0';
//  }
//     
  SensorColor();
  strcpy(dataToSend, data);
}


// SENSOR COLOR (FUNCIONS)

void SensorColor() {
  uint16_t clear, red, green, blue;

  tcs.setInterrupt(false);
  delay(60); // Cuesta 50ms capturar el color
  tcs.getRawData(&red, &green, &blue, &clear);
  tcs.setInterrupt(true);

  // Hacer rgb medición relativa
  uint32_t sum = clear;
  float r, g, b;
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;

  // Escalar rgb a bytes
  r *= 256; g *= 256; b *= 256;

  // Convertir a hue, saturation, value
  double hue, saturation, value;
  ColorConverter::RgbToHsv(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), hue, saturation, value);

  // Mostrar nombre de color
  printColorName(hue * 360, saturation, value *  100);

  delay(50);
}

void printColorName(double hue, double saturation, double value){
  if(saturation <= 0.20){
    saturation = 0;
  } else{
    saturation = 1;
  }
  
 if(((hue > 0 && hue < 50) && (saturation == 0)) && (value > 35)){
    strcpy(data, "Blanco");
  } else if((hue == 0 && hue < 50) || (hue == 120.00) || (hue == 180.00) ||  (hue == 150.00)){
    strcpy(data, "Negro");
  } else if(((hue > 0 && hue < 50) && (saturation == 1)) && (value > 35)){
    strcpy(data, "Rojo");
  } else if(((hue > 50 && hue < 100) && (saturation == 1)) && (value > 35)){
    strcpy(data, "Amarillo");
  } else if(((hue > 100 && hue < 175) && (saturation == 1)) && (value > 35)){
    strcpy(data, "Verde");
  }else if(((hue > 200) && (saturation == 1)) || (value >  35)){
    strcpy(data, "Azul");
  }
}
