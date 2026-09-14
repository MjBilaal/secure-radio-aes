#include <SPI.h>
#include <mrf24j.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <AES.h>
#include <stdlib.h>

const int pin_reset = 6;
const int pin_cs = 52;
const int pin_interrupt = 5;

#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);
AES aes;

unsigned long last_time = 0;
unsigned long tx_interval = 3000;

const word PAN_ID    = 0xBABA;
const byte CHANNEL   = 14;
const word SRC_ADDR  = 0xB1A1;
const word DEST_ADDR = 0xB1B2;

// Clé AES-128
byte key[] = {
  0x2b, 0x7e, 0x15, 0x16,
  0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88,
  0x09, 0xcf, 0x4f, 0x3c
};

unsigned long long int my_iv = 36753562;

uint16_t crc16Modbus(const uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;

  for (uint16_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

String crcToHex(uint16_t crc) {
  char buffer[5];
  sprintf(buffer, "%04X", crc);
  return String(buffer);
}

String bytesToHex(byte *data, int len) {
  String s = "";
  for (int i = 0; i < len; i++) {
    if (data[i] < 0x10) s += "0";
    s += String(data[i], HEX);
    if (i < len - 1) s += " ";
  }
  s.toUpperCase();
  return s;
}

void interrupt_routine() {
  mrf.interrupt_handler();
}

void setup() {
#ifdef _SAM3XA_
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.begin();
#endif

  SerialUSB.begin(115200);
  delay(2000);
  SerialUSB.println("Exo2 Q5/Q6 - TX AES demarre");

  dht.begin();

  mrf.reset();
  mrf.init();
  mrf.set_pan(PAN_ID);
  mrf.set_channel(CHANNEL);
  mrf.address16_write(SRC_ADDR);

  attachInterrupt(digitalPinToInterrupt(pin_interrupt), interrupt_routine, CHANGE);

  last_time = millis();
}

void loop() {
  mrf.check_flags(&handle_rx, &handle_tx);

  unsigned long current_time = millis();
  if (current_time - last_time >= tx_interval) {
    last_time = current_time;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      SerialUSB.println("Erreur lecture DHT");
      return;
    }

    // Trame claire
    String trameSansCRC = "ID=Bilal;H=" + String(h, 2) + ";T=" + String(t, 2);
    uint16_t crc = crc16Modbus((const uint8_t*)trameSansCRC.c_str(), trameSansCRC.length());
    String trameClaire = trameSansCRC + ";CRC=" + crcToHex(crc);

    byte plain[128];
    byte cipher[128];
    byte iv[N_BLOCK];

    memset(plain, 0, sizeof(plain));
    memset(cipher, 0, sizeof(cipher));

    int plainLength = trameClaire.length();
    memcpy(plain, trameClaire.c_str(), plainLength);

    int paddedLength = plainLength;
    if (paddedLength % N_BLOCK != 0) {
      paddedLength += N_BLOCK - (paddedLength % N_BLOCK);
    }

    aes.set_IV(my_iv);
    aes.get_IV(iv);
    aes.do_aes_encrypt(plain, plainLength, cipher, key, 128, iv);

    String trameChiffreeHex = bytesToHex(cipher, paddedLength);

    char payload[300];
    trameChiffreeHex.toCharArray(payload, sizeof(payload));

    SerialUSB.println("Trame claire :");
    SerialUSB.println(trameClaire);

    SerialUSB.println("Trame chiffree (HEX) :");
    SerialUSB.println(trameChiffreeHex);

    mrf.send16(DEST_ADDR, payload);
    SerialUSB.println("----------------------------");
  }
}

void handle_rx() {
}

void handle_tx() {
  if (mrf.get_txinfo()->tx_ok) {
    SerialUSB.println("TX OK");
  } else {
    SerialUSB.print("TX failed apres ");
    SerialUSB.print(mrf.get_txinfo()->retries);
    SerialUSB.println(" retries");
  }
}