#include <SPI.h>
#include <mrf24j.h>

const int pin_reset = 6;
const int pin_cs = 52;
const int pin_interrupt = 5;

Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

unsigned long last_time = 0;
unsigned long tx_interval = 3000;

void setup() {
#ifdef _SAM3XA_
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.begin();
#endif

  SerialUSB.begin(115200);
  delay(2000);
  SerialUSB.println("TX demarre");

  mrf.reset();
  mrf.init();

  mrf.set_pan(0xcafe);
  mrf.set_channel(11);      // canal test
  mrf.address16_write(0x6001); // adresse du TX

  attachInterrupt(pin_interrupt, interrupt_routine, CHANGE);

  last_time = millis();
}

void interrupt_routine() {
  mrf.interrupt_handler();
}

void loop() {
  mrf.check_flags(&handle_rx, &handle_tx);

  unsigned long current_time = millis();
  if (current_time - last_time >= tx_interval) {
    last_time = current_time;

    SerialUSB.println("Envoi du message...");
    mrf.send16(0x6002, "abcd");
  }
}

void handle_rx() {
}

void handle_tx() {
  if (mrf.get_txinfo()->tx_ok) {
    SerialUSB.println("TX OK, ACK recu");
  } else {
    SerialUSB.print("TX failed apres ");
    SerialUSB.print(mrf.get_txinfo()->retries);
    SerialUSB.println(" retries");
  }
}