//remote IO Unit 
#include <Ra01S.h>
#include <SPI.h>

#define RF_FREQUENCY                 915000000 // Hz  center frequency
#define TX_OUTPUT_POWER              22        // dBm tx output power
#define LORA_BANDWIDTH               4         // 4: 125Khz                                                               
#define LORA_SPREADING_FACTOR        7         // SF7
#define LORA_CODINGRATE              1         // 1: 4/5
#define LORA_PREAMBLE_LENGTH         8         // Same for Tx and Rx
#define LORA_PAYLOADLENGTH           0         // 0: Variable length packet

// IO pins for HackerBox LoRa I/O Board
#define NSS  5
#define RST  27
#define BUSY 32
#define SCK  18
#define MOSI 23
#define MISO 19
#define DIO1 33 // not used
#define LED  2   // On-board LED
#define PIR_SENSOR_PIN 4  // Pin connected to the PIR sensor

SX126x lora(NSS, RST, BUSY);

unsigned long pkt_num = 0;

void setup() 
{
  SPI.begin();
  pinMode(LED, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  Serial.begin(115200);

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
  int pirValue = digitalRead(PIR_SENSOR_PIN);
  if (pirValue == HIGH) {  // Motion detected
    uint8_t txData[255];
    sprintf((char *)txData, "Motion Detected, Packet:%lu", pkt_num);
    uint8_t len = strlen((char *)txData);

    if (lora.Send(txData, len, SX126x_TXMODE_SYNC)) {
      digitalWrite(LED, HIGH);
      delay(200); 
      digitalWrite(LED, LOW); 
      Serial.print("Send success ");
      Serial.println(pkt_num);
      pkt_num++;
    } else {
      Serial.println("Send fail");
    }

    delay(2000);  // Avoid multiple detections
  }
}
