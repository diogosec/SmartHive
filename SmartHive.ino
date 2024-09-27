#include "arduino_secrets.h"
#include <WiFi.h>
#include <HX711.h>
#include <Callmebot_ESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ToneESP32.h>
#include <OneButton.h>
#include "thingProperties.h"

// Definições de pinos
#define PIN_DT 3
#define PIN_SCK 2
#define ONE_WIRE_BUS 10
#define BUZZER_PIN 4
#define BUTTON_PIN 5  

// Inicializar os objetos
HX711 scale;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
OneButton button;

String phoneNumber = "+558393179199";
String apiKey = "2516991"; 
float weightValue;
bool isInitialSetupSet = false;  // Flag para saber se o configuração inicial foi realizada
float previousWeight = 0;  // Armazena o peso anterior
unsigned long lastCheckTime = 0;  // Armazena o tempo da última verificação da diferença de peso
unsigned long lastScaleOperationTime = 0; // Armazena o tempo da última operação (ligar/desligar) do sensor de peso
unsigned long lastTempOperationTime = 0; // Armazena o tempo da última verificação de temperatura

//Declaração de Funções
void initialSetup();
void realTimeWeight(unsigned long seconds);
void checkWeightDifference(float weightGrams, float minutes);
void realTimeTemperature(unsigned long seconds);
void beep(int beepNumbers, int beepDuration, int beepPause);


void setup() {
  Serial.begin(9600);
  delay(1500); 

  //Arduino Iot Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  sensor.begin(); //Sensor de temperatura
  
  //Sensor de peso
  scale.begin(PIN_DT, PIN_SCK);
  scale.set_scale(29240); //Valor de calibração
  delay(2000);
  scale.tare(); //Zerar balança
  
  // Setup OneButton
  button.setup(BUTTON_PIN, INPUT_PULLUP, true);
  button.attachDoubleClick(initialSetup); //doubleclick

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // Wi-Fi
  WiFi.begin(SSID, PASS);
  Serial.print("Conectando ao Wi-Fi ");
  // Aguarda a conexão com o Wi-Fi 
  while (WiFi.status() != WL_CONNECTED ) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print("Conectado à rede Wi-Fi: ");
  Serial.println(SSID);
  beep(2, 150, 150);

  // Instruções para o usuário
  Serial.println("Por favor, coloque a caixa de abelha na balança.");
  Serial.println("Após colocar a caixa, pressione o 'Botão de Configuração'.");
}


void loop() {
  ArduinoCloud.update();

  if (!isInitialSetupSet) {
    button.tick();    
  } else {
      //Serial.println("Estou no else");
      realTimeWeight(2); // Faz a leitura do peso a cada 'n' segundos
      checkWeightDifference(500, 0.5);  // Verifica se a diferença de peso excede 'x' gramas após 'y' minutos
      realTimeTemperature(5); // Faz a leitura da temperatura a cada 'n' segundos
  }
}

void initialSetup(){
  previousWeight = scale.get_units(10);  // Captura o peso inicial da caixa de abelha
  lastCheckTime = millis();
  lastScaleOperationTime = millis();
  lastTempOperationTime = millis();
  isInitialSetupSet = true;
  beep(1, 500, 0);
  
  Serial.print("Peso da caixa capturado: ");
  Serial.println(previousWeight, 3);
  
}

void realTimeWeight(unsigned long seconds){

  unsigned long checkInterval = seconds * 1000;
  
  // Verifica se já passou o intervalo desde a última verificação
  if (millis() - lastScaleOperationTime >= checkInterval) {
    
    weightValue = scale.get_units(10);
    weight = (weightValue < 0) ? 0 : weightValue;
  
    Serial.print("Peso em tempo real: ");
    Serial.println(weight, 3);
  
    // Atualiza o tempo da última verificação
    lastScaleOperationTime = millis();
  }
}

void checkWeightDifference(float weightGrams, float minutes) {
  // Converte o limite de diferença de peso de gramas para quilogramas
  float weightKg = weightGrams / 1000.0;
  
  // Converte minutos para milissegundos
  unsigned long checkInterval = minutes * 60000;

  unsigned long lastCheckTimea = millis();

  // Verifica se já passou o intervalo desde a última verificação
  if (millis() - lastCheckTime >= checkInterval) {
    // Calcula a diferença entre o peso atual e o anterior
    float weightDifference = weightValue - previousWeight;
  
    // Verifica se o peso aumentou além do limite
    if (weightDifference > weightKg) {
        String message = "Alerta: O peso aumentou " + String(weightDifference, 3) + "kg em " + String(minutes) + " minutos";
        Serial.println(message);
        
        Callmebot.whatsappMessage(phoneNumber, apiKey, message);
    }
    // Verifica se o peso diminuiu além do limite
    else if (weightDifference < -weightKg) {
        String message = "Alerta: O peso diminuiu " + String(weightDifference, 3) + "kg em " + String(minutes) + " minutos";
        Serial.println(message);
        
        Callmebot.whatsappMessage(phoneNumber, apiKey, message);
    }
  
    // Atualiza o peso anterior e o tempo da última verificação
    previousWeight = weightValue;
    lastCheckTime = millis();
  }
}

void realTimeTemperature(unsigned long seconds){
  // Converte minutos para milissegundos
  unsigned long checkInterval = seconds * 1000;
  
  // Verifica se já passou o intervalo desde a última verificação
  if (millis() - lastTempOperationTime >= checkInterval) {
    sensor.requestTemperatures();
    float tempC=sensor.getTempCByIndex(0); // Obtém a temperatura em Celsius
    
    temperature = tempC;
    lastTempOperationTime = millis();
  
    Serial.print("Temperatura: ");
    Serial.print(tempC);
    Serial.println(" °C");
  }
}

void beep(int beepNumbers, int beepDuration, int beepPause) {
  for (int i = 0; i < beepNumbers; i++) {
    tone(BUZZER_PIN, 1000); // Gera um som de 'n' Hz no buzzer
    delay(beepDuration);             
    noTone(BUZZER_PIN);     
    delay(beepPause);             
  }
}



