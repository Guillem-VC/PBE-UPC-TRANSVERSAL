#include <Servo.h>

Servo servo;
int angle = 0;

void setup() {
  servo.attach(8);
  servo.write(angle);
  for(angle = 0; angle <70 ; angle++)  //modificar per trobar els angles exactes
  {                                  
    servo.write(angle); 
    delay(10);    
  }
  delay(2000);

  for(angle = 70; angle > 10; angle--)    
  {                                
    servo.write(angle);               
  } 
}

//al codi general-> if(color detectat){setupservo();
//un cop acabat enviar missatge que digui "càrrega entregada"

void loop() 
{ 
  
}
