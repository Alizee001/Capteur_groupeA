#include <MPS.h>
#include "config.h"

// ---- Constantes ----
constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kHeartbeatPeriodMs = 1000;
constexpr uint32_t kCounterPeriodMs = 2000;
constexpr size_t kMacAddressSize = 6;

// ---- Structures de payload ----
struct __attribute__((packed)) CounterPayload {
  uint32_t value;
};
static_assert(sizeof(CounterPayload) == 4, "CounterPayload = 4 octets");


// ---- Variables globales ----
static CounterPayload counter = {0};


// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME);
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset_counter");


// ---- Callbacks ----

// Appelé à l'ouverture de la session : republie l'état, la passerelle ne garde rien.
static void onSessionOpened() {
  Serial.println("Session opened!");
  MPS.publishValue(kSubStatusCounter, counter);
}

// Remet le compteur à 0 sur commande, puis confirme par un status_.
static void onCmdResetCounter(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return; // ce format n'a pas de payload
  counter.value = 0;
  MPS.publishValue(kSubStatusCounter, counter); // l'IHM voit le résultat réel
}

// Appelé en cas d'échec d'envoi : signale le message perdu sur le moniteur série.
static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}


// ---- Fonctions appelées par setup/loop ----

// Envoie un heartbeat toutes les kHeartbeatPeriodMs, sans bloquer.
// Il prouve à l'IHM que le code "métier" du capteur n'est pas planté.
void publishHeartbeat() {
  static uint32_t lastHeartbeatTimeMs = 0;
  if (millis() - lastHeartbeatTimeMs >= kHeartbeatPeriodMs) {
    lastHeartbeatTimeMs = millis();
    MPS.publishHeartbeat(kSubHb);
  }
}

// Incrémente et publie le compteur toutes les kCounterPeriodMs, sans bloquer.
void publishCounter() {
  static uint32_t lastCounterTimeMs = 0;
  if (millis() - lastCounterTimeMs >= kCounterPeriodMs) {
    lastCounterTimeMs = millis();
    counter.value++;
    MPS.publishValue(kSubStatusCounter, counter);
  }
}


// ---- Fonctions Arduino ----

// Exécutée une fois au démarrage : configure le log, les callbacks et démarre le client MPS.
void setup() {
  Serial.begin(kSerialBaudRate);
  MPS.setLogOutput(&Serial);

  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  static const uint8_t kUnsetMacAddress[kMacAddressSize] = {0};
  if (memcmp(kGatewayMacAddress, kUnsetMacAddress, kMacAddressSize) == 0) {
    Serial.println("MPS: renseignez kGatewayMacAddress dans config.h (adresse affichee par l'IHM)");
    return;
  }

  MPS.on(kSubCmdResetCounter, onCmdResetCounter);

  if (!MPS.begin(kGatewayMacAddress, DEVICE_NAME, kWifiChannel)) Serial.println("MPS: demarrage impossible");
}

// Exécutée en boucle : tout doit rester non bloquant (pas de delay()).
void loop() {
  MPS.loop();
  publishCounter();
  publishHeartbeat();
}