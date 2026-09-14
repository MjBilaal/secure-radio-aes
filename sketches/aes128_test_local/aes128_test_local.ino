#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <AES.h>

#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
AES aes;

// Clé AES-128 = 16 octets
byte key[] = {
  0x2b, 0x7e, 0x15, 0x16,
  0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88,
  0x09, 0xcf, 0x4f, 0x3c
};

// IV simple pour la démo
unsigned long long int my_iv = 36753562;

void printHex(byte *data, int len) {
  for (int i = 0; i < len; i++) {
    if (data[i] < 0x10) {
      SerialUSB.print("0");
    }
    SerialUSB.print(data[i], HEX);
    SerialUSB.print(" ");
  }
  SerialUSB.println();
}

void setup() {
  SerialUSB.begin(9600);
  while (!SerialUSB) {
    ;
  }

  dht.begin();

  SerialUSB.println("Test AES-128 sur la temperature");
}

void loop() {
  delay(2000);

  float tempCelcius = dht.readTemperature();

  if (isnan(tempCelcius)) {
    SerialUSB.println("Erreur de lecture du capteur DHT !");
    return;
  }

  // Convertir la température en texte
  String texteTemp = String(tempCelcius, 2);

  byte plain[32];
  byte cipher[32];
  byte check[32];
  byte iv[N_BLOCK];

  memset(plain, 0, sizeof(plain));
  memset(cipher, 0, sizeof(cipher));
  memset(check, 0, sizeof(check));

  int plainLength = texteTemp.length();
  memcpy(plain, texteTemp.c_str(), plainLength);

  int paddedLength = plainLength + N_BLOCK - (plainLength % N_BLOCK);

  // Chiffrement AES-128
  aes.set_IV(my_iv);
  aes.get_IV(iv);
  aes.do_aes_encrypt(plain, plainLength, cipher, key, 128, iv);

  // Déchiffrement AES-128
  aes.set_IV(my_iv);
  aes.get_IV(iv);
  aes.do_aes_decrypt(cipher, paddedLength, check, key, 128, iv);

  // Fin de chaîne pour affichage
  check[plainLength] = '\0';

  SerialUSB.print("Temperature originale : ");
  SerialUSB.println(texteTemp);

  SerialUSB.print("Temperature chiffree (hex) : ");
  printHex(cipher, paddedLength);

  SerialUSB.print("Temperature dechiffree : ");
  SerialUSB.println((char*)check);

  SerialUSB.println("--------------------------------");
}