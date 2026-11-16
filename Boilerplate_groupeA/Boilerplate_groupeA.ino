#include <MPS.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "config.h"

// ---- Constantes ----
constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kHeartbeatPeriodMs = 1000;
constexpr uint32_t kCounterPeriodMs = 2000;
constexpr uint32_t kAnalogPeriodMs = 1000;
constexpr uint32_t kEnvironmentPeriodMs = 2000;
constexpr size_t kMacAddressSize = 6;
constexpr uint8_t kAnalogPin = 4;
constexpr uint8_t kDhtPin = 0;
constexpr uint8_t kDhtType = DHT11;
constexpr float kCentiPerUnit = 100.0f;

// ---- Structures de payload ----
// Valeur du compteur, sur 4 octets.
struct __attribute__((packed)) CounterPayload {
  uint32_t value;
};
static_assert(sizeof(CounterPayload) == 4, "CounterPayload = 4 octets");

// Lecture brute du convertisseur analogique, sur 2 octets.
struct __attribute__((packed)) AnalogPayload {
  uint16_t adcRawCounts;
};
static_assert(sizeof(AnalogPayload) == 2, "AnalogPayload = 2 octets");

// Mesures du DHT11 en centiemes d'unite, sur 4 octets.
struct __attribute__((packed)) EnvironmentPayload {
  int16_t temperatureCentiDegrees;
  uint16_t humidityCentiPercent;
};
static_assert(sizeof(EnvironmentPayload) == 4, "EnvironmentPayload = 4 octets");

// ---- Variables globales ----
static CounterPayload counter = {0};
static AnalogPayload analogReading = {0};
static EnvironmentPayload environment = {0, 0};
static bool isAnalogValid = false;
static bool isEnvironmentValid = false;
static DHT dht(kDhtPin, kDhtType);

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME);
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset_counter");
constexpr mps::Subject kSubStatusAnalog = mps::makeSubject("status_" DEVICE_NAME "_analog");
constexpr mps::Subject kSubStatusEnvironment = mps::makeSubject("status_" DEVICE_NAME "_environment");

// ---- Callbacks ----

// Republie les dernieres valeurs disponibles quand la session s'ouvre.
// La passerelle ne conserve pas les messages precedents.
static void onSessionOpened() {
  Serial.println("Session opened!");
  MPS.publishValue(kSubStatusCounter, counter);

  if (isAnalogValid) {
    MPS.publishValue(kSubStatusAnalog, analogReading);
  }

  if (isEnvironmentValid) {
    MPS.publishValue(kSubStatusEnvironment, environment);
  }
}

// Remet le compteur a zero uniquement si la commande a un payload vide.
// Publie ensuite l'etat reel pour confirmer l'action a l'IHM.
static void onCmdResetCounter(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return;

  counter.value = 0;
  MPS.publishValue(kSubStatusCounter, counter);
}

// Signale les envois perdus afin de faciliter le diagnostic.
static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId,
                         mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n",
                sequenceNumber, (unsigned long)subjectId);
}

// ---- Fonctions appelees par setup/loop ----

// Indique si une periode est ecoulee sans mettre la boucle en attente.
static bool hasPeriodElapsed(uint32_t& lastTimeMs, uint32_t periodMs) {
  const uint32_t nowMs = millis();
  if (nowMs - lastTimeMs < periodMs) return false;

  lastTimeMs = nowMs;
  return true;
}

// Envoie un heartbeat periodique pour signaler que le capteur fonctionne.
static void publishHeartbeat() {
  static uint32_t lastHeartbeatTimeMs = 0;
  if (!hasPeriodElapsed(lastHeartbeatTimeMs, kHeartbeatPeriodMs)) return;

  MPS.publishHeartbeat(kSubHb);
}

// Incremente et publie le compteur toutes les deux secondes.
static void publishCounter() {
  static uint32_t lastCounterTimeMs = 0;
  if (!hasPeriodElapsed(lastCounterTimeMs, kCounterPeriodMs)) return;

  counter.value++;
  MPS.publishValue(kSubStatusCounter, counter);
}

// Lit la broche analogique et publie sa valeur brute.
static void publishAnalog() {
  static uint32_t lastAnalogTimeMs = 0;
  if (!hasPeriodElapsed(lastAnalogTimeMs, kAnalogPeriodMs)) return;

  analogReading.adcRawCounts = static_cast<uint16_t>(analogRead(kAnalogPin));
  isAnalogValid = true;
  MPS.publishValue(kSubStatusAnalog, analogReading);
}

// Lit le DHT11 et publie temperature et humidite en centiemes d'unite.
// Une mesure invalide est ignoree et ne remplace pas la derniere valide.
static void publishEnvironment() {
  static uint32_t lastEnvironmentTimeMs = 0;
  if (!hasPeriodElapsed(lastEnvironmentTimeMs, kEnvironmentPeriodMs)) return;

  const float temperatureCelsius = dht.readTemperature();
  const float humidityPercent = dht.readHumidity();

  if (isnan(temperatureCelsius) || isnan(humidityPercent)) {
    Serial.println("DHT11: lecture impossible");
    return;
  }

  environment.temperatureCentiDegrees =
      static_cast<int16_t>(lroundf(temperatureCelsius * kCentiPerUnit));
  environment.humidityCentiPercent =
      static_cast<uint16_t>(lroundf(humidityPercent * kCentiPerUnit));
  isEnvironmentValid = true;

  MPS.publishValue(kSubStatusEnvironment, environment);
}

// ---- Fonctions Arduino ----

// Initialise le port serie, le DHT11, les callbacks et la connexion MPS.
void setup() {
  Serial.begin(kSerialBaudRate);
  MPS.setLogOutput(&Serial);
  dht.begin();

  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  static const uint8_t kUnsetMacAddress[kMacAddressSize] = {0};
  if (memcmp(kGatewayMacAddress, kUnsetMacAddress, kMacAddressSize) == 0) {
    Serial.println("MPS: renseignez kGatewayMacAddress dans config.h (adresse affichee par l'IHM)");
    return;
  }

  MPS.on(kSubCmdResetCounter, onCmdResetCounter);

  if (!MPS.begin(kGatewayMacAddress, DEVICE_NAME, kWifiChannel)) {
    Serial.println("MPS: demarrage impossible");
  }
}

// Fait tourner le protocole et publie les mesures sans delay explicite.
void loop() {
  MPS.loop();
  publishCounter();
  publishHeartbeat();
  publishAnalog();
  publishEnvironment();
}
