// Wire Master Reader
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Reads data from an I2C/TWI slave device
// Refer to the "Wire Slave Sender" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>
#include <stdio.h>

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
  Wire.requestFrom(0x7f, 4);    // request 4 bytes from slave device 0x7f

  while (Wire.available())   // slave may send less than requested
  {
    int x = Wire.read(); // receive a byte as character
    
    Serial.print(x, BIN);    
    Serial.print("\t");         // print the character
  }
  Serial.println("");
  
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
}


