// librerias
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_bt.h>
#include <esp_system.h>

// servicio bluetooth
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// nombre del bluetooth
#define NOMBRE_DISPOSITIVO "robot soccer"

// estado del bluetooth
BLECharacteristic *pTxCharacteristic;
BLEServer *pServerGlobal;
bool bleListo = false;

// pines del driver de motores
const int pinSTBY = 13;

// motor izquierdo (A)
const int pinAIN1 = 27;
const int pinAIN2 = 26;
const int pinPWMA = 14;
const int canalPWMA = 0;

// motor derecho (B)
const int pinBIN1 = 25;
const int pinBIN2 = 33;
const int pinPWMB = 32;
const int canalPWMB = 1;

// velocidad actual (0-255), la cambia el slider
int velocidadBase = 200;

// movimiento actual
enum Movimiento { PARADO, ADELANTE, ATRAS, GIRO_IZQ, GIRO_DER };
Movimiento movimiento = PARADO;

// motor izquierdo: positivo adelante, negativo atras
void motorA(int velocidad) {
  if (velocidad >= 0) {
    digitalWrite(pinAIN1, HIGH);
    digitalWrite(pinAIN2, LOW);
  } else {
    digitalWrite(pinAIN1, LOW);
    digitalWrite(pinAIN2, HIGH);
    velocidad = -velocidad;
  }
  ledcWrite(canalPWMA, velocidad);
}

// motor derecho: positivo adelante, negativo atras
void motorB(int velocidad) {
  if (velocidad >= 0) {
    digitalWrite(pinBIN1, HIGH);
    digitalWrite(pinBIN2, LOW);
  } else {
    digitalWrite(pinBIN1, LOW);
    digitalWrite(pinBIN2, HIGH);
    velocidad = -velocidad;
  }
  ledcWrite(canalPWMB, velocidad);
}

// parar ambos motores
void detenerMotores() {
  motorA(0);
  motorB(0);
}

// pone los motores segun el movimiento y la velocidad actuales
void aplicarMovimiento() {
  switch (movimiento) {
    case ADELANTE:
      motorA(velocidadBase);
      motorB(velocidadBase);
      break;
    case ATRAS:
      motorA(-velocidadBase);
      motorB(-velocidadBase);
      break;
    case GIRO_IZQ:
      motorA(-velocidadBase / 2);
      motorB(velocidadBase / 2);
      break;
    case GIRO_DER:
      motorA(velocidadBase / 2);
      motorB(-velocidadBase / 2);
      break;
    default:
      detenerMotores();
      break;
  }
}

// cambia el movimiento y lo aplica
void mover(Movimiento nuevo) {
  movimiento = nuevo;
  aplicarMovimiento();
}

// conexion y desconexion
class MyServerCallbacks: public BLEServerCallbacks {
    // alguien se conecta
    void onConnect(BLEServer* pServer) {
      Serial.println("Conectado!");
    }
    // se pierde la conexion: para motores y vuelve a anunciarse
    void onDisconnect(BLEServer* pServer) {
      Serial.println("Desconectado! Reintentando...");
      mover(PARADO);
      delay(50);
      pServer->startAdvertising();
    }
};

// llega un comando desde la app
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      if (value.length() == 0) return;

      Serial.print("Mensaje completo: ");
      Serial.println(value);

      // adelante
      if (value == "UP") {
        mover(ADELANTE);
        Serial.println("ADELANTE");
      }
      // atras
      else if (value == "DOWN") {
        mover(ATRAS);
        Serial.println("ATRAS");
      }
      // giro a la izquierda
      else if (value == "LEFT") {
        mover(GIRO_IZQ);
        Serial.println("IZQUIERDA");
      }
      // giro a la derecha
      else if (value == "RIGHT") {
        mover(GIRO_DER);
        Serial.println("DERECHA");
      }
      // stop
      else if (value == "C") {
        mover(PARADO);
        Serial.println("STOP");
      }
      // slider de velocidad (0-100): cambia la velocidad al instante
      else if (value.startsWith("Speed_")) {
        int pos = value.lastIndexOf('_');
        int valorSlider = constrain((int)value.substring(pos + 1).toInt(), 0, 100);
        velocidadBase = map(valorSlider, 0, 100, 0, 255);
        aplicarMovimiento();
        Serial.print("Velocidad base actualizada: ");
        Serial.println(velocidadBase);
      }
    }
};

// revision corta al arrancar
void revisionInicial() {
  Serial.println();
  Serial.println("===== REVISION INICIAL =====");

  // motivo del ultimo reinicio
  esp_reset_reason_t motivo = esp_reset_reason();
  Serial.print("Arranque: ");
  switch (motivo) {
    case ESP_RST_POWERON:
      Serial.println("encendido normal (o recien subido el codigo)");
      break;
    case ESP_RST_SW:
      Serial.println("reinicio por software");
      break;
    case ESP_RST_EXT:
      Serial.println("reinicio con el boton EN/RESET");
      break;
    case ESP_RST_BROWNOUT:
      Serial.println("ADVERTENCIA: se reinicio por BROWNOUT (caida de voltaje)");
      break;
    case ESP_RST_PANIC:
      Serial.println("ADVERTENCIA: el programa se cayo (crash) y se reinicio");
      break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      Serial.println("ADVERTENCIA: reinicio por watchdog (el programa se trabo)");
      break;
    default:
      Serial.print("otro motivo (codigo ");
      Serial.print((int)motivo);
      Serial.println(")");
      break;
  }

  // estado del bluetooth
  if (bleListo) {
    Serial.print("Bluetooth: OK, anunciandose como '");
    Serial.print(NOMBRE_DISPOSITIVO);
    Serial.println("'");
  } else {
    Serial.println("Bluetooth: FALLO al iniciar");
  }
  Serial.println("============================");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // inicio de driver de motores
  pinMode(pinSTBY, OUTPUT);
  pinMode(pinAIN1, OUTPUT);
  pinMode(pinAIN2, OUTPUT);
  pinMode(pinBIN1, OUTPUT);
  pinMode(pinBIN2, OUTPUT);

  // inicio de motor izquierdo
  ledcSetup(canalPWMA, 5000, 8);
  ledcAttachPin(pinPWMA, canalPWMA);

  // inicio de motor derecho
  ledcSetup(canalPWMB, 5000, 8);
  ledcAttachPin(pinPWMB, canalPWMB);

  // activa el driver
  digitalWrite(pinSTBY, HIGH);

  // inicio del bluetooth con antena al maximo
  BLEDevice::init(NOMBRE_DISPOSITIVO);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

  pServerGlobal = BLEDevice::createServer();
  pServerGlobal->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServerGlobal->createService(SERVICE_UUID);

  // canal de envio (sin uso)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  // canal de recepcion de comandos
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  // empieza a anunciarse
  BLEAdvertising *pAdvertising = pServerGlobal->getAdvertising();
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();

  bleListo = (pServerGlobal != nullptr && pService != nullptr);

  // revision inicial
  revisionInicial();
}

// todo el trabajo lo hacen los eventos del bluetooth
void loop() {
  delay(1000);
}
