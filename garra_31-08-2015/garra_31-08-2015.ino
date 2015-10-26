#include <Servo.h> 
Servo base;  // create servo object to control a servo 
Servo garra;                // a maximum of eight servo objects can be created 
int pos = 70;    // variable to store the servo position 
int pos2 = 53; 
void setup() 
{ 
 Serial.begin(9600);
 base.write(pos);
 garra.write(pos2);
} 
 
void loop() 
{ 
  base.attach(9);
   garra.attach(11);
   char var = Serial.read();
   
   if (var == '1')
   {
      for(pos; pos < 155; pos += 1)   
      {  
		base.write(pos);              
		delay(50); 
		char var = Serial.read();
		if (var == '0')
		{
			break; 
		}

	  }
  base.detach();
   }
   else if (var == '2')
   {
     for(pos; pos > 0; pos -= 1)   
     {
		base.write(pos);
		delay(50);    
        char var = Serial.read();
		if (var == '0')
		{
			break;
		}		
   }
 base.detach();  
 }
   if (var == '3')
   {
     for(pos2; pos2 < 55; pos2 += 1)   
     {
		garra.write(pos2);
		delay(100);    
                char var = Serial.read();
		if (var == '0')
		{
			break;
		}		
   }
   garra.detach();
   }
   
   else if (var == '4')
   {
     for(pos2; pos2 > 5; pos2 -= 1)   
     {
		garra.write(pos2);
		delay(50);    
                char var = Serial.read();
		if (var == '0')
		{
			break;
		}		
   }
   garra.detach();
   }
    Serial.println(var);

}
