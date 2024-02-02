#include <SimpleDHT.h> 
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "time.h"
#define ctsPin 34
int buzzPin = 14;

const char* ssid     = "Pixel";
const char* password = "0928661531";

String Linetoken = "";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;//格林威治時間,一格3600,GMT+8就是8*3600=28800
const int   daylightOffset_sec = 3600;

int TOUTCH_PIN=27; 
int light_sensor = 32;
int led =33;
int led_red =25;
int led_green =26;
int val_1;
int pinDHT11 = 3; //ESP32 GPIO15
SimpleDHT11 dht11;

WiFiClientSecure client;
char host[] = "notify-api.line.me";//LINE Notify API網址

//OLED設定
#define SCREEN_WIDTH 128 //OLED寬度
#define SCREEN_HEIGHT 64 //OLED高度
#define OLED_RESET    -1 //Reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)
 
void setup() {
Serial.begin(115200);
  pinMode(light_sensor, INPUT);
  pinMode(led, OUTPUT);
  pinMode(TOUTCH_PIN,INPUT);
  pinMode(led_green,OUTPUT);
  pinMode(led_red,OUTPUT);
  pinMode(ctsPin, INPUT);
  pinMode(buzzPin, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.display();
  
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  client.setInsecure();
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();


}

void loop() {
    printLocalTime();
    byte temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    val_1=digitalRead(TOUTCH_PIN);
    // start working...
    if ((err = dht11.read(pinDHT11, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
       Serial.print("Read DHT11 failed, err="); Serial.println(err);delay(1000);
       return;
    }
    
    Serial.print("Humidity = ");   
    Serial.print((int)humidity);   
    Serial.print("% , ");   
    Serial.print("Temperature = ");   
    Serial.print((int)temperature);   
    Serial.println("C ");   
    delay(1000);  //每1秒顯示一次
    display.setCursor(0, 22);//設定游標位置
    display.print("temp:");display.println((int)temperature);
    display.setCursor(0, 33);//設定游標位置
    display.print("humi:");display.println((int)humidity);
//小夜燈*****************
    int val=analogRead(light_sensor);
    Serial.print("light: ");
    Serial.println(val);
    if (val<1200){
      digitalWrite(led, LOW);
      }
    else digitalWrite(led, HIGH);
//****************************
//使用狀況*********************
    if (val_1==0){
    digitalWrite(led_red, LOW);
    digitalWrite(led_green, HIGH);
    display.setTextSize(1);//設定文字大小
    display.setTextColor(WHITE);//文字顏色
    display.setCursor(0, 44);//設定游標位置
    display.print("room1:using");
    }
    else {
    digitalWrite(led_green, LOW);
    digitalWrite(led_red, HIGH); 
    display.setTextSize(1);//設定文字大小
    display.setTextColor(WHITE);//文字顏色
    display.setCursor(0, 44);//設定游標位置
    display.print("room1:no use");
      }
   display.display();//顯示螢幕   

//觸摸開關****************************
    int ctsValue = digitalRead(ctsPin);
    
  if (ctsValue == HIGH){
    digitalWrite(buzzPin, HIGH); //有源蜂鳴器響起
    String message = "發生危險，目前狀態：";
    Serial.println(message);
//********line post************
    if (client.connect(host, 443)) {     
      int LEN = message.length();
      String url = "/api/notify";
      client.println("POST " + url + " HTTP/1.1");
      client.print("Host: "); client.println(host);
      //權杖
      client.print("Authorization: Bearer "); client.println(Linetoken);
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: "); client.println( String((LEN + 8)) );
      client.println();      
      client.print("message="); client.println(message);
      client.println();
      //等候回應
      delay(2000);
      String response = client.readString();
      //顯示傳遞結果
      Serial.println(response);
      client.stop(); //斷線，否則只能傳5次
    }
    else {
      //傳送失敗
      Serial.println("connected fail");
    }
  }
  //每5秒讀取一次溫濕度
//  delay(5000); 
//  }
  else{
    digitalWrite(buzzPin, LOW);  //有源蜂鳴器關閉
    delay(2000);
  } 
  Serial.println(buzzPin);

}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%F %R");
     display.clearDisplay();//清除螢幕
    display.setTextSize(1);//設定文字大小
    display.setTextColor(WHITE);//文字顏色
      display.setCursor(0, 0);//設定游標位置
    display.print("date:");display.println(&timeinfo, "%F");
    display.setCursor(0, 11);//設定游標位置
    display.print("time:");display.println(&timeinfo, "%R");
}
