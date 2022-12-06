#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//deklarasi variable keperluan ultrasonic
#define SOUND_VELOCITY 0.034

long duration;
float distanceCm;
float distanceMtr;

// deklarasi OLED DISPLAY pin I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//deklarasi keperluan pin sensor
const int trigPin = 12;
const int echoPin = 14;
int pirInput = 13; 

//deklarasi keperluan NTP (Mengambil waktu saat ini dari server NTP)
const long utcOffsetInSeconds = 25200;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//DEKLARASI Wifi dan define MQTT
const char* ssid = "LIA"; // Enter your WiFi name
const char* password =  "mikalucu"; // Enter WiFi password
const char* mqttServer = "192.168.1.192";//IP Raspberry pi zero w
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
WiFiClient espClient;
PubSubClient client(espClient);

//mendefinisikan NTP Client untuk mendapatkan waktu dari server NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


// Deklarasi MAC Address untuk tujuan pengiriman halte, uncomment salah satu untuk menggunakan
uint8_t broadcastAddress[] = {0x50,0x02,0x91,0xe1,0xa6,0x00}; //send to h1 (digunakan di Halte4) 
//uint8_t broadcastAddress[] = {0x50,0x02,0x91,0xe1,0x4e,0xac}; //send to h2 (digunakan di Halte1) 
//uint8_t broadcastAddress[] = {0xcc,0x50,0xe3,0x5d,0x9e,0x89}; //send to h3 (digunakan di Halte2) 
//uint8_t broadcastAddress[] = {0x50,0x02,0x91,0xe1,0xab,0xf7}; //send to h4 (digunakan di Halte3) 

// deklarasi variabel untuk menyimpan pesan 
// dari komunikasi ESPNOW yang akan dikirim ke Halte selanjutnya.
char angkotLoc[32]; 
char angkotETA[32];

// deklarasi variabel untuk menyimpan pesan 
// yang datang dari halte sebelumnya ataupun onboard unit
char incomingAngkotLoc[32];
char incomingAngkotETA[32];

// variabel yang disimpan ketika pengiriman data sukses [ESPNOW]
String success;

// struktur untuk pengiriman data [ESPNOW]
typedef struct struct_message {
    char angkotLoc[32];
    char angkotETA[32];
} struct_message;

// membuat struct_message bernama infoAngkot untuk menahan atau 
// (menyimpan pembacaan pesan yang diterima halte saat ini)
struct_message infoAngkot;

// membuat struct_message bernama infoAngkot untuk menahan 
// atau(menyimpan pembacaan pesan yang diterima dari halte sebelumnya)
struct_message incomingReadings;

// callback untuk espnow saat data terkirim
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

// callback untuk espnow saat data diterima
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  sprintf (incomingAngkotLoc, incomingReadings.angkotLoc);
  sprintf (incomingAngkotETA, incomingReadings.angkotETA);
}

//method untuk menampilkan pesan yang diterima dari halte sebelumnya ke serial monitor untuk debugging
void printIncomingReadings(){
  // Display Readings in Serial Monitor
  Serial.println("    _______ START OF DATA ______   ");
  Serial.println("Data Diterima: ");
  Serial.print("Posisi: - ");
  Serial.println(incomingAngkotLoc);
  Serial.print("ETA: - ");
  Serial.print(incomingAngkotETA);
  Serial.println(" ");
  Serial.println("    ________ END OF DATA _______   ");
}
 
//method setup untuk dijalankan sekali 
void setup() {
  // inisialisasi Serial Monitor
  Serial.begin(115200);
 
  // mengatur perangkat menjadi mode Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // inisialisasi ESPNOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // mengatur role ESP-NOW menjadi combo (bisa receive dan send sekaligus)
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  
  // setelah dilakukan inisialisasi, dilakukan register fungsi callback OnDataSent
  // untuk mengetahui status dari paket yang ditransmisi(dikirim) pada ESPNOW
  esp_now_register_send_cb(OnDataSent);
  
  // register peer (mac perangkat tujuan pengiriman)
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // dilakukan register fungsi callback OnDataRecv yang akan dipanggil ketika data diterima
  esp_now_register_recv_cb(OnDataRecv);

  //INISIALISASI WIFI
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  //memulai routine NTP
  timeClient.begin();
  
  //inisialisasi pin untuk sensor ultrasonik
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  //inisialisasi pin untuk sensor Passive infrared
  pinMode(pirInput, INPUT);

//inisialisasi OLED dan test
// Address I2C 0x3D for 128x64 OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  //dilakukan delay untuk memberi jarak setelah inisialisasi untuk tes layar
  delay(2000); 
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(30, 10);
  // Display text statis
  display.println("OLED OK!");
  display.display();
  display.clearDisplay(); 

}


// Method pembacaan sensor ultrasonik
void ultrasc() {
  // fungsi set reset ini berguna untuk mengirim sinyal ultrasonik pada sensor
  // reset pada trigger pin dengan set jadi low (off)
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // set trigger pin ke HIGH untuk 10ms dan set LOW lagi
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // membaca echoPin, dan membaca jarak kembali gelombang suara ke satuan ms
  duration = pulseIn(echoPin, HIGH);
  
  // menghitung jarak dari pembacaan
  distanceCm = duration * SOUND_VELOCITY/2;
  
  // konversi dari meter ke centimeter
  distanceMtr = distanceCm/100;
  
  // menampilkan hasil pembacaan pada Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distance (meter): ");
  Serial.println(distanceMtr);
 }

// Method pembacaan sensor Passive Infrared (PIR) motion
void pir () {
    long state = digitalRead(pirInput);
    if(state == HIGH) {
      Serial.println("_____________________________");
      Serial.println("Terkonfirmasi Pergerakan ");
    }
    else {
            Serial.println("_____________________________");
      Serial.println("Tiada Pergerakan ");
      }  
  }

// Method main looping 
void loop() {

  // melakukan inisialisasi MQTT dan menyambung ke broker
  client.setServer(mqttServer, mqttPort);
//  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {

      Serial.println("connected");

    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

// mengambil waktu dari server NTP
  timeClient.update();

//memanggil method untuk pembacaan sensor
  ultrasc();
  pir();

//seleksi kondisi untuk konfirmasi pergerakan pada PIR dan pembacaan jarak
 long state = digitalRead(pirInput);
     if(distanceMtr<0.60 & state == HIGH) {

      //pesan ditambah identitas halte pada setiap node, ini bisa diganti manual saat akan upload ke perangkat
      //publish ke MQTT ketika terkonfirmasi terdapat penumpang
      client.publish("halte/penumpang", "Terdeteksi Penumpang Halte 4");
      //menampilkan pada serial monitor 
      Serial.println("                          _____________________________");
      Serial.println("                         | Mungkin Manusia >> MQTT SEND ");
            Serial.print(daysOfTheWeek[timeClient.getDay()]);
            Serial.print(", ");
            Serial.print(timeClient.getHours());
            Serial.print(":");
            Serial.print(timeClient.getMinutes());
            Serial.print(":");
            Serial.println(timeClient.getSeconds());
      Serial.println("                         |_____________________________");
      Serial.println("                         |______________________________");
      //menampilkan output pada OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 10);
        display.println("Halo Penumpang!");
        display.display();
        display.setCursor(0, 40);
        display.println("Angkot Akan Datang");
        display.display();
    }
    else {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0, 10);
        // Display static text
        display.println("ETA : ");
        display.display();
        display.setTextSize(1);
        display.setCursor(0, 40);
        display.println(incomingAngkotETA);
        display.display();
      }  

// deklarasi untuk manipulasi variabel pesan 
// yang datang agar sama dengan pesan yang akan dikirim
  sprintf (infoAngkot.angkotLoc, incomingAngkotLoc);
  sprintf (infoAngkot.angkotETA, incomingAngkotETA);
      
// mengirim pesan kosong kepada topik yang tidak digunakan
// untuk menjaga koneksi tidak timeout
  client.publish("antitimeout", "");

// mengirim pesan ke halte berikutnya 
  Serial.println("Mengirim (relay) Pesan ke Node Selanjutnya..");
  esp_now_send(broadcastAddress, (uint8_t *) &infoAngkot, sizeof(infoAngkot));

// memanggil methot pembacaan pesan yang datang
  printIncomingReadings();    
  delay (5000);
}
