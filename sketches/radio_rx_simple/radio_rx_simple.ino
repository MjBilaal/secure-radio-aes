#include <SPI.h>
#include <mrf24j.h>

const int pin_reset = 6;
const int pin_cs = 52;
const int pin_interrupt = 5;

Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

int nbrpaket = 0;

void setup() {
#ifdef _SAM3XA_
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.begin();
#endif

  SerialUSB.begin(115200);
  delay(2000);
  SerialUSB.println("RX demarre");

  mrf.reset();
  mrf.init();

  mrf.set_pan(0xcafe);
  mrf.set_channel(11);     // canal test
  mrf.address16_write(0x6002); // adresse du RX

  attachInterrupt(pin_interrupt, interrupt_routine, CHANGE);
}

void interrupt_routine() {
  mrf.interrupt_handler();
}

void loop() {
  mrf.check_flags(&handle_rx, &handle_tx);
}

void handle_rx() {
  nbrpaket++;

  SerialUSB.print("Packet recu #");
  SerialUSB.println(nbrpaket);

  SerialUSB.print("Donnees : ");
  for (int i = 0; i < mrf.rx_datalength(); i++) {
    SerialUSB.write(mrf.get_rxinfo()->rx_data[i]);
  }
  SerialUSB.println();
  SerialUSB.println("--------------------");
}

void handle_tx() {
}