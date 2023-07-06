// LLIBRERIES
  // Transceptor NRF24L01
    #include <SPI.h>
    #include <printf.h>
    #include <nRF24L01.h>
    #include <RF24_config.h>
    #include <RF24.h>
  
  // Sensor de Color
    #include <Wire.h>
    #include "Adafruit_TCS34725.h"
    #include <ColorConverterLib.h>

  // Sensor de Línia
    #include <QTRSensors.h>
    
  //Servo
    #include <Servo.h>

// CONSTANTS

  // Sensor de Línia
    #define MOT_A1_PIN 5
    #define MOT_A2_PIN 4
    #define MOT_B1_PIN 2
    #define MOT_B2_PIN 3

    #define INTERSECCIO 0 
    #define RECTE 1 
    #define GIR_SUAU_ESQUERRA 2
    #define GIR_SUAU_DRETA 3
    #define GIR_FORT_ESQUERRA 4
    #define GIR_FORT_DRETA 5

    #define INTERSECCIONS_TOTALS 4 // NOMBRE D'INTERSECCIONS TOTALS

    QTRSensors qtr;

    const uint8_t SensorCount = 8;
    uint16_t sensorValues[SensorCount];
    int base_iz = 40; // Velocidades base
    int base_der = 40;
    int correccion = 0;  // factor de corrección de las velocidades
    int error = 0; // El error que es calculado cada vez
    int error_anterior = 0;
    float Kp =0.5;//1.2;//0.3;   // El factor proporcional, hay que ajustarlo en el valor ideal  
    float Ki =0;//0.1;   // El factor integral, hay que ajustarlo                              
    float Kd =0.9;//0.5;//0.5;   // El factor derivativo que hay que ajustar                     
    int integral = 0; // la integral que va acumulando los errores
    int derivativa = 0; // La derivativa calcula el incremento del error
    int leftMotorSpeed; // las velocidades de los motores
    int rightMotorSpeed;  
    int vel_maxima = 90;
    int vel_minima = 0;
    int sumaValors = 0;  
    int interseccions = 0;
  
  // Transceptor NRF24L01
    #define CE_PIN   48
    #define CSN_PIN 53
    const byte slaveAddress[5] = {'R','x','A','A','A'};
    const byte masterAddress[5] = {'T','X','a','a','a'};
    RF24 radio(CE_PIN, CSN_PIN); // Crear una ràdio
  

  // Sensor de Color
    Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);  
    #define BLANC 0
    #define NEGRE 5
    #define VERMELL 1
    #define GROC 2
    #define BLAU 3
    #define VERD 4
    


   // Estats
    #define INICI 0
    #define AVANCAR 1
    #define COMPARAR_COLOR 2
    #define DEIXAR_CARREGA 3
    #define FINAL 4

   // Altres
    #define ARRAY_SIZE_TX 2

// VARIABLES

  // Transceptor NRF24L01
    int dataToSend[ARRAY_SIZE_TX]; // HEM DE MIRAR COM HO FEM PER ENVIAR DIVERSES INFORMACI0NS!!!
    // int estatEnviat;
    int colorRebut; // to hold the data from the slave - must match replyData[] in the slave
    int dataSensorColor;
    int dataSensorMotorLinia;
  
    bool newData = false;
  
  // Servo 
    Servo servo;
    int angle = 0;
    bool final = false; //NOU
    bool carregaDeixada = false;
  
  // Altres
    unsigned long currentMillis;
    unsigned long prevMillis;
    unsigned long txIntervalMillis = 500; // Enviar cada 500ms (0.5 segons)
    bool circuitCompletat = false;;

//========================

// SETUP

  void setup() {
    Serial.begin(9600);
    // servo.attach(8);    
    // servo.write(105);
    // servo.detach();
    setupSensorColor();       
    setupCalibracioMotors();
    setupSensorComunicacions();
  }

    // Setup Calibració de Motors    
      void setupCalibracioMotors() {
        pinMode(13, OUTPUT);  // LED para indicar que está calibrando
        //Setup Channel A
          pinMode(MOT_A1_PIN, OUTPUT); //Initiates Motor Channel A pin
          pinMode(MOT_A2_PIN, OUTPUT); //Initiates Brake Channel A pin
        //Setup Channel B
          pinMode(MOT_B1_PIN, OUTPUT); //Initiates Motor Channel A pin
          pinMode(MOT_B2_PIN, OUTPUT);  //Initiates Brake Channel A pin
          analogWrite(MOT_A1_PIN, LOW); 
          analogWrite(MOT_B1_PIN, LOW); 
        
        Serial.println("Calibrant");
        // Configuración de los sensores. Aquí ya usamos los que quedan libres del motor shield.
          qtr.setTypeRC();
          qtr.setSensorPins((const uint8_t[]){24,26,28,30,32,34,36,38}, SensorCount);
          qtr.setEmitterPin(16);
      
        delay(500);
      
        digitalWrite(13, HIGH); // Encendemos un led conectado a A3 para indicar que estamos calibrando
      
        //calibration take about 10 seconds.
        for (uint16_t i = 0; i < 400; i++) {
          qtr.calibrate();
        }
        digitalWrite(13, LOW); // Ya ha calibrado
        // Imprimir los valores mínimos obtenidos cuando se calibraba          (NO ES NECESSARI SI VOLEM)
        for (uint8_t i = 0; i < SensorCount; i++) {
          Serial.print(qtr.calibrationOn.minimum[i]);
          Serial.print(' ');
        }
        Serial.println();
        // Imprimir los valores máximos obtenidos al calibrar:
        for (uint8_t i = 0; i < SensorCount; i++) {
          Serial.print(qtr.calibrationOn.maximum[i]);
          Serial.print(' ');
        }
        Serial.println();
        Serial.println();
        //delay(1000);
      }

    // Setup Sensor de Color   
      void setupSensorColor() {
        if (!tcs.begin()) {
          Serial.println("Error al iniciar TCS34725");
          while (1) delay(500);
        }
      }

    // Setup Sensor de Comunicacions (NRF24L01)
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

//========================

// LOOP

  void loop() {
    currentMillis = millis();
    if (currentMillis - prevMillis >= txIntervalMillis) {
    send();
    prevMillis = millis();
    }
    getData();
    
    seguirLinia(); // Prova
    sumaValors = (sensorValues[0]+sensorValues[1]+sensorValues[2]+sensorValues[3]+sensorValues[4]+sensorValues[5]+sensorValues[6]+sensorValues[7]);

    // Intersecció
    if(sumaValors>=6000){ // INTERSECCIO (Atura els motors)
      analogWrite(MOT_B1_PIN, 0);
      analogWrite(MOT_A1_PIN, 0);
      analogWrite(MOT_B2_PIN, 0);
      analogWrite(MOT_A2_PIN, 0);
      delay(1000);

      sensorColor();

      // Inici o Final (es troba a la intersecció verda)
      
        if(dataSensorColor == VERD){
          if(circuitCompletat == false){
            while((colorRebut != BLAU) && (colorRebut != GROC) && (colorRebut != VERMELL)){
            // delay(250);
            currentMillis = millis();
            if (currentMillis - prevMillis >= txIntervalMillis) {
            send();
            prevMillis = millis();
            }
            getData();
            // getData();
            Serial.print(colorRebut);
            }
          } else if(circuitCompletat == true){
              while(dataSensorColor == VERD){
                analogWrite(MOT_A1_PIN, 0);
                analogWrite(MOT_B1_PIN, 0);
                analogWrite(MOT_B2_PIN, 0);
                analogWrite(MOT_A2_PIN, 0);             
              }        
          }

        }
      
      // // Final (ja ha deixat la càrrega) HO HAUREM DE TREURE
      //   if(interseccions == 4 && final==true){ // FINAL (Ha deixat la càrrega i arribat al punt verd. S'espera)
      //     // estatEnviat=FINAL;
      //     delay(100000);
      //   } 

      // Detecta un color incorrecte
        else if(dataSensorColor != colorRebut){
          interseccions++;
        }

      // Detecta el color  correcte  
        else if(dataSensorColor == colorRebut){
          sortirCami();
          circuitCompletat = true;
        }

      // Petita embranzida després de la intersecció  
        analogWrite(MOT_B1_PIN, 70);
        analogWrite(MOT_A1_PIN, 70);
        delay(250);          
    }
    

}


  //   if(estatRebut == AVANCAR && carregaDeixada == false){

  //     sumaValors = (sensorValues[0]+sensorValues[1]+sensorValues[2]+sensorValues[3]+sensorValues[4]+sensorValues[5]+sensorValues[6]+sensorValues[7]);
  //     if(sumaValors>=6000){ // INTERSECCIO (Atura els motors)
  //       analogWrite(MOT_B1_PIN, 0);
  //       analogWrite(MOT_A1_PIN, 0);
  //       analogWrite(MOT_B2_PIN, 0);
  //       analogWrite(MOT_A2_PIN, 0);        
  //       estatEnviat = COMPARAR_COLOR;
  //       interseccions = interseccions + 1;
  //       delay(2000);

  //       //seguirLinia();
  //     }
  //       seguirLinia(); 
  //   } 

  //   if(estatRebut == AVANCAR && carregaDeixada == true){

  //     sumaValors = (sensorValues[0]+sensorValues[1]+sensorValues[2]+sensorValues[3]+sensorValues[4]+sensorValues[5]+sensorValues[6]+sensorValues[7]);
  //     if(interseccions == INTERSECCIONS_TOTALS){
  //       analogWrite(MOT_A1_PIN, 0);
  //       analogWrite(MOT_B1_PIN, 0);
  //       analogWrite(MOT_B2_PIN, 0);
  //       analogWrite(MOT_A2_PIN, 0);
  //       estatEnviat = FINAL;
  //       delay(100000000); // Es para durnt 100 segons
  //     }
  //     if(sumaValors>=6000){ // INTERSECCIO (Atura els motors)
  //       analogWrite(MOT_A1_PIN, 0);
  //       analogWrite(MOT_B1_PIN, 0);
  //       analogWrite(MOT_B2_PIN, 0);
  //       analogWrite(MOT_A2_PIN, 0);
  //       estatEnviat = AVANCAR;
  //       interseccions = interseccions + 1;
  //       delay(1000);  
  //     }
  //     seguirLinia();
  //   }

  //   if(estatRebut == COMPARAR_COLOR){
  //     sensorColor();  
  //     estatEnviat = COMPARAR_COLOR;    
  //   }

  //   if(estatRebut == DEIXAR_CARREGA){
  //     // FER L'ACCIÓ DE DEIXAR LA CÀRREGA
  //     sortirCami();
  //     carregaDeixada = true;
  //     estatEnviat = AVANCAR;
  //   }
  

    // Loop Sensor Color   
      void sensorColor() {
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
      
        printColorName(hue * 360, saturation, value *  100);    

        //delay(100); // AJUSTAR
      }

        // Imprimir ColorName
        void printColorName(double hue, double saturation, double value) {
          if(saturation <= 0.20) {
            saturation = 0;
          } else{
            saturation = 1;
          }
          
          if(((hue > 0 && hue < 50) && (saturation == 0)) && (value > 35)) {
            dataSensorColor = BLANC; 
          } else if((hue == 0 && hue < 50) || (hue == 120.00) || (hue == 180.00) ||  (hue == 150.00)) {
            dataSensorColor = NEGRE; 
          } else if(((hue > 0 && hue < 50) && (saturation == 1)) && (value > 35)) {
            dataSensorColor = VERMELL; 
          } else if(((hue > 50 && hue < 100) && (saturation == 1)) && (value > 35)) {
            dataSensorColor = GROC; 
          } else if(((hue > 100 && hue < 175) && (saturation == 1)) && (value > 35)) {
            dataSensorColor = VERD; 
          } else if(((hue > 200) && (saturation == 1)) || (value >  35)) {
            dataSensorColor = BLAU; 
          }
          // Serial.println(dataSensorColor);
      }

    // Loop Motors i Sensor de Línia
    
      void seguirLinia() {
        uint16_t position = qtr.readLineBlack(sensorValues);

        // Pintar los valores de los sensores de 0 a 1000 donde 1000 significa negro y 0 blanco  ((NO CAL PRINTEJAR SI NO VOLEM))
        /*for (uint8_t i = 0; i < SensorCount; i++)
        {
          Serial.print(sensorValues[i]);
          Serial.print('\t');
        } */

        /*Serial.println(position);
        Serial.println(" | \t");*/

        error = position - 3500;
        error = error/20; //quant mes alt el factor divisor mes be va

        integral = integral + error; // La integral
        derivativa = error - error_anterior; // La derivativa
        correccion = Kp * error + Ki * integral + Kd * derivativa;

        leftMotorSpeed = base_iz + correccion;
        rightMotorSpeed  = base_der - correccion;

        error_anterior = error; //per a la derivada

        if(leftMotorSpeed > vel_maxima) { leftMotorSpeed = vel_maxima; digitalWrite(13, HIGH);} //led
        if(rightMotorSpeed > vel_maxima) { rightMotorSpeed = vel_maxima; digitalWrite(13, HIGH);}
        // digitalWrite(MOT_B2_PIN,LOW); // Quitamos el freno antes de decidir si lo ponemos
        if(leftMotorSpeed <= vel_minima) { leftMotorSpeed = vel_minima; digitalWrite(13,HIGH);  /*digitalWrite(A3, HIGH);*/} // El freno
        // digitalWrite(MOT_A2_PIN,LOW); // Quitamos el freno antes de decidir si lo ponemos
        if(rightMotorSpeed <= vel_minima) { rightMotorSpeed = vel_minima; digitalWrite(13,HIGH);  /*digitalWrite(A3, HIGH);*/} // El freno
        
        analogWrite(MOT_A1_PIN, rightMotorSpeed);    //Spins the motor on Channel A 
        analogWrite(MOT_B1_PIN, leftMotorSpeed);   //Spins the motor on Channel B 
/*
        if(3000<=position<4000){
          dataSensorMotorLinia = RECTE;
        }
        else if(1500<=position<3000){
          dataSensorMotorLinia = GIR_SUAU_DRETA; 
        }
        else if(0<=position<1500){
          dataSensorMotorLinia = GIR_FORT_DRETA;
        }
        else if(4000<=position<5500){
          dataSensorMotorLinia = GIR_SUAU_ESQUERRA;
        }
        else if(5500<=position<=7000){
          dataSensorMotorLinia = GIR_FORT_ESQUERRA;
        }*/

        dataSensorMotorLinia = position;
      }

    // Sensor de Comunicacions

      // Enviar Dades
        void send() {
          radio.stopListening();
          bool rslt;
          rslt = radio.write( &dataToSend, sizeof(dataToSend) );
          radio.startListening();
          if (rslt) {
            Serial.print("Dades enviades: [");
            Serial.print(dataToSend[0]);
            Serial.print(", ");
            Serial.print(dataToSend[1]);
            Serial.print("]");
            
            // Serial.println("    Dades processades correctament");
            updateMessage();
          }
          else {
              Serial.println("  Tx fallida");
          }
        }
    
      // Obtenir Dades
        void getData() {
          if (radio.available()) {
            radio.read( &colorRebut, sizeof(colorRebut) );
            newData = true;
            showData();
          }
        }
    
      // Ensenyar Dades
        void showData() {
          if (newData == true) {
            Serial.print("    Dades rebudes: ");
            Serial.println(colorRebut);
            Serial.println();
            newData = false;
          }
        }
    
      // Actualitzar Missatge
        void updateMessage() { // Data a enviar per part dels sensors 
          dataToSend[0] = dataSensorColor;
          dataToSend[1] = dataSensorMotorLinia; 
        }
//A esquerre 
    //Servo i sortir del cami
    void sortirCami(){
        // Enrere
        analogWrite(MOT_A1_PIN, 0);
        analogWrite(MOT_B1_PIN, 0);
        analogWrite(MOT_B2_PIN, 0);
        analogWrite(MOT_A2_PIN, 70);
        delay(700);
        // Aturat
        analogWrite(MOT_A1_PIN, 0);
        analogWrite(MOT_B1_PIN, 0);
        analogWrite(MOT_B2_PIN, 0);
        analogWrite(MOT_A2_PIN, 0);

        deixarCarrega();
        // Tornar al camí
        analogWrite(MOT_A1_PIN, 70);
        analogWrite(MOT_B1_PIN, 0);
        analogWrite(MOT_B2_PIN, 0);
        analogWrite(MOT_A2_PIN, 0);
        delay(500);
        // Pausa
        analogWrite(MOT_A1_PIN, 0);
        analogWrite(MOT_B1_PIN, 0);
        analogWrite(MOT_B2_PIN, 0);
        analogWrite(MOT_A2_PIN, 0);
        delay(700);
    }

    void deixarCarrega(){
      servo.attach(8);
      // servo.write(angle);
      // for(angle = 0; angle <70 ; angle++)  //modificar per trobar els angles exactes
      // {                                  
      //   servo.write(angle); 
      //   delay(10);    
      // }
      servo.write(105);
      delay(1000);
      servo.write(0);
      // for(angle = 70; angle > 10; angle--)    
      // {                                
      //   servo.write(angle);               
      // } 
      delay(1000);
      servo.write(105);
      delay(1000);
      servo.detach();
   }
    

