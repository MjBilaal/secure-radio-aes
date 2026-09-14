#include <DHT.h>
#include <Adafruit_Sensor.h>

// Constantes
#define DHTPIN 7           // Broche sur laquelle le capteur est câblé
#define DHTTYPE DHT22      // Type de capteur

// Déclaration du capteur
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Lancement de la liaison série sur Arduino Due (Native USB Port)
  SerialUSB.begin(9600);
  while (!SerialUSB) {
    ; // attente de l'ouverture du moniteur série
  }

  // Initialisation du capteur
  dht.begin();
}

void loop() {
  // Attente avant la prochaine mesure
  delay(2000);

  // Humidité
  float humidite = dht.readHumidity();

  // Température en Celsius
  float tempCelcius = dht.readTemperature();

  // Température en Fahrenheit
  float tempFahr = dht.readTemperature(true);

  // Vérification qu'il n'y ait pas d'erreurs de lecture
  if (isnan(humidite) || isnan(tempCelcius) || isnan(tempFahr)) {
    SerialUSB.println("Erreur de lecture du capteur DHT!");
    return;
  }

  // Température ressentie
  float tempRessentie = dht.computeHeatIndex(tempCelcius, humidite, false);

  // Affichage sur le moniteur série
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
}