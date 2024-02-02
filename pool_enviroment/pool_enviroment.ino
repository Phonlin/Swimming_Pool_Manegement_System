#include <WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <SimpleDHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

int gasSensor = 32; // 指定要量測的analog腳位為2
int gasval = 0;


const char ssid[]     = "Pixel";// change to your WIFI SSID
const char password[] = "0928661531";// change to your WIFI Password

IPAddress server_addr(192,168,68,176); // change to you server ip, note its form split by "," not "."
int MYSQLPort = 3333; //mysql port default is 3306
char user[] = "swimuser";// Your MySQL user login username(default is root),and note to change MYSQL user root can access from local to internet(%)
char pass[] = "bcps8562620";// Your MYSQL password

WiFiClient client;
MySQL_Connection conn((Client *)&client);


int pinDHT11 = 25;
SimpleDHT11 dht11(pinDHT11);
const int oneWireBus = 15;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

#define TdsSensorPin 35
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  1            // sum of sample point

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation

// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}



void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin,INPUT);
  delay(200);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  

    WiFi.begin(ssid, password);
  //try to connect to WIFI
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  sensors.begin();

  //try to connect to mysql server
  if (conn.connect(server_addr, 3333, user, pass)) {
    delay(1000);
  }
  else {
    Serial.println("Connection failed.");
  }
  

//  pinMode(TdsSensorPin,INPUT);
  
  delay(2000);
}


void loop() {
//***********TGS2602偵測空氣中的揮發物汙染**************
  gasval = analogRead(gasSensor);
  Serial.println( gasval );
  delay(500);
//**************************************************

  //讀取DHT11
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;

  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err); delay(1000);
    return;
  }
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, ");
  Serial.print((int)humidity); Serial.println(" H");


  //將溫濕度加入SQL字串
  String INSERT_SQL = "INSERT INTO swim.datalog (temp,humd,temp_water,quality_water,quality_air) VALUES ('" + String((int)temperature) + "','" + String((int)humidity) + "','" + String((float)temperatureC) + "','" + String((int)tdsValue) + "','" + String((int)gasval) + "')";
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(INSERT_SQL.c_str());//execute SQL
  delete cur_mem;
  //conn.close();// close the connection
  Serial.println("Data Saved.");
  delay(5000);

static unsigned long analogSampleTimepoint = millis();
  
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];


      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;

//     
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
    }
  }


}
