/*
 * Project weatherStation
 * Description: Weather station with logging
 * Author: Erlend Birketvedt
 * Date: Jun-04-2017
 */

 // This #include statement was automatically added by the Particle IDE.
 #include <HttpClient.h>

 // This #include statement was automatically added by the Particle IDE.
 #include <Adafruit_DHT.h>

 // HTTP-client config (for exporting data to our database)
 #define INFLUXDB_HOST "http://139.59.131.167:8086/"
 #define INFLUXDB_PORT 8086
 #define INFLUXDB_DB "weatherstation"

 HttpClient http;

 // Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    { "Host", INFLUXDB_HOST },
    { "Connection", "close" },
    { "Accept" , "application/json" },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

bool sendInflux(String payload) {
    http_request_t     request;
    http_response_t    response;

    request.hostname = INFLUXDB_HOST;
    request.port     = INFLUXDB_PORT;
    request.path     = "/write?db=" + String(INFLUXDB_DB);
    request.body     = payload;

    http.post(request, response, headers);

    if (response.status == 204) {
        return true;
    } else {
        return false;
    }
}

 // DHT config
 #define DHTPIN 2
 #define DHTTYPE DHT11

 // Counter config
 #define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)

 // Variables from DHT sensor
 int temp;
 int humidity;

 /* Light sensor
  * Define the number of samples to keep track of.  The higher the number,
  * the more the readings will be smoothed, but the slower the output will
  * respond to the input.  Using a constant rather than a normal variable lets
  * use this value to determine the size of the readings array.
  */
 const int numReadings = 10;
 int readings[numReadings];      // the readings from the analog input
 int readIndex = 0;              // the index of the current reading
 int total = 0;                  // the running total
 int average = 0;                // the average

 int photoresistor = A0; // This is where your photoresistor is plugged in. The other side goes to the "power" pin (below).

 int power = A5; // This is the other end of your photoresistor. The other side is plugged into the "photoresistor" pin (above).
 // The reason we have plugged one side into an analog pin instead of to "power" is because we want a very steady voltage to be sent to the photoresistor.
 // That way, when we read the value from the other side of the photoresistor, we can accurately calculate a voltage drop.

 // Sensor initialization
 DHT dht(DHTPIN, DHTTYPE);

 // Counter used for delaying functions without blocking
 unsigned long lastSync = millis();

 void setup() {
     pinMode(photoresistor,INPUT);
     pinMode(power,OUTPUT);
     digitalWrite(power,HIGH);
     // Initialize all the readings to 0 for the light sensor:
     for (int thisReading = 0; thisReading < numReadings; thisReading++) {
         readings[thisReading] = 0;
     }
     // Initialize the DHT11 sensor
     dht.begin();
 }

 /* Request time synchronization from the Particle Cloud once every 24 hours.
  * This is not necessary for a unit that is frequently rebooted.
  * For a weather station that should be left unattended for months, if not years,
  * the built-in RTC will become inaccurate after long periods so to be safe
  * we will update the RTC periodically.
  */
 void syncRTC() {
     if (millis() - lastSync > ONE_DAY_MILLIS) {
     Particle.syncTime();
     lastSync = millis();
     }

 }

 void checkDHT11() {
     temp = dht.getTempCelcius();
     humidity = dht.getHumidity();

 }

 void lightSensor() {
     // subtract the last reading:
   total = total - readings[readIndex];
   // read from the sensor:
   readings[readIndex] = analogRead(photoresistor);
   // add the reading to the total:
   total = total + readings[readIndex];
   // advance to the next position in the array:
   readIndex = readIndex + 1;

   // if we're at the end of the array...
   if (readIndex >= numReadings) {
     // ...wrap around to the beginning:
     readIndex = 0;
   }

   // calculate the average:
   average = total / numReadings;
   average = map(average, 0, 4095, 0, 100);
   delay(1);        // delay in between reads for stability
 }

 void publish() {
     // Publish once every 60 sec
     if (millis() - lastSync > 10000) {
         Particle.publish("Temperature", String(temp), PRIVATE);
         Particle.publish("Humidity", String(humidity), PRIVATE);
         Particle.publish("Time", String(Time.format(TIME_FORMAT_ISO8601_FULL)), PRIVATE);
         Particle.publish("Light output", String(average), PRIVATE);
         lastSync = millis();
         String influxData = "Temperature = " + (temp);
         sendInflux(influxData);
     }

 }

 void loop() {
     syncRTC();
     checkDHT11();
     lightSensor();
     publish();
     // Add a delay to avoid overloading the Photon unnecessarily
     // delay(1000);

 }
