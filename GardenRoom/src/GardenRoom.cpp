/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/Nemo/Documents/IoT/l11_sensors-ZaccRan/GardenRoom/src/GardenRoom.ino"
/*
 * Project GardenRoom
 * Description:MidTerm 2
 * Author:Zacc R
 * Date:March 17 2023
 */
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"
#include "HX711.h"
#include <Adafruit_BME280.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
void setup();
void loop();
void Pump();
void Duster();
#line 15 "c:/Users/Nemo/Documents/IoT/l11_sensors-ZaccRan/GardenRoom/src/GardenRoom.ino"
#define OLED_RESET D4
Adafruit_SSD1306 display(OLED_RESET);
#define XPOS 0
#define YPOS 1
#define DELTAY 2                      //OLED
Adafruit_BME280 bme;
HX711 myScale(A3,A4);  //(DT,SCK) position matters
SYSTEM_MODE(AUTOMATIC);   // needs auto to publish
TCPClient TheClient; 

Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname> 
// Adafruit_MQTT_Subscribe subFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/feed1"); 
// Adafruit_MQTT_Publish pubFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/feed2");
Adafruit_MQTT_Subscribe subFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/push");
Adafruit_MQTT_Publish pubFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/feedz");  //Air
Adafruit_MQTT_Publish pubFeed2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/feedy"); //Dust
Adafruit_MQTT_Publish pubFeed3 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/feedx"); //Moist
Adafruit_MQTT_Publish pubFeedF = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");
Adafruit_MQTT_Publish pubFeedH = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humid");
Adafruit_MQTT_Publish pubFeedM = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/press");

unsigned int last,lastTime;
int inHg,humidRH,pressPA,tempF,tempC;
void MQTT_connect();
bool MQTT_ping();
bool status;
int z,y,x;

const int DUST=D6;
const int MOIST=A2;
const int AIR=A0;
const int RELAY=D8;

const int CAL_FACTOR=368; //368 is really close to Grams
const int SAMPLES=10; // number of data points averaged when using get_units or get_value
float weight,rawData,calibration;
int offset,subFeedValue;

unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

void setup() {
  pinMode(A0,INPUT);    //MP-503 Air Quality Sensor v1.3
  pinMode(D6,INPUT);    //Grove Dust Sensor
  pinMode(A2,INPUT);    //Moisture Sensor
  pinMode(D8,OUTPUT);     //Relay to Pump
  Serial.begin(9600);
  Wire.begin();
  WiFi . connect () ; // Connect to internet , but not Particle Cloud
  while ( WiFi . connecting () ) {
  Serial . printf (".");
    }
  status=bme.begin(0x76);
  if(status==false){
      Serial.printf("BME280 at address 0x%02X failed to start\n", 0x76);
    }
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  myScale.set_scale(); // initialize loadcell
  delay (5000); // let the loadcell settle
  myScale.tare(); // set the tare weight (or zero )
  myScale.set_scale (CAL_FACTOR); // adjust when calibrating scale to desired units

  mqtt.subscribe(&subFeed);
}

void loop() {
  MQTT_connect();
  MQTT_ping();
  tempC=bme.readTemperature();//deg C
  tempF=(tempC*1.8)+32;
  pressPA=bme.readPressure();//pascals
  inHg=pressPA*0.0002953;
  humidRH=bme.readHumidity();//%RH
  z=analogRead(A0);
  x=analogRead(A2);
  Duster();
  Serial.printf("Air Quality%i\n",z);
  Serial.printf("Dust%i\n",y);
  Serial.printf("Soil%i\n",x);
  Serial.printf("Temperature is %uF\nPressure is %u\nHumidity is %u\n",tempF,inHg,humidRH);
  Serial.printf("%0.1fGrams\n",weight);
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription=mqtt.readSubscription(10000))) {
    if (subscription==&subFeed) {
      subFeedValue= atoi((char *)subFeed.lastread);
    }
    if (subFeedValue== HIGH) {
      digitalWrite (RELAY, HIGH);
      Serial.printf("Dashboard Water Button Pressed ON \n");
    }
    else {
      digitalWrite(RELAY, LOW);
      Serial.printf("Dashboard Water Button Pressed OFF \n");
    }
  }
  if(x>2000){Pump();
    }
  if((millis()-lastTime > 30000)) {
    if(mqtt.Update()) {
      pubFeed.publish(z);
      pubFeed2.publish(y);
      pubFeed3.publish(x);
      pubFeedF.publish(tempF);
      pubFeedH.publish(humidRH);
      pubFeedM.publish(inHg);
      }
    lastTime = millis();
    }
  y=concentration;
  display.clearDisplay();
  display.display();
  weight=myScale.get_units(SAMPLES); // return weight in units set by set_scale ();
  display.setRotation(1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("Soil %i\n\nDust %i\n\nAirQuality%i\n\nTemp %uF\n\nPress %u\n\nHumid %u\n\n%0.1f g\n",x,y,z,tempF,inHg,humidRH,weight);
  display.display();
  // delay(250);

  // rawData = myScale . get_value ( SAMPLES ) ;// returns raw loadcell reading minus offset
  // offset = myScale . get_offset () ; // returns the offset set by tare ();
  // calibration = myScale . get_scale () ;

}
void MQTT_connect() {
  int8_t ret;
  // Return if already connected.
  if (mqtt.connected()) {
    return; }
  Serial.print("Connecting to MQTT... ");
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
}
  Serial.printf("MQTT Connected!\n");
}
bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;
  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();}
      last = millis();
  } return pingStatus;
}
void Pump() {
  digitalWrite(RELAY,HIGH);
  // Serial.printf("Pump is ON \n");
  delay(500);
  digitalWrite(RELAY,LOW);
  // Serial.printf("Pump is OFF \n");
}
void Duster(){
  duration=pulseIn(D6,LOW);
  lowpulseoccupancy=lowpulseoccupancy+duration;
  if ((millis()-starttime) > sampletime_ms){    //if the sampel time == 30s
        ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
        concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
        lowpulseoccupancy = 0;
        starttime = millis();
    }
}