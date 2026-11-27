# Création d’un capteur connecté — Livrable 1 & 2

**Groupe : A**  
**Appareil :** `sensor_a`  
**Livrables :**
- `livrable-1` : version correspondant au Livrable 1 (mise en route, heartbeat, compteur et reset)
- - `livrable-2` : version finale correspondant au Livrable 2 (mesure capteur, contrôle LED RGB, tuile Godot complète et documentation du contrat)

## 1. Présentation du projet

Ce projet met en œuvre un capteur connecté basé sur une carte **ESP32-C3** communiquant via la bibliothèque **MPS** avec la passerelle du TP et une interface graphique **Godot**.

- **Livrable 1 (`livrable-1`) :** mise en route de la liaison capteur <-> passerelle <-> IHM, heartbeat, compteur d'exemple et remise à zéro (`reset`).
- **Livrable 2 (`livrable-2`) :** lecture d'une donnée de capteur (broche analogique/numérique et/ou DHT11 en bonus), pilotage de l'état et de la couleur de la LED RGB intégrée depuis l'IHM, gestion complète de l'état/déconnexion côté Godot, et respect strict des conventions et contrats de messages.

## 2. Fonctionnalités

### Livrable 1 (tag `livrable-1`)

- Enregistrement du capteur `sensor_a` auprès de la passerelle.
- Affichage de son identifiant (`node_id`) et de son état de connexion.
- Émission d'un heartbeat toutes les secondes pour signaler que le capteur fonctionne.
- Incrémentation et publication d'un compteur toutes les 2 secondes.
- Bouton `Reset` dans l'IHM pour demander la remise à zéro du compteur.
- Mise à jour du compteur dans l'IHM.
- Republication de la valeur actuelle du compteur à chaque lancement.

### Livrable 2 (tag `livrable-2`)
- **Acquisition de mesure :** lecture d'une broche analogique ou numérique (câblée ou non) — *et/ou capteur DHT11 sur pin `0` (température et humidité dans des unités lisibles) si bonus réalisé*.
- **Contrôle de la LED RGB intégrée (ESP32-C3) :**
  - Commande d'allumage / extinction (`cmd_...`) déclenchée depuis l'IHM.
  - Définition de la couleur (`set_...`) générée côté IHM (ex. slider ou couleur aléatoire) et appliquée par le capteur.
- **Fiabilité et non-blocage :** structures C++ `packed` de taille fixe vérifiée à la compilation (`static_assert`), noms de champs intégrant l'unité (ex. `centiDegrees`, `periodMs`, etc.), `loop()` et handlers non bloquants (pas d'attente bloquante).
- **Synchronisation et session :** republication complète de l'état (mesures, état et couleur de la LED) au démarrage et à chaque ouverture de session.
- **Robustesse des commandes :** rejet des messages de taille inattendue et confirmation de chaque commande/réglage appliqué par un message `status_...`.
- **IHM Godot :**
  - Noms de subjects identiques des deux côtés (construits à partir de `DEVICE_NAME` / identifiant appareil) et validation stricte de la taille des payloads avant décodage.
  - Mise à jour de l'affichage uniquement sur réception des `status_...` (jamais directement au clic utilisateur).
  - Gestion de la déconnexion : valeurs marquées comme périmées (ex. grisées) et commandes indisponibles tant que le capteur n'est pas reconnecté.

## 3. Matériel et configuration

### Matériel

- Une carte ESP32-C3.
- La passerelle MPS.
- Il y a aucun cablage supplémentaire pour cette partie. 

### Configuration Arduino

Les paramètres de connexion sont centralisés dans `config.h` :

| Paramètre | Valeur |
|---|---|
| Nom de l'appareil (`DEVICE_NAME`) | `sensor_a` |
| Adresse MAC de la passerelle (`kGatewayMacAddress`) | `90:DA:72:49:03:6C` **cette valeur est variable en fonction de la passserelle utilisée**
| Canal Wi-Fi (`kWifiChannel`) | `3` **cette valeur est variable en fonction de la passserelle utilisée**
| Vitesse du moniteur série | `115200` bauds |


## 4. Organisation des fichiers

Les deux livrables sont suivis dans le même dépôt GitHub via des **tags** :

```bash
# Consulter les livrables
git tag
git checkout livrable-1   # État du Livrable 1
git checkout livrable-2   # État final du Livrable 2
```

Organisation du dépôt :

```text
groupe_a_livrable_1/
├── README.md
├── Boilerplate_groupeA/
│       ├── Boilerplate_groupeA.ino
│       └── config.h
└── hmi/
    ├── project.godot
    ├── addons/
    ├── core/
    ├── main/
    ├── style/
    └── tiles/
        ├── sensor_boiler/
        │   ├── tile.gd
        │   └── tile.tscn
        ├── Led/
        │   ├── tile.gd
        │   └── tile.tscn
        └── capteur/
            ├── tile.gd
            └── tile.tscn
```

## 5. Communication entre le capteur et l'IHM

### `hb_sensor_a`

| Élément | Description |
|---|---|
| Émetteur | ESP32-C3 |
| Destinataire | IHM, via la passerelle |
| Rôle | Indiquer que le programme du capteur est actif |
| Fréquence | 1 seconde |
| Envoi | `MPS.publishHeartbeat(kSubHb)` |
| Payload | Format de heartbeat géré par la bibliothèque MPS, sans structure personnalisée dans ce projet |

### `status_sensor_a_counter`

| Élément | Description |
|---|---|
| Émetteur | ESP32-C3 |
| Destinataire | IHM, via la passerelle |
| Rôle | Transmettre la valeur réelle du compteur |
| Fréquence | Toutes les 2 secondes, après un Reset et à chaque ouverture de session |
| Structure C++ | `CounterPayload` (`packed`) |
| Taille totale | 4 octets, vérifiée par `static_assert` |

| Offset | Champ | Type | Unité | Taille |
|---|---|---|---|---|
| 0 | `value` | `uint32_t` | Sans unité (nombre d'incréments) | 4 octets |

Côté Godot, le payload est accepté uniquement s'il contient exactement **4 octets**, puis décodé avec `decode_u32(0)`.

### `cmd_sensor_a_reset_counter`

| Élément | Description |
|---|---|
| Émetteur | IHM Godot |
| Destinataire | ESP32-C3, via la passerelle |
| Rôle | Demander la remise à zéro du compteur |
| Payload | Vide : aucun champ, **0 octet** |
| Réponse | Publication immédiate de `status_sensor_a_counter` avec la nouvelle valeur |

Le programme Arduino ignore une commande Reset dont le payload n'est pas vide. L'IHM **ne met pas directement le compteur à zéro** lors du clic : elle attend de recevoir le `status_` du capteur.

## 6. Fonctionnement du programme

- Au démarrage, l'ESP32-C3 initialise la communication MPS et s'enregistre auprès de la passerelle.
- La boucle Arduino appelle `MPS.loop()` et planifie le heartbeat ainsi que le compteur à l'aide de `millis()`, **sans `delay()`**.
- À l'ouverture d'une session, `onSessionOpened()` republie la valeur actuelle du compteur, car la passerelle ne conserve pas les anciens messages.
- Lorsque l'utilisateur clique sur **Reset**, Godot publie la commande `cmd_sensor_a_reset_counter`.
- Le capteur vérifie la commande, remet son compteur à zéro et publie son nouvel état. Godot met alors à jour l'affichage.

## 7. Périmètre du livrable

Ce **livrable 1** couvre la configuration et la mise en route du système connecté (objectifs 1 à 5 du sujet). La lecture d'une véritable donnée de capteur, le pilotage de la LED RGB et les fonctionnalités avancées de la tuile Godot appartiennent au **livrable 2**.
