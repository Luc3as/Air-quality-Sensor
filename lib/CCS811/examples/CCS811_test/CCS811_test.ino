

#include <CCS811.h>

#define ADDR      0x5A
//#define ADDR      0x5B
#define WAKE_PIN  14

CCS811 sensor;

void setup()
{
  Serial.begin(115200);
  Serial.println("CCS811 test");
  if(!sensor.begin(uint8_t(ADDR), uint8_t(WAKE_PIN)))
    Serial.println("Initialization failed.");
}

void loop()
{ 
  //sensor.compensate(22.56, 30.73);  // replace with t and rh values from sensor
  sensor.getData();
  Serial.print("CO2 concentration : "); Serial.print(sensor.readCO2()); Serial.println(" ppm");
  Serial.print("TVOC concentration : "); Serial.print(sensor.readTVOC()); Serial.println(" ppb");
  Serial.println();
  delay(2000);
}
