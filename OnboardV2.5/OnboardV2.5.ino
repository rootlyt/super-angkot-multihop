#include <SPI.h>
#include <MFRC522.h>
#include <esp_now.h>
#include <WiFi.h>

#define SS_PIN 21
#define RST_PIN 22
// membuat instance MFRC522
// untuk mengakses pin pada MODUL RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

// deklarasi variabel pin untuk modul motor driver 
// Motor A
int motor1Pin1 = 27; 
int motor1Pin2 = 26; 
int enable1Pin = 14; 
// Motor B
int motor2Pin1 = 33; 
int motor2Pin2 = 32; 
int enable2Pin = 25;

// pengaturan PWM untuk modul motor
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

// deklarasi variabel unsigned byte untuk alamat MAC Address untuk setiap halte
uint8_t broadcastAddress1[] = {0x50,0x02,0x91,0xe1,0xa6,0x00}; //halte 1
uint8_t broadcastAddress2[] = {0x50,0x02,0x91,0xe1,0x4e,0xac}; //halte 2
uint8_t broadcastAddress3[] = {0xcc,0x50,0xe3,0x5d,0x9e,0x89}; //halte 3
uint8_t broadcastAddress4[] = {0x50,0x02,0x91,0xe1,0xab,0xf7}; //halte 4

// struktur untuk pengiriman data [ESPNOW]
typedef struct infoAngkot_struct {
   char angkotLoc[32];
   char angkotETA[32];
} infoAngkot_struct;

// membuat infoAngkot_struct bernama infoAngkot untuk variabel pesan
// yang akan dikirim
infoAngkot_struct infoAngkot;

// callback untuk espnow saat data terkirim
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet to: ");
  // menyalin mac pengirim (onboard unit kedalam string)
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// method setup untuk dijalankan sekali 
void setup() {
  // inisialisasi Serial Monitor
  Serial.begin(115200);

  // melakukan set pinMode dengan variabel berisi nomor pin
  // sebagai output untuk mengontrol driver motor
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable2Pin, OUTPUT);
  // fungsi ledcSetup untuk mengatur PWM 
  ledcSetup(pwmChannel, freq, resolution);
  // menyambung pin untuk dikontrol sebagai PWM
  ledcAttachPin(enable1Pin, pwmChannel);
  ledcAttachPin(enable2Pin, pwmChannel);

  // inisialisasi SPI untuk digunakan modul RFID Reader
  SPI.begin();          // inisialisasi bus SPI
  mfrc522.PCD_Init();   // inisialisasi MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println(); 

  // mengatur perangkat menjadi mode Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // inisialisasi ESPNOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // setelah dilakukan inisialisasi, dilakukan register fungsi callback OnDataSent
  // untuk mengetahui status dari paket yang ditransmisi(dikirim) pada ESPNOW
  esp_now_register_send_cb(OnDataSent);
   
  // mengatur channel 802.11 pada ESPNOW untuk digunakan pengiriman
  // dan register peer (mac perangkat tujuan pengiriman)
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  // register halte 1 
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // register halte 2  
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // register halte 3
  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
   // register halte 4
  memcpy(peerInfo.peer_addr, broadcastAddress4, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

//memberi delay 3 detik sebagai persiapan 
//memantau alat 
  delay(3000);
}

//method untuk menggerakkan motor
void maju(){
// menggerakkan motor agar maju
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  ledcWrite(pwmChannel, dutyCycle); 
  Serial.println("Forward with duty cycle: 200 /n");
  dutyCycle = 200;
  }

//method untuk memberhentikan motor
void setop(){
  Serial.println("Motor stopped");
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
  }

//method untuk melakukan pembacaan RFID tag  
void rfid(){
  // mendeteksi apakah ada kartu
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // membaca kartu
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  // menampilkan UID pada serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  //melakukan serial print untuk pembacaan tag dengan compare UID
  Serial.println();
  Serial.print("Posisi : ");
  content.toUpperCase();
  // jika terdeteksi uid halte 1
  if (content.substring(1) == "8C 05 C9 75")
  {
    setop();
    //mengirim ke halte 1 menggunakan ESPNOW
          sprintf (infoAngkot.angkotLoc,"Angkot Sampai di H1");
          sprintf (infoAngkot.angkotETA,"ETA ke H2 2xH1 Detik");
       
        esp_err_t result = esp_now_send(broadcastAddress1, (uint8_t *) &infoAngkot, sizeof(infoAngkot_struct));
         
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
        delay(1000);
    Serial.println("Angkot Sampai di Halte 1 ! Data akan dikirim ke Halte");
    Serial.println();
    delay(5000);
  }
  // jika terdeteksi uid halte 2
  else if (content.substring(1) == "C9 E0 7A B2")
  {
    setop();
          //mengirim ke halte 2 menggunakan ESPNOW
          sprintf (infoAngkot.angkotLoc,"Angkot Sampai di H2");
          sprintf (infoAngkot.angkotETA,"ETA ke H3 3xH1 Detik");
                 
        esp_err_t result = esp_now_send(broadcastAddress2, (uint8_t *) &infoAngkot, sizeof(infoAngkot_struct));
                
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
        Serial.println("Error sending the data");
        }                
        delay(1000);                    
    Serial.println("Angkot Sampai di Halte 2 ! Data akan dikirim ke Halte");
    Serial.println();
    delay(5000);
  }
  // jika terdeteksi uid halte 3
  else if (content.substring(1) == "40 B6 AF 30")
  {
    setop();
          //mengirim ke halte 3 menggunakan ESPNOW
          sprintf (infoAngkot.angkotLoc,"Angkot Sampai di H3");
          sprintf (infoAngkot.angkotETA,"ETA ke H4 4xH1 Detik");
                 
        esp_err_t result = esp_now_send(broadcastAddress3, (uint8_t *) &infoAngkot, sizeof(infoAngkot_struct));
        
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
        Serial.println("Error sending the data");
        }                  
        delay(1000);
    Serial.println("Angkot Sampai di Halte 3 ! Data akan dikirim ke Halte");
    Serial.println();
    delay(5000);
  }
  // jika terdeteksi uid halte 4
  else if (content.substring(1) == "CA 9F C9 75")
  {
    setop();
          //mengirim ke halte 3 menggunakan ESPNOW
          sprintf (infoAngkot.angkotLoc,"Angkot Sampai di H4");
          sprintf (infoAngkot.angkotETA,"Pemberhentian Terakhir H4");
                 
        esp_err_t result = esp_now_send(broadcastAddress4, (uint8_t *) &infoAngkot, sizeof(infoAngkot_struct));
         
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
        Serial.println("Error sending the data");
        }                  
        delay(1000);
    Serial.println("Angkot Sampai di Halte 4 ! Data akan dikirim ke Halte");
    Serial.println();
    delay(5000);
  }
 // jika uid tidak terdaftar sebagai halte
 else   {
    setop();
    Serial.println("Halte Tak Terdaftar ");
    delay(5000);
  }
}

// method main looping
void loop() {
// memanggil method maju menggerakkan motor, 
// rfid untuk pembacaan dan maju lagi untuk maju.
  maju();
  rfid();
  maju();
}