#include <Ra01S.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define RF_FREQUENCY                 915000000 // Hz  center frequency
#define TX_OUTPUT_POWER              22        // dBm tx output power
#define LORA_BANDWIDTH               4         // 4: 125Khz                                                               
#define LORA_SPREADING_FACTOR        7         // SF7
#define LORA_CODINGRATE              1         // 1: 4/5
#define LORA_PREAMBLE_LENGTH         8         // Same for Tx and Rx
#define LORA_PAYLOADLENGTH           0         // 0: Variable length packet

// IO pins for HackerBox LoRa C3 OLED Board
#define NSS  1
#define RST  0
#define BUSY 10
#define SCK  4 
#define MOSI 3 
#define MISO 7 
#define DIO1 20 // not used
#define RGBLEDPIN 21  // Corrected pin to 21
#define SDA  5
#define SCL  6

SX126x lora(NSS, RST, BUSY);

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, RGBLEDPIN, NEO_GRB + NEO_KHZ800);

int first_rx = 1;
unsigned long prior_pkt = 0;
unsigned long err_count = 0;
char txtout[15];

void setup() 
{
  SPI.begin(SCK, MISO, MOSI, NSS);
  pinMode(RGBLEDPIN, OUTPUT);

  Wire.begin(SDA, SCL);
  u8g2.begin();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(115200);

  Serial.println("Disable TCXO");
  int16_t ret = lora.begin(RF_FREQUENCY, TX_OUTPUT_POWER);
  if (ret != ERR_NONE) while(1) {delay(1);}

  lora.LoRaConfig(LORA_SPREADING_FACTOR, 
                  LORA_BANDWIDTH, 
                  LORA_CODINGRATE, 
                  LORA_PREAMBLE_LENGTH, 
                  LORA_PAYLOADLENGTH, 
                  true,               // crcOn  
                  false);             // invertIrq
}

void loop() 
{
  uint8_t rxData[255];
  uint8_t rxLen = lora.Receive(rxData, 255);
  unsigned long cur_pkt = 0;
  if ( rxLen > 0 )
  { 
    digitalWrite(RGBLEDPIN, LOW);
    delay(200); 
    digitalWrite(RGBLEDPIN, HIGH); 
    Serial.print("Receive rxLen:");
    Serial.println(rxLen);
    for(int i=0;i< rxLen;i++) {
      Serial.print(rxData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    String receivedData = "";
    for(int i=0; i< rxLen; i++) {
      receivedData += (char)rxData[i];
    }
    Serial.println("Received Data: " + receivedData);

    int motionIndex = receivedData.indexOf("Motion Detected");
    int packetIndex = receivedData.indexOf("Packet:");

    if (motionIndex >= 0 && packetIndex >= 0) {
      cur_pkt = receivedData.substring(packetIndex + 7).toInt();

      if (first_rx) {
        first_rx = 0;
      } else {
        if (cur_pkt != (prior_pkt+1)) {
          err_count++;
        }
      }
      prior_pkt = cur_pkt;

      int8_t rssi, snr;
      lora.GetPacketStatus(&rssi, &snr);

      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_mf);
      sprintf(txtout,"Motion Detected");
      u8g2.drawStr(0, 10, txtout);    
      sprintf(txtout,"Rx Num %lu", cur_pkt);
      u8g2.drawStr(0, 20, txtout);
      sprintf(txtout,"Errors %lu", err_count);
      u8g2.drawStr(0, 30, txtout);
      sprintf(txtout,"RSSI %i dBm", rssi);
      u8g2.drawStr(0, 40, txtout);
      sprintf(txtout,"SNR %i dB", snr);
      u8g2.drawStr(0, 50, txtout);
      u8g2.sendBuffer();

      // Blink the RGB LED ring
      for (int i = 0; i < 3; i++) {
        strip.fill(strip.Color(255, 0, 0), 0, strip.numPixels()); // Red
        strip.show();
        delay(500);
        strip.fill(strip.Color(0, 0, 0), 0, strip.numPixels()); // Off
        strip.show();
        delay(500);
      }
    }
  }

  delay(1);
}
