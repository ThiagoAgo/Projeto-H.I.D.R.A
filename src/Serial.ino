#include <Arduino.h>
#include "prototipos.h"
#include "config.h"
#include "externos.h"


void acao_serial(){
  String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("atualizar")) {
      iniciarWifi_OTA();
    } 
    else if(input.startsWith("clock ")) {
      int c = input.substring(6).toFloat();
      mudar_Clock(c);
    }
    else if(input.startsWith("clock")) {
      mostrar_Clock();
    }
    else if(input.startsWith("reiniciar")) {
      ESP.restart();
    }
    else if(input.startsWith("wifi")) {
      IPAddress IP = WiFi.softAPIP();
      Serial.print("IP: ");
      Serial.println(IP);
    }
    else if(input.startsWith("formatar")) {
      formatar_SPIFFS();
    }
    else if(input.startsWith("versao")) {
      texto_hidra_serial();
    }
    else if(input.startsWith("corrigir")) {
      
      if (input.indexOf("bat") != -1) {
        int pos = input.indexOf("bat");
        String valorStr = input.substring(pos + 3);
        valorStr.trim();
        ModificarParametroJSON("/config.json", "fc_bateria", valorStr);
      }
      else if (input.indexOf("ph") != -1) {
        int pos = input.indexOf("ph");
        String valorStr = input.substring(pos + 2);
        valorStr.trim();
        ModificarParametroJSON("/config.json", "fc_ph", valorStr);
      }
      else if (input.indexOf("turb") != -1) {
        int pos = input.indexOf("turb");
        String valorStr = input.substring(pos + 4);
        valorStr.trim();
        ModificarParametroJSON("/config.json", "fc_turbidez", valorStr);
      }
      else if (input.indexOf("condu") != -1) {
        int pos = input.indexOf("condu");
        String valorStr = input.substring(pos + 5);
        valorStr.trim();
        ModificarParametroJSON("/config.json", "fc_condutividade", valorStr);
      }
      else if (input.indexOf("temp") != -1) {
        int pos = input.indexOf("temp");
        String valorStr = input.substring(pos + 4);
        valorStr.trim();
        ModificarParametroJSON("/config.json", "fc_temperatura", valorStr);
      }
      else if (input.indexOf("prof") != -1) {
        int pos = input.indexOf("prof");
        String valorStr = input.substring(pos + 4);
        valorStr.trim();
        ModificarParametroJSON("/config.json", "fc_profundicade", valorStr);
      }
      else{
        Serial.println("O comando deve ter o formato ''corrigir <variavel> <valor>''");
        Serial.println("Variaveis disponivies: bat, ph, turb, condu, temp e prof");
      }


    }    
    else if(input.startsWith("sensores")) {
      Serial.println("              Valor corrigido    Valor lido    Fator de correção");
      Serial.printf("ID:             %d             -                -\nPH:             %.3f              %.3f         %.3f   \nTurbidez:       %.3f              %.3f         %.3f   \nTemperatura:    %.3f             %.3f         %.3f   \nCondutividade:  %.3f              %.3f         %.3f   \nLocalizacao:    %s\nBateria:        %.3f               %.3f         %.3f   \nProfundidade:    %.3f              %.3f         %.3f   \n",
                    chipID, Leitura[0].PH, Leitura[0].PH/Fator_correcao_ph, Fator_correcao_ph, Leitura[0].Turbidez, Leitura[0].Turbidez/Fator_correcao_turbidez, Fator_correcao_turbidez, Leitura[0].Temperatura, Leitura[0].Temperatura/Fator_correcao_temperatura, Fator_correcao_temperatura, Leitura[0].Condutividade, Leitura[0].Condutividade/Fator_correcao_condutividade, Fator_correcao_condutividade, Leitura[0].Localizacao.c_str(), Leitura[0].Bateria, Leitura[0].Bateria/Fator_correcao_bateria, Fator_correcao_bateria, Leitura[0].Profundidade, Leitura[0].Profundidade/Fator_correcao_profundicade, Fator_correcao_profundicade);
      Serial.println(Estimado.bateria);
                  }
    else if(input.startsWith("AT")){
      sendAT(input, 5000, false); 
    }
    
    else {
      Serial.println(F("Comando inválido. Comandos disponíveis:"));
      Serial.println(F(" atualizar     - Inicia o modo OTA para atualização via Wi-Fi"));
      Serial.println(F(" clock <valor> - Altera a frequência da CPU (ex: clock 80)"));
      Serial.println(F(" clock         - Mostra a frequência atual da CPU"));
      Serial.println(F(" reiniciar     - Reinicia o ESP32"));
      Serial.println(F(" wifi          - Mostra o IP da rede criada pelo ESP"));
      Serial.println(F(" formatar      - Formata partição SPIFFS"));
      Serial.println(F(" sensores      - Exibe os dados lidos pelos sensores"));
      Serial.println(F(" corrigir      - Menu de correção para sensores"));
      Serial.println(F(" AT <comanddo> - Envia comando AT ao modulo 4G LTE"));
      Serial.println(F(" versao        - Exibe a versão do software"));

    }
}

void texto_hidra_serial(){
  Serial.println("");
  Serial.println("ooooo   ooooo ooooo oooooooooo.   ooooooooo.         .o.");
  Serial.println("`888'   `888' `888' `888'   `Y8b  `888   `Y88.      .888.");
  Serial.println(" 888     888   888   888      888  888   .d88'     .8'888.");
  Serial.println(" 888ooooo888   888   888      888  888ooo88P'     .8' `888.");
  Serial.println(" 888     888   888   888      888  888`88b.      .88ooo8888.");
  Serial.println(" 888     888   888   888     d88'  888  `88b.   .8'     `888.");
  Serial.println("o888o   o888o o888o o888bood8P'   o888o  o888o o88o     o8888o");
  Serial.printf("\nVersão: %.2f \n\n", versao_software);
}