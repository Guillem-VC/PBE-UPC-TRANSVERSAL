#include <QTRSensors.h>
#define MOT_A1_PIN 5
#define MOT_A2_PIN 4
#define MOT_B1_PIN 2
#define MOT_B2_PIN 3

QTRSensors qtr;

const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
int base_iz = 60; // Velocidades base
int base_der = 60;
int correccion = 0;  // factor de corrección de las velocidades
int error = 0; // El error que es calculado cada vez
int error_anterior = 0;
float Kp =0.5;//1.2;//0.3;   // El factor proporcional, hay que ajustarlo en el valor ideal  //2
float Ki =0;//0.1;   // El factor integral, hay que ajustarlo                              //10000 //16000
float Kd =0.9;//0.5;//0.5;   // El factor derivativo que hay que ajustar                     //14  //7
int integral = 0; // la integral que va acumulando los errores
int derivativa = 0; // La derivativa calcula el incremento del error
int leftMotorSpeed; // las velocidades de los motores
int rightMotorSpeed;  
int vel_maxima = 120;
int vel_minima = 0;
int sumaValors= 0;

void setup()
{
   pinMode(13, OUTPUT);  // LED para indicar que está calibrando
 Serial.begin(9600);
   //Setup Channel A
  pinMode(MOT_A1_PIN, OUTPUT); //Initiates Motor Channel A pin
  pinMode(MOT_A2_PIN, OUTPUT); //Initiates Brake Channel A pin
  //Setup Channel B
  pinMode(MOT_B1_PIN, OUTPUT); //Initiates Motor Channel A pin
  pinMode(MOT_B2_PIN, OUTPUT);  //Initiates Brake Channel A pin
  analogWrite(MOT_A1_PIN, LOW); 
  analogWrite(MOT_B1_PIN, LOW); 
  
   Serial.println("Calibrando");
  // Configuración de los sensores. Aquí ya usamos los que quedan libres del motor shield.
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){46,47,48,49,50,51,52,53}, SensorCount);
  qtr.setEmitterPin(16);

  delay(500);

  digitalWrite(13, HIGH); // Encendemos un led conectado a A3 para indicar que estamos calibrando

  // 2.5 ms RC read timeout (default) * 10 reads per calibrate() call
  // = ~25 ms per calibrate() call.
  // Call calibrate() 400 times to make calibration take about 10 seconds.
  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
  }
  digitalWrite(13, LOW); // Ya ha calibrado
  // Imprimir los valores mínimos obtenidos cuando se calibraba:
  
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(qtr.calibrationOn.minimum[i]);
    Serial.print(' ');
  }
  Serial.println();

  // Imprimir los valores máximos obtenidos al calibrar:
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(qtr.calibrationOn.maximum[i]);
    Serial.print(' ');
  }
  Serial.println();
  Serial.println();
  //delay(1000);

}
void loop()
{
  sumaValors = (sensorValues[0]+sensorValues[1]+sensorValues[2]+sensorValues[3]+sensorValues[4]+sensorValues[5]+
  sensorValues[6]+sensorValues[7]);

  if(sumaValors>=5000){
   analogWrite(MOT_A1_PIN, 0);
   analogWrite(MOT_A2_PIN, 0);
   delay(4000);
  }
  avancar();
   
} 
void avancar(){
  uint16_t position = qtr.readLineBlack(sensorValues);

  // Pintar los valores de los sensores de 0 a 1000 donde 1000 significa negro y 0 blanco
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }

  Serial.println(position);
  Serial.println(" | \t");

  error = position - 3500;
  error = error/20; //quant mes alt el factor divisor mes be va

  integral = integral + error; // La integral
  derivativa = error - error_anterior; // La derivativa
  correccion = Kp * error + Ki * integral + Kd * derivativa;
  //velocidad_recta = diferencia*exp(-Kv*abs(Kp*error));  // a más error, mucha menos velocidad (per anar mes rapid a la recta)
  //if(Kv == 0) velocidad_recta=0;
  leftMotorSpeed = base_iz + correccion;
  rightMotorSpeed  = base_der - correccion;

  error_anterior = error; //per a la derivada
  /////// La parte de frenar y limitar las velocidades
  if(leftMotorSpeed > vel_maxima) { leftMotorSpeed = vel_maxima; digitalWrite(13, HIGH);} //led
  if(rightMotorSpeed > vel_maxima) { rightMotorSpeed = vel_maxima; digitalWrite(13, HIGH);}
  digitalWrite(MOT_B2_PIN,LOW); // Quitamos el freno antes de decidir si lo ponemos
  if(leftMotorSpeed <= vel_minima) { leftMotorSpeed = vel_minima; digitalWrite(13,HIGH);  /*digitalWrite(A3, HIGH);*/} // El freno
  digitalWrite(MOT_A2_PIN,LOW); // Quitamos el freno antes de decidir si lo ponemos
  if(rightMotorSpeed <= vel_minima) { rightMotorSpeed = vel_minima; digitalWrite(13,HIGH);  /*digitalWrite(A3, HIGH);*/} // El freno
  
  analogWrite(MOT_A1_PIN, rightMotorSpeed);    //Spins the motor on Channel A 
  analogWrite(MOT_B1_PIN, leftMotorSpeed);   //Spins the motor on Channel B 
}
  //delay(250);