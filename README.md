# Création d’un capteur connecté — Livrables 1 et 2

**Groupe A**  
**Nom du capteur** `sensor_a`

- **Livrable 1** — tag `livrable-1` : mise en route, heartbeat, compteur et remise à zéro.
- **Livrable 2** — tag `livrable-2` : mesure analogique, température et humidité, pilotage de la LED RGB et gestion de la déconnexion dans Godot.

## 1. Présentation du projet

Ce projet met en œuvre un capteur connecté sur une carte **ESP32-C3**. Le firmware utilise la bibliothèque **MPS** pour communiquer avec la passerelle du TP, reliée en USB à l’ordinateur qui exécute l’interface **Godot**.

La carte publie ses données vers l’interface. L’interface envoie des commandes à la carte pour remettre le compteur à zéro dans le livrable 1 et pour piloter la LED RGB dans le livrable 2.

## 2. Fonctionnalités

### Livrable 1

- Enregistrement de `sensor_a` auprès de la passerelle et affichage de son identifiant `node_id`.
- Émission d’un heartbeat toutes les secondes pour indiquer que le capteur est actif.
- Incrémentation, publication et affichage du compteur toutes les 2 secondes.
- Remise à zéro à l’aide du bouton **Reset**. L’affichage est mis à jour lorsque le capteur confirme la nouvelle valeur.
- Republication de la valeur courante du compteur à l’ouverture d’une session MPS.

### Livrable 2 

- Lecture de l’entrée analogique `GPIO 4` et affichage dans Godot de la valeur brute et d’une estimation de la tension.
- Lecture du **DHT11 sur `GPIO 0`** et affichage de la température en °C et de l’humidité relative en %. Cette fonctionnalité correspond au bonus du TP.
- Allumage, extinction et changement de couleur de la LED RGB SK6812 depuis Godot. Le bouton **Led On/off** alterne entre le blanc et l’extinction. Quand la LED est allumée, **Change color** choisit une couleur aléatoire dans Godot.
- Validation de la taille des messages avant leur traitement. Chaque commande appliquée est confirmée par un message `status_`.
- Après **3,5 secondes sans heartbeat**, les mesures sont grisées et les boutons LED sont désactivés. La reprise du heartbeat rétablit l’état connecté.

## 3. Matériel et configuration

### Matériel

- Une carte **ESP32-C3** du TP avec une LED RGB SK6812 et un DHT11 précâblés.
- Une passerelle **MPS**.
- Des câbles USB de données pour relier la passerelle et l’ESP32-C3 à l’ordinateur.

| Élément | Broche | Utilisation |
| --- | --- | --- |
| LED RGB SK6812 | `GPIO 2` (`G2`) | Pilotée avec `rgbLedWrite()`. |
| DHT11 | `GPIO 0` (`G0`) | Mesure de la température et de l’humidité relative. |
| Entrée analogique | `GPIO 4` (`G4`) | Lecture avec `analogRead()`. La broche est laissée libre sur la carte du TP, ce qui explique les fluctuations. |

### Configuration Arduino

Les paramètres de connexion se trouvent dans [`config.h`](Boilerplate_groupeA/config.h). La MAC et le canal ci-dessous correspondent à la configuration enregistrée dans `main`. Les adapter à la passerelle utilisée.

| Paramètre | Valeur |
| --- | --- |
| Nom de l’appareil (`DEVICE_NAME`) | `sensor_a` |
| Adresse MAC de la passerelle (`kGatewayMacAddress`) | `variable` |
| Canal Wi-Fi de la passerelle (`kWifiChannel`) | `variable` |
| Vitesse du moniteur série Arduino | `115200` bauds |

### Lancement

1. Brancher la passerelle en USB. Ouvrir [`hmi/project.godot`](hmi/project.godot) avec **Godot 4.7**, puis lancer l’IHM.
2. Attendre la détection automatique de la passerelle. Relever son adresse MAC et son canal Wi-Fi dans le journal Godot.
3. Dans l’IDE Arduino, installer le support de carte ESP32, la bibliothèque MPS fournie pour le TP, **DHT sensor library** et **Adafruit Unified Sensor**.
4. Ouvrir [`Boilerplate_groupeA.ino`](Boilerplate_groupeA/Boilerplate_groupeA.ino) et renseigner la MAC et le canal de la passerelle dans `config.h`.
5. Sélectionner l’ESP32-C3 et son port série, puis téléverser le croquis. Vérifier que le `node_id` apparaît dans Godot et que l’icône du heartbeat clignote.

## 4. Organisation des fichiers

```bash
# Récupérer le dépôt
git clone https://github.com/Alizee001/Capteur_groupeA.git
cd Capteur_groupeA

# Consulter le livrable 1
git switch --detach livrable-1

# Consulter le livrable 2
git switch --detach livrable-2
```

| Fichier ou dossier | Rôle |
| --- | --- |
| [`Boilerplate_groupeA/Boilerplate_groupeA.ino`](Boilerplate_groupeA/Boilerplate_groupeA.ino) | Firmware Arduino, lectures des capteurs et gestion des messages MPS. |
| [`Boilerplate_groupeA/config.h`](Boilerplate_groupeA/config.h) | Paramètres de connexion du capteur. |
| [`hmi/project.godot`](hmi/project.godot) | Projet Godot à importer. |
| [`hmi/tiles/Led/`](hmi/tiles/Led/) | Script et scène de la tuile LED. |
| [`hmi/tiles/capteur/`](hmi/tiles/capteur/) | Script et scène de la tuile de mesures. |
| [`hmi/tiles/sensor_boiler/`](https://github.com/Alizee001/Capteur_groupeA/tree/livrable-1/hmi/tiles/sensor_boiler) | Tuile du livrable 1 avec le compteur et le bouton **Reset**. |

## 5. Communication entre le capteur et l’IHM

Tous les messages passent par la passerelle MPS. Les noms des sujets sont construits à partir de `DEVICE_NAME`. Les offsets indiquent la position de chaque champ dans le payload, en octets à partir de `0`. Les entiers sur plusieurs octets sont transmis en *little-endian*.

| Sujet | Sens | Taille du payload | Rôle et déclenchement |
| --- | --- | --- | --- |
| `hb_sensor_a` | ESP32-C3 → Godot | Format géré par MPS | Signal de présence envoyé toutes les secondes avec `MPS.publishHeartbeat()`. |
| `status_sensor_a_counter` | ESP32-C3 → Godot | 4 octets | Valeur du compteur envoyée toutes les 2 secondes, après un reset et à l’ouverture d’une session. |
| `cmd_sensor_a_reset_counter` | Godot → ESP32-C3 | 0 octet | Commande envoyée par le bouton **Reset**. Le capteur répond avec `status_sensor_a_counter` contenant `0`. |
| `status_sensor_a_analog` | ESP32-C3 → Godot | 2 octets | Valeur brute de `GPIO 4` envoyée toutes les secondes. La dernière lecture disponible est republiée à l’ouverture d’une session. |
| `status_sensor_a_environment` | ESP32-C3 → Godot | 4 octets | Température et humidité du DHT11. La lecture est tentée toutes les 2 secondes et publiée si elle est valide. La dernière mesure valide est republiée à l’ouverture d’une session. |
| `cmd_sensor_a_set_led` | Godot → ESP32-C3 | 3 octets | Valeurs RGB envoyées par **Led On/off** ou **Change color**. Le capteur applique les valeurs et confirme avec `status_sensor_a_led`. |
| `status_sensor_a_led` | ESP32-C3 → Godot | 3 octets | Valeurs RGB effectivement appliquées, envoyées après chaque commande valide et à l’ouverture d’une session. L’état initial est `(0, 0, 0)`. |

### Champs des payloads

| Sujet(s) | Offset | Champ | Type | Taille | Unité ou plage |
| --- | --- | --- | --- | --- | --- |
| `status_sensor_a_counter` | `0` | `value` | `uint32_t` | 4 octets | Nombre d’incréments. |
| `status_sensor_a_analog` | `0` | `adcRawCounts` | `uint16_t` | 2 octets | Valeur brute ADC sur 12 bits, de `0` à `4095`. |
| `status_sensor_a_environment` | `0` | `temperatureCentiDegrees` | `int16_t` | 2 octets | Centièmes de degré Celsius. |
| `status_sensor_a_environment` | `2` | `humidityCentiPercent` | `uint16_t` | 2 octets | Centièmes de point de pourcentage d’humidité relative. |
| `cmd_sensor_a_set_led`, `status_sensor_a_led` | `0` | `r` | `uint8_t` | 1 octet | Intensité du rouge, de `0` à `255`. |
| `cmd_sensor_a_set_led`, `status_sensor_a_led` | `1` | `g` | `uint8_t` | 1 octet | Intensité du vert, de `0` à `255`. |
| `cmd_sensor_a_set_led`, `status_sensor_a_led` | `2` | `b` | `uint8_t` | 1 octet | Intensité du bleu, de `0` à `255`. |

Le heartbeat utilise le format de la bibliothèque MPS. La commande `cmd_sensor_a_reset_counter` attend un payload vide. Pour la LED, `(0, 0, 0)` correspond à l’extinction et `(255, 255, 255)` au blanc.

## 6. Fonctionnement du programme

- **Démarrage** — l’ESP32-C3 initialise le DHT11 et la communication MPS, puis s’enregistre auprès de la passerelle. La LED est initialement éteinte.
- **Publications périodiques** — la boucle Arduino appelle `MPS.loop()` et utilise `millis()` pour déclencher le heartbeat, le compteur et les lectures, sans `delay()` explicite.
- **Ouverture de session** — `onSessionOpened()` republie le compteur, l’état RGB de la LED et les dernières mesures disponibles.
- **Validation** — les structures `CounterPayload`, `AnalogPayload`, `EnvironmentPayload` et `LedPayload` sont déclarées `packed`. Leurs tailles sont vérifiées avec `static_assert`. Le firmware ignore les commandes de taille inattendue et Godot vérifie la taille des statuts avant le décodage.
- **Affichage des mesures** — Godot estime la tension analogique avec `valeur brute × 3,3 / 4095`. Pour le DHT11, Godot divise les deux valeurs reçues par `100`. Si la lecture DHT11 échoue, le firmware conserve la dernière mesure valide.
- **Confirmation des commandes** — le clic déclenche l’envoi d’une commande. L’IHM actualise la valeur du compteur ou l’état de la LED à la réception du `status_` correspondant.

## 7. Périmètre et vérification du livrable

Le **livrable 1** contient la tuile de mise en route avec le compteur et le bouton **Reset**. Le **livrable 2** contient les tuiles de mesures et de commande de la LED. Le firmware de `main` conserve le compteur et sa commande de remise à zéro. Leur démonstration dans l’IHM se fait avec la tuile du tag `livrable-1`.

| Vérification | Résultat attendu |
| --- | --- |
| Démarrer avec la passerelle branchée | Le `node_id` apparaît et le heartbeat clignote. |
| Cliquer sur **Reset** dans le livrable 1 | Le compteur affiche `0` après réception de la confirmation. |
| Consulter la tuile de mesures dans `main` | L’IHM affiche la valeur analogique, la température en °C et l’humidité en %. |
| Allumer la LED, demander une couleur, puis l’éteindre | La LED physique et le statut affiché correspondent. Une couleur tirée au hasard peut être identique à la précédente. |
| Débrancher puis rebrancher l’ESP32-C3 | Les mesures sont grisées et les boutons LED désactivés pendant la déconnexion. Les mesures et les commandes redeviennent accessibles à la reconnexion. |