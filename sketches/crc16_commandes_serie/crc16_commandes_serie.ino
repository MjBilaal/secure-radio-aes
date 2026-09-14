#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <stdlib.h>

// Constantes
#define DHTPIN 7
#define DHTTYPE DHT22

// Déclaration du capteur
DHT dht(DHTPIN, DHTTYPE);

// Variables de contrôle
bool acquisition = false;
bool langueFrancais = true;

unsigned long dernierTemps = 0;
const unsigned long intervalle = 2000; // 2 secondes

String trame = "";

// Fonction CRC16 Modbus
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

// Fonction pour convertir le CRC en texte hexadécimal sur 4 caractères
String crcToHex(uint16_t crc) {
  char buffer[5];
  sprintf(buffer, "%04X", crc);
  return String(buffer);
}

void setup() {
  SerialUSB.begin(9600);
  while (!SerialUSB) {
    ;
  }

  dht.begin();

  SerialUSB.println("Envoyer : commande|CRC");
  SerialUSB.println("Exemples :");
  SerialUSB.println("start|624F");
  SerialUSB.println("stop|AE76");
  SerialUSB.println("start_fr|2083");
  SerialUSB.println("start_en|1982");
}

void loop() {
  // Lecture de la trame reçue dans le moniteur série
  if (SerialUSB.available()) {
    trame = SerialUSB.readStringUntil('\n');
    trame.trim();

    int separateur = trame.indexOf('|');

    if (separateur == -1) {
      SerialUSB.println("Format invalide. Utiliser : commande|CRC");
      return;
    }

    String commande = trame.substring(0, separateur);
    String crcRecuTexte = trame.substring(separateur + 1);

    commande.trim();
    crcRecuTexte.trim();
    crcRecuTexte.toUpperCase();

    uint16_t crcCalcule = crc16Modbus((const uint8_t *)commande.c_str(), commande.length());
    String crcCalculeTexte = crcToHex(crcCalcule);

    // Vérification du CRC
    if (crcRecuTexte != crcCalculeTexte) {
      SerialUSB.print("CRC invalide ! Attendu: ");
      SerialUSB.print(crcCalculeTexte);
      SerialUSB.print("  Recu: ");
      SerialUSB.println(crcRecuTexte);
      return;
    }

    // CRC valide
    SerialUSB.println("CRC valide");

    if (commande == "start") {
      acquisition = true;
      SerialUSB.println("Acquisition demarree");
    }
    else if (commande == "stop") {
      acquisition = false;
      SerialUSB.println("Acquisition arretee");
    }
    else if (commande == "start_fr") {
      acquisition = true;
      langueFrancais = true;
      SerialUSB.println("Acquisition demarree en francais");
    }
    else if (commande == "start_en") {
      acquisition = true;
      langueFrancais = false;
      SerialUSB.println("Acquisition started in English");
    }
    else {
      SerialUSB.println("Commande inconnue");
    }
  }

  // Acquisition toutes les 2 secondes si elle est active
  if (acquisition && millis() - dernierTemps >= intervalle) {
    dernierTemps = millis();

    float humidite = dht.readHumidity();
    float tempCelcius = dht.readTemperature();
    float tempFahr = dht.readTemperature(true);

    if (isnan(humidite) || isnan(tempCelcius) || isnan(tempFahr)) {
      if (langueFrancais) {
        SerialUSB.println("Erreur de lecture du capteur DHT !");
      } else {
        SerialUSB.println("DHT sensor reading error!");
      }
      return;
    }

    float tempRessentie = dht.computeHeatIndex(tempCelcius, humidite, false);

    if (langueFrancais) {
      SerialUSB.print("Humidite: ");
      SerialUSB.print(humidite);
      SerialUSB.print(" % ");

      SerialUSB.print("Temperature: ");
      SerialUSB.print(tempCelcius);
      SerialUSB.print(" *C ");

      SerialUSB.print(tempFahr);
      SerialUSB.print(" *F ");

      SerialUSB.print("Temperature ressentie: ");
      SerialUSB.print(tempRessentie);
      SerialUSB.println(" *C");
    } else {
      SerialUSB.print("Humidity: ");
      SerialUSB.print(humidite);
      SerialUSB.print(" % ");

      SerialUSB.print("Temperature: ");
      SerialUSB.print(tempCelcius);
      SerialUSB.print(" *C ");

      SerialUSB.print(tempFahr);
      SerialUSB.print(" *F ");

      SerialUSB.print("Heat index: ");
      SerialUSB.print(tempRessentie);
      SerialUSB.println(" *C");
    }
  }
}