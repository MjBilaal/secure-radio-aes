#include <SPI.h>
#include <mrf24j.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

const int pin_reset = 6;
const int pin_cs = 52;
const int pin_interrupt = 5;

#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

unsigned long last_time = 0;
unsigned long tx_interval = 3000;

// Q2 : configuration réseau
const word PAN_ID    = 0xBABA;
const byte CHANNEL   = 14;
const word SRC_ADDR  = 0xB1A1;
const word DEST_ADDR = 0xB1B2;

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
  SerialUSB.println("Exo2 Q2 - TX Config demarre");

  dht.begin();

  mrf.reset();
  mrf.init();
  mrf.set_pan(PAN_ID);
  mrf.set_channel(CHANNEL);
  mrf.address16_write(SRC_ADDR);

  attachInterrupt(digitalPinToInterrupt(pin_interrupt), interrupt_routine, CHANGE);

  SerialUSB.print("PAN_ID   = 0x");
  SerialUSB.println(PAN_ID, HEX);
  SerialUSB.print("CHANNEL  = ");
  SerialUSB.println(CHANNEL);
  SerialUSB.print("SRC_ADDR = 0x");
  SerialUSB.println(SRC_ADDR, HEX);
  SerialUSB.print("DEST_ADDR= 0x");
  SerialUSB.println(DEST_ADDR, HEX);
  SerialUSB.println("----------------------------");

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

    String trame = "ID=Bilal;H=" + String(h, 2) + ";T=" + String(t, 2);

    char payload[100];
    trame.toCharArray(payload, sizeof(payload));

    SerialUSB.print("Trame envoyee : ");
    SerialUSB.println(trame);

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