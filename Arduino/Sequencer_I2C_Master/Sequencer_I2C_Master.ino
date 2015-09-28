// Sequencer Board Control

#include <Wire.h>
#include <stdio.h>

#define TWI_SLAVE_ADDRESS	0x7f

byte note_n = 0;

void setup()
{
  pinMode(13, OUTPUT);
  
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(1);
  digitalWrite(2, HIGH);
  
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  Serial.println("Read Slave Data...");
}

void loop()
{
  // Reciever
  //
  Wire.requestFrom(TWI_SLAVE_ADDRESS, 6);    // request 6 bytes from slave device 0x7f

  while (Wire.available())                   // slave may send less than requested
  {
    int x = Wire.read();                     // receive a byte as character
    
    Serial.print(x);    
    Serial.print("\t");
  }
  
  // Trancemitter
  //
  Wire.beginTransmission(TWI_SLAVE_ADDRESS); // transmit to slave device
  Wire.write(note_n);                        // sends value byte  
  Wire.endTransmission();                    // stop transmitting
  Serial.println(note_n);
  
  note_n++;
  if (note_n == 16)
    note_n = 0;

  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
}


