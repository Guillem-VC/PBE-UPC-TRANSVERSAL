// LLIBRERIES

  // Transceptor NRF24L01
    #include <SPI.h>
    #include <nRF24L01.h>
    #include <RF24.h>

// CONSTANTS

  // Transceptor NRF24L01
    #define CE_PIN   9 // Pin 9 de l'Arduino Uno
    #define CSN_PIN 10 // Pin 10 de l'Arduino Uno
    const byte slaveAddress[5] = {'R','x','A','A','A'};
    const byte masterAddress[5] = {'T','X','a','a','a'};
    RF24 radio(CE_PIN, CSN_PIN); // Crear una ràdio

  // Sensor de Motor i Línia  
    #define INTERSECCIO 0 
    #define RECTE 1 
    #define GIR_SUAU_ESQUERRA 2
    #define GIR_SUAU_DRETA 3
    #define GIR_FORT_ESQUERRA 4
    #define GIR_FORT_DRETA 5

  // Sensor de Color
    #define BLANC 0
    #define VERMELL 1
    #define GROC 2
    #define BLAU 3 
    #define VERD 3
    #define NEGRE 5
  

  // Estats
    #define INICI 0 
    #define AVANCAR 1
    #define COMPARAR_COLOR 2
    #define DEIXAR_CARREGA 3
    #define FINAL 4

  // Altres
    #define ARRAY_SIZE_RX 2
    
// VARIABLES

  int dataReceived[ARRAY_SIZE_RX]; // Ha de coincidir amb el tamany de les dades enviades del "master"
  // int replyData; 
  bool newData = false;

  // Estats
    int estatRebut;
    int colorRebut;
    int direccioRebuda;
    int nouEstat;

  // Altres
    unsigned long currentMillis;
    unsigned long prevMillis;
    unsigned long txIntervalMillis = 500; // Enviar ua vegada cada 0,5 segons

  // Color
    int colorTriat;
    int colorCorrecte;

// ================

// SETUP

  void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println();
    Serial.print("Iniciant Receptor"); 
    delay(500); 
    Serial.print(".");
    delay(500);
    Serial.print("..");
    delay(500);
    Serial.print("..");
    delay(500);
    Serial.println("..");
    delay(500);    

    radio.begin();
    radio.setDataRate( RF24_250KBPS );
    radio.openWritingPipe(masterAddress); 
    radio.openReadingPipe(1, slaveAddress);
    radio.setRetries(3,5); // delay, compte
    radio.startListening();

    // Escollir el color
      Serial.print("Escollir color:    1 - VERMELL    2 - GROC    3 - BLAU");
      while(Serial.available()== false){
        // S'espera
      }
      colorTriat = Serial.parseInt();
      Serial.println();
      Serial.print("Color Triat: ");
      Serial.println(colorTriat);
      delay(1000); // Per poder veure clarament la informació que s'ha escrit pel Serial Monitor
  }

//====================

// LOOP

  void loop() {
    getData(); // Obté les dades enviades pel cotxe
    showData(); // Ensenya les dades rebudes (en cas d'haver-hi)
    send(); // Conté la funció updateReplyData() 
    delay(500);
  }

    // Enviar Dades (conté updateReplyData())
      void send() {
        if (newData == true) {
          radio.stopListening();
          bool rslt;
          rslt = radio.write( &colorTriat, sizeof(colorTriat) );
          radio.startListening();
            if (rslt) {
              // Serial.print("Color Enviat: ");
              // // Serial.print(colorTriat);
              // if(colorTriat == 0){
              //   Serial.print("Blanc");
              // } else if(colorTriat == 1){
              //   Serial.print("Vermell");
              // } else if(colorTriat == 2){
              //   Serial.print("Groc");
              // } else if(colorTriat == 3){
              //   Serial.print("Blau");
              // } else if(colorTriat == 4){
              //   Serial.print("Verd");
              // } else if(colorTriat == 5){
              //   Serial.print("Negre");
              // }
              // updateReplyData();
            }
            else {
              Serial.println("Tx fallida");
            }
            Serial.println();
            newData = false;
          }
      }

      // // Actualitzar Dades a Enviar (S'encarrega de fer la comparació de colors i enviar l'estat DEIXAR_CARREGA quan el color és el demanat)
      //   void updateReplyData() {
      //     replyData = colorTriat;             
      //   }

    // Obtenir Dades
      void getData() {     
        if ( radio.available() ) {
          radio.read( &dataReceived, sizeof(dataReceived) );
          newData = true;
          colorRebut = dataReceived[0];
          direccioRebuda = dataReceived[1];
        } else if(newData == false){
        Serial.println("    **** No es reben dades ****"); // No es reben dades
        }
      }
    
    // Ensenyar Dades
      void showData() {
        if (newData == true) {
          Serial.print("Dades Rebudes: "); 

          // // Estat 
          //   Serial.print("    Estat: ");
          //     if(estatRebut == 0){
          //       Serial.print("INICI");
          //     } else if(estatRebut == 1){
          //       Serial.print("AVANÇAR");
          //     } else if(estatRebut == 2){
          //       Serial.print("COMPARAR COLOR");
          //     } else if(estatRebut == 3){
          //       Serial.print("DEIXAR CARREGA");
          //     } else if(estatRebut == 4){
          //       Serial.print("FINAL");
          //     }    
          //     Serial.print(estatRebut);    

          // Color
            Serial.print("Color: ");
              if(colorRebut == 0){
                Serial.print("Blanc");
              } else if(colorRebut == 1){
                Serial.print("Vermell");
              } else if(colorRebut == 2){
                Serial.print("Groc");
              } else if(colorRebut == 3){
                Serial.print("Blau");
              } else if(colorRebut == 4){
                Serial.print("Verd");
              } else if(colorRebut == 5){
                Serial.print("Negre");
              }
          // Direcció
            Serial.print("    Direccio: ");
              if(direccioRebuda >= 2900 && direccioRebuda <= 3600){ // 
                Serial.println("Recte");
              } else if(direccioRebuda >= 0 && direccioRebuda <= 2900){
                Serial.println("Dreta");
              } else if(direccioRebuda >= 3600 && direccioRebuda <= 7000){ // He canviat 4000 --> 3800
                Serial.println("Esquerra");

              }                                   
              // if(direccioRebuda >= 3000 && direccioRebuda <= 3800){ // hE CANVIAT 4000 --> 3800
              //   Serial.println("Recte");
              // } else if(direccioRebuda >= 1500 && direccioRebuda <= 3000){
              //   Serial.println("Dreta_Suau");
              // } else if(direccioRebuda >= 0 && direccioRebuda <= 1500){
              //   Serial.println("Dreta_Fort");
              // } else if(direccioRebuda >= 3600 && direccioRebuda <= 5500){ // He canviat 4000 --> 3800
              //   Serial.println("Esquerra_Suau");
              // } else if(direccioRebuda >= 5500 && direccioRebuda <= 7000){
              //   Serial.println("Esquerra_Fort");
              // }                       
            Serial.println();
        }
      }
