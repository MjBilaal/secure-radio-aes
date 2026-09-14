# secure-radio-aes

Capteur de température et d'humidité sur Arduino Due qui transmet ses mesures par radio IEEE 802.15.4 (MRF24J40), avec intégrité CRC16, chiffrement AES-128 et changement de canal en cas de brouillage.

## Description

Progression en huit sketches, de la lecture capteur à l'émission chiffrée résistante au brouillage :

| Sketch | Rôle |
|---|---|
| `dht22_lecture` | Lecture DHT22 : humidité, température (°C / °F), température ressentie |
| `crc16_commandes_serie` | Commandes série `start` / `stop` / `start_fr` / `start_en` au format `commande\|CRC`, rejetées si le CRC16 Modbus est invalide |
| `aes128_test_local` | Chiffrement puis déchiffrement AES-128 de la température, affichage hexadécimal |
| `radio_tx_simple` | Émetteur MRF24J40 : envoie `abcd` à `0x6002` (PAN `0xCAFE`, canal 11) avec accusé de réception |
| `radio_rx_simple` | Récepteur MRF24J40 : adresse `0x6002`, affiche les trames reçues |
| `radio_tx_capteur` | Émission des mesures en clair : `ID=Bilal;H=..;T=..` (PAN `0xBABA`, canal 14, `0xB1A1` → `0xB1B2`) |
| `radio_tx_aes_crc` | Ajout d'un CRC16 à la trame, chiffrement AES-128, envoi en hexadécimal |
| `radio_tx_aes_antijam` | Même trame chiffrée ; après 3 échecs d'émission consécutifs, bascule sur le canal suivant parmi 14, 17, 18, 19, 20 |

`lib/mrf24j/` contient la bibliothèque MRF24J40 de Karl Palsson (licence BSD modifiée / Apache), modifiée pour le TP : `send16` prend la longueur en paramètre (`send16(dest, len, data)`) au lieu de la calculer avec `strlen`, ce qui permet d'envoyer des octets bruts contenant `0x00`.

## Stack technique

- Arduino Due (SAM3X8E), port USB natif (`SerialUSB`)
- Capteur DHT22 sur la broche 7
- Module radio MRF24J40 (IEEE 802.15.4, 2,4 GHz) en SPI : RESET = 6, CS = 52, INT = 5
- Bibliothèques : DHT sensor library et Adafruit Unified Sensor (Adafruit), [AES](https://github.com/spaniakos/AES) (spaniakos), Mrf24j
- C++ / Arduino

## Structure

```
.
├── lib/mrf24j/        # Bibliothèque MRF24J40 modifiée (mrf24j.h, mrf24j.cpp)
└── sketches/
    ├── dht22_lecture/
    ├── crc16_commandes_serie/
    ├── aes128_test_local/
    ├── radio_tx_simple/
    ├── radio_rx_simple/
    ├── radio_tx_capteur/
    ├── radio_tx_aes_crc/
    └── radio_tx_aes_antijam/
```

## Installation et lancement

1. Arduino IDE → Gestionnaire de cartes → installer **Arduino SAM Boards (32-bits ARM Cortex-M3)**, puis choisir **Arduino Due (Native USB Port)**.
2. Gestionnaire de bibliothèques → installer **DHT sensor library**, **Adafruit Unified Sensor** et **AES** (spaniakos).
3. Installer la bibliothèque MRF24J40 :
   - les sketches appellent `mrf.send16(dest, payload)`, la signature de la bibliothèque d'origine ([karlp/Mrf24j40-arduino-library](https://github.com/karlp/Mrf24j40-arduino-library)) ;
   - pour utiliser la version modifiée, copier `lib/mrf24j/` dans le dossier `libraries/` de l'IDE et appeler `mrf.send16(dest, strlen(payload), payload)`.
4. Ouvrir un sketch (`sketches/<nom>/<nom>.ino`), téléverser, puis ouvrir le moniteur série :
   - 9600 bauds pour `dht22_lecture`, `crc16_commandes_serie` et `aes128_test_local` ;
   - 115200 bauds pour les sketches radio.

Pour tester la radio, téléverser `radio_tx_simple` sur une carte et `radio_rx_simple` sur une seconde.

Commandes de test pour `crc16_commandes_serie` : `start|624F`, `stop|AE76`, `start_fr|2083`, `start_en|1982`.

## Ce que j'ai appris / côté sécurité

Le code chiffre correctement au sens fonctionnel, mais plusieurs choix sont volontairement simples pour le TP. Les identifier fait partie de l'exercice :

- **Clé publique et codée en dur.** `2b7e1516 28aed2a6 abf71588 09cf4f3c` est la clé de test du standard NIST (FIPS-197 / SP 800-38A). Même avec une clé aléatoire, une clé compilée dans le firmware se récupère par lecture de la flash (JTAG/SWD). Il faudrait une clé par équipement, provisionnée hors du code.
- **IV fixe (`36753562`) réutilisé à chaque trame.** Le chiffrement devient déterministe : une même trame en clair donne toujours la même trame chiffrée. Le premier bloc de 16 octets (`ID=Bilal;H=45.20`) ne dépend que de l'humidité. Un observateur passif peut donc repérer les valeurs identiques et construire un dictionnaire bloc chiffré ↔ mesure, sans connaître la clé.
- **Le CRC16 n'authentifie rien.** Il détecte les erreurs de transmission, mais n'importe qui peut le recalculer. Sur le port série, `commande|CRC` n'empêche pas d'envoyer `stop|AE76`. Pour garantir l'origine et l'intégrité, il faut un MAC (HMAC) ou un chiffrement authentifié, par exemple AES-CCM, prévu par la couche sécurité de 802.15.4.
- **Pas de protection contre le rejeu.** La trame ne contient ni compteur ni horodatage : une trame capturée peut être renvoyée telle quelle.
- **Anti-brouillage réactif et prévisible.** Le changement de canal intervient après 3 échecs, dans une liste fixe et connue. Un brouilleur peut suivre la même séquence, et le récepteur doit changer de canal de la même façon (aucun mécanisme de synchronisation n'est prévu).
- **Taille de trame.** La trame chiffrée (48 octets) est envoyée en hexadécimal séparé par des espaces, soit 143 caractères. Cela dépasse la taille maximale d'une trame 802.15.4 (127 octets, dont 116 de données utiles). Envoyer les octets bruts réduirait la charge à 48 octets : c'est l'intérêt de la modification de `send16` avec longueur explicite.
