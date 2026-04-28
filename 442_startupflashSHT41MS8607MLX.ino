uint32_t timer = 0;
const uint32_t interval = 1000000;
#define VBATPIN A7


//IR Temp sensor includes...   https://www.adafruit.com/product/1747
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//Includes for Adafruit Sensirion SHT40, 41, 45 Temperature & Humidity Sensors https://learn.adafruit.com/adafruit-sht40-temperature-humidity-sensor
#include <Adafruit_SHT4x.h>
Adafruit_SHT4x sht4 = Adafruit_SHT4x(); 

//MS8607 PHT sensor includes...   https://learn.adafruit.com/adafruit-te-ms8607-pht-sensor/arduino
#include <Adafruit_MS8607.h>
Adafruit_MS8607 ms8607;

//Datalogger includes...
#include <SPI.h>
#include <SD.h>
const byte chipSelect = 4;

//GPS includes...
#include <Adafruit_GPS.h>

// Name hardware serial port and connect to the GPS on the hardware port
#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
   if (!SD.begin(chipSelect)) {
 Serial.println("card error.");
   // don't do anything more:
   return;
 }
 Serial.println("card initialized.");
/*
  mlx.begin();    // Initialize IR temperature sensor 


  sht4.begin();
  sht4.setPrecision(SHT4X_HIGH_PRECISION); //options: HIGH MED LOW
  sht4.setHeater(SHT4X_NO_HEATER); //options: NO_HEATER HIGH_HEATER_1S HIGH_HEATER_100MS HEATER_1S HEATER_100MS HEATER_1S HEATER_100MS 
*/
  ms8607.begin();
  ms8607.setHumidityResolution(MS8607_HUMIDITY_RESOLUTION_OSR_8b); // options: 8b 10b 11b 12b
  ms8607.setPressureResolution(MS8607_PRESSURE_RESOLUTION_OSR_8192); // options: 256 512 1024 2048 4096 8192

  // Set up GPS...9600baud NMEA, RMC (recommended minimum) and GGA (fix data), 1 Hz update rate...
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(1000);

 String dataString = ""; 
 dataString += "Test Code based on 441 includ startupflash & support for MS8607, SHT41 & MLX\r"; //Marks the beginning of the session in the serial output
 dataString += "millis,micros,date,time,lat,lon,alt,speed,angle,satellites,P(ms8607),T(ms8607),Hum(ms8607),Vbat,readwritetime(us)"; //Headings for CSV columns
   
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      Serial.println(dataString);
    }

  for (int i = 0; i < 4; i++) {
   digitalWrite(LED_BUILTIN, HIGH);
   delay(300);
   digitalWrite(LED_BUILTIN, LOW);
   delay(300);
  }
}


void loop() {

    char gpsChar = GPS.read(); // pull next byte from the serial buffer into the GPS library; call each loop so complete sentences are assembled and newNMEAreceived() can fire


  if (GPS.newNMEAreceived()) // if this character completes a sentence...
  { 
    if (!GPS.parse(GPS.lastNMEA())) // ...check the checksum, set the newNMEAreceived() flag to false, and parse the sentence.
      return; // If it fails to parse a sentence, just wait for another.
  }
 
  if (timer > micros()) timer = micros();  // if micros() wraps around, fully reset the timer too.
  if (micros() - timer >= interval) {  // if the interval has passed, execute code (otherwise continue looping)
    timer += interval; // reset the timer

    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)

    String dataString = "";      // make a string for assembling the data to log:
    dataString += millis();      // add the number of milliseconds since startup to the dataString
    dataString += ",";           
    dataString += micros();      // add the number of microseconds since startup to the dataString
    dataString += ",";           
    dataString += GPS.year;
    dataString += "-";
    dataString += GPS.month;
    dataString += "-";
    dataString += GPS.day;
    dataString += ",";
    dataString += GPS.hour;
    dataString += ":";
    dataString += GPS.minute;
    dataString += ":";
    if (GPS.milliseconds > 500) {
      dataString += (GPS.seconds + 1); //rounds second up when GPS reports early (i.e. :01.984 becomes 02.000)
    }
    else { dataString += GPS.seconds; } //just records second when rounding correction isn't needed 
    dataString += ",";

    if (GPS.fix) {
      dataString += GPS.latitude;
      dataString += ",";
      dataString += GPS.longitude;
      dataString += ",";
      dataString += GPS.altitude;
      dataString += ",";
      dataString += GPS.speed;
      dataString += ",";
      dataString += GPS.angle;
      dataString += ",";
    }
    else { dataString += "0,0,0,0,0,"; } //fills zero values when the GPS doesn't have a fix
    
    dataString += GPS.satellites;
    dataString += ",";         

    dataString += mlx.readAmbientTempC();
    dataString += ",";
    dataString += mlx.readObjectTempC();
    dataString += ",";

    sensors_event_t temp, humidity; // create variables to hold data
    sht4.getEvent(&temp, &humidity); // populate temp and humidity variables with fresh data
    dataString += temp.temperature; // add temperature to dataString
    dataString += ",";          
    dataString += humidity.relative_humidity; // add humidity to dataString
    dataString += ",";         
    
    sensors_event_t pressure;
    ms8607.getEvent(&pressure, &temp, &humidity);
    dataString += pressure.pressure; 
    dataString += ",";
    dataString += temp.temperature; 
    dataString += ",";
    dataString += humidity.relative_humidity; 
    dataString += ",";
    
    dataString += analogRead(VBATPIN)*2*3.3/1024;  //read value of 10k/10k voltage divider on adalogger and convert to volts
    dataString += ",";
    dataString += micros()-timer; //record time passed since beginning of read cycle

  // open the file...
   File dataFile = SD.open("datalog.csv", FILE_WRITE);

   // if the file is available, write to it:
   if (dataFile) {
     dataFile.println(dataString);
     dataFile.close();
     Serial.println(dataString);
  } else {				 	              // if the datafile isn’t available (e.g. loose SD card)
   Serial.print("write failed "); // indicate this on the serial monitor
   Serial.println(dataString);	  // display the data
   SD.begin(chipSelect);		      // attempt to reconnect the SD card, in case it has reseated
   }

       digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
        //
  }
}
