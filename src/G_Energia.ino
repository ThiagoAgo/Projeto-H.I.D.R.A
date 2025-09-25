#include <Arduino.h>
#include "prototipos.h"
#include "config.h"
#include "externos.h"

void ModemSleep() {
  WiFi.disconnect(true);  // desconecta da rede wifi (se conectado)
  WiFi.mode(WIFI_OFF);    // desliga o (moldem wifi)
  btStop();               // desliga Bluetooth se estiver usando
  #ifdef tagarela
    Serial.println("Entrou em Modem Sleep (Wi-Fi OFF, CPU ativa)");
  #endif
}

void LightSleep(uint64_t tempo, char escala_tempo) {  
  uint64_t tempo_us = tempo * tempo_para_microssegundos(escala_tempo);
  #ifdef tagarela
    Serial.println("Entrando em Light Sleep...");
  #endif
  esp_sleep_enable_timer_wakeup(tempo_us);  // tempo em microssegundos
  esp_light_sleep_start();
  #ifdef tagarela
    Serial.println("Acordou do Light Sleep");
  #endif
}

// para o ESP32-C3 isso e mesmo que hibernar e os dados não ficam salvos na Ram
void DeepSleep(uint64_t tempo, char escala_tempo) { 
  uint64_t tempo_us = tempo * tempo_para_microssegundos(escala_tempo);
  #ifdef tagarela
    Serial.println("Entrando em Deep Sleep...");
  #endif
  esp_sleep_enable_timer_wakeup(tempo_us);  // tempo em microssegundos
  esp_deep_sleep_start();
  #ifdef tagarela
    Serial.println("Acordou do Deep Sleep");
  #endif
}

void Hibernacao(uint64_t tempo, char escala_tempo) {
  uint64_t tempo_us = tempo * tempo_para_microssegundos(escala_tempo);
  #ifdef tagarela
    Serial.println("Entrando em hibernação...");
  #endif
  esp_sleep_enable_timer_wakeup(tempo_us);  // tempo em microssegundos
  esp_deep_sleep_start();  // Isso efetivamente coloca o ESP em hibernação
}

void mudar_Clock(int clock){
   //Garente que o CPU opera na frequencia minima para wifi funcionar
  if(OTA_Wifi == 1 && clock < 80){
    setCpuFrequencyMhz(80);
    Serial.println("Esse valor não e permitido quando Wifi esta ligado");
    Serial.println("clock da cpu foi mudado para 80MHz");
  }else{
    setCpuFrequencyMhz(clock); // CPU opera na frequencia recebida
    #ifdef tagarela
      Serial.printf("clock da cpu foi mudado para %dMHz\n", clock);
    #endif
  }
}

void mostrar_Clock(){
  Serial.print("Frequência da CPU: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
}

uint64_t tempo_para_microssegundos(char escala_tempo){
  uint64_t multiplicador;
  switch (escala_tempo){
    case 104: // h na tabela ASCII 
      multiplicador = 3.6E+9;
      break;
    case 109: // m na tabela ASCII 
      multiplicador = 6E+7;
      break;
    case 115: // s na tabela ASCII 
      multiplicador = 1E+6;
      break;
    default:
      multiplicador = 1;
      break;
  }
  return multiplicador;
}

void energia(bool tipo, bool estado){
  if(tipo == sensores){
    digitalWrite(Pino_ON_OFF_5V , estado); // liga/desliga sensores que funcionam com 5V
    digitalWrite(Pino_ON_OFF_3V3, estado); // liga/desliga sensores que funcionam com 3.3V
  }else if(tipo == lte){
    digitalWrite(Pino_conv_boost, estado); // liga/desliga conversor boost (O modulo 4G e sensores de 5V estão ligados a ele)
  }
}
