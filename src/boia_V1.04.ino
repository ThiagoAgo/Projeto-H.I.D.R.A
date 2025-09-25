/*
  -------------------------------------------------------
  Projeto: Monitoramento de qualidade da agua com ESP32
  Autores: Thiago Elias, Josue Luna
  Data de criação: 16/05/2025
  Última modificação: 27/07/2025
  Placa: ESP32-WROOM-32D
  IDE: PlatformIO PlatformIO 6.1.18

  Descrição:
  Este código realiza a leitura de um sensor de pH, tempera, 
  turbidez e condutividade da água exibe os dados via Serial 
  e pagina web, e permite comandos de configuração via porta 
  serial.

  Recursos utilizados:
  - ADC para leitura do sensor
  - Serial para comunicação com o usuário
  - Atualização OTA
  - Modos de hivernação
  - clock variavel
  - Partição SPIFFS

  Observações:
  - Para o wifi funcionar o  clock do processador deve ser >= 80MHz
  - mudar o clock do procesador abaixo de 80MHz muda a velocidade da comunicação serial
    para 40, 20 e 10 Mhz a velocidade cai respectivamente em 1/2, 1/4 e 1/8
  - O ACD2 NÃO pode ser usado juntamnte com Wifi

  Comandos usando porta serial
  - Para ativar atualizações por wifi escreva "atualizar" na Serial
  - Para mudar o clock do processador manualmente escreva "clock xx"
    n0 serial onde xx pode ser 10, 20, 40, 80, 160 e 240 (MHz)
  - Para saber o clock atual do processador escreva na serial "clock?"

  Atualizações
  V1.01  18/05/2025  Fatores de converção e acesso a EEPROM adicionados
  V1.02  23/05/2025  O ID da boia agora é o ChipID do ESP32, Funções de hibernação agora recebem argumentos de horas, minutos e segundos
  V1.03  04/06/2025  EEPROM substituida por partição SPIFFS, numeros de versão remodelados, paginas de configuração via wifi
  V1.04  05/06/2025  Paginas web modificadas e movidas para partição SPIFFS
  V1.041 09/06/2025  Pagina para envio de Arquivos para memoria interna, Correção de bugs, mudança de pinos 
  V1.05  30/06/2025  Adicionado sensor de pressão (Profundidade) 
  V1.06  01/07/2025  Adicionada comunicação com modulo 4G 
  V1.07  03/07/2025  comunicação 4G funcional
  V1.08  12/07/2025  Corrigido erro caso arquivo com configuração wifi esteja ausente; adicionado nivel do sinal GSM; 
                     Adicionadas data e hora da rede GSM; Agora a APN e o servidor podem ser alterados por arquivo json; 
                     Adicioandas verificações durante a inicialização do modulo GSM; Chamadas da biblioteca ArduinoJson foram atualizadas 
                     (removidos avisos durante a compilação)
  V1.09  18/07/2025  Adicionados valores estimados, Removidos valores aleatorios de sensores para testes
  V1.10  18/07/2025  (Projeto migrou para PCB hidra com ESP32-S3) Pinos alterados; arquivo platformio.ini foi modificado;
  V1.11  26/07/2025  Leituras agora são armazenadas na memoria RTC; Tempo entre leituras pode ser configurado via arquivo Json; 
                     Criada função para controle de energia ligando e desligando modulos chamada "energia" 
  V1.12  27/07/2025  Criada função para geração de arquivo Json; Função estimar_parametros agora recebe um ponteiro para o endereço do dado;
                     Funçao "enviar_dados" agora retorna o valor 1 se houver erro no envio; Criada fila para envio de dados;
  V1.13  31/07/2025  Potencia de transmição do wifi agora pode ser definida no arquivo "wifi.json"; O wifi agora pode iniciar ligado se 
                     configurado no arquivo "wifi.json"
  V1.14  05/08/2025  Agora e possivel reiniciar o  sistema remotamente via wifi acessando: <ip>/reiniciar; Corrigidos pinos do modulo 4G LTE
                     adicionada pagina para configurar redes sem fio;
  V1.15  07/08/2025  Adicionada estimativa do percentual de bateria; Pino de leitura da bateria foi modificado; Pagina "Fatores de correção modificada";
  V1.16  12/08/2025  Adicionada função de filtro de media; Sensores agora são filtrados; Adicionado no arquivo de partição "spiffs_8MB.csv"
                     Arquivo de partição alterado em "platformio.ini" (partição spiffs aumentada de 0.1875MB -> 4.1250MB); 
  v1.17  12/08/2025  Adicionado no arquivo de partição "spiffs_16MB.csv"; Arquivo de partição alterado em "platformio.ini" 
                     (partição spiffs aumentada de 4.125MB -> 12.125MB);
  v1.18  16/08/2025  Controle do modulo 4G LTE transferido para o nucleo 1; Inicializado do modulo 4G LTE otimizada; Adicionados mais codigos de erro para
                     o modulo 4G LTE; Adicionada pagina com historico de comunicação com modulo LTE; Limite de tenstiva de conexão podem ser configuradas 
                     por arquivo jsom
  v1.19  21/08/2025  Corigido erro na leitura dos sensores de temperatura e presão apos o religamento da energia
  V1.20  25/08/2025  Localização do GPS agora e enviada corretamente ao servidor; Força do sinal 4G agora e enviada ao servidor;
  V1.21  29/08/2025  Dados qua não foram enviados por qualquer erro agora são enviados na proxima tentativa; Pequenas correções;
  V1.22  09/09/2025  Equação para turbidez modificada; Correção no envio de localizaçao ao servidor
  
  Melhorias futuras 
  Watchdog
  https://github.com/phfbertoleti/ESP32_FUOTA_example/blob/main/Software/fota_ESP32.ino
  -------------------------------------------------------
*/
#include <Arduino.h>
#include "prototipos.h"
#include "config.h"

//==================== Variaveis globais============================
const uint32_t chipID = (uint32_t)(ESP.getEfuseMac() >> 32); //um ID exclusivo do Chip...
uint8_t OTA_Wifi = 0;                     // sinaliza se o wifi fica ligado para receber um  atualização
uint8_t Codigo_erro = 0;                  //Usado para sinalizar erros no modulo 4G 
//DadosObtidos Leitura;                   // Cria um variavel do tipo "DadosObtidos"
float   Fator_correcao_bateria;
float   Fator_correcao_ph;
float   Fator_correcao_turbidez;
float   Fator_correcao_temperatura;
float   Fator_correcao_condutividade;
float   Fator_correcao_profundicade;
long    tempo_anterior = 0;
uint8_t hora;
uint8_t minuto;
uint8_t segundo;
double latitude;
double longitude;
double v;

String FIREBASE_URL;                          // No arquivo wifi.json
String APN;                                   // No arquivo wifi.json
uint8_t potencia_wifi = 40;                   // No arquivo wifi.json (40/4) = 10dBm | (84 = 21 dBm = Max)
uint8_t iniciar_com_wifi = 0;                 // No arquivo wifi.json
uint16_t tempo_entre_leituras = 30;           // No arquivo outros.json (em segundos)
uint8_t Limite_tentativas_sim = 5;            // No arquivo outros.json
uint8_t Limite_tentativas_sinal_rede = 20;    // No arquivo outros.json
uint8_t Limite_tentativas_registro_rede = 10; // No arquivo outros.json
uint8_t Limite_tentativas_GPS = 3;            // No arquivo outros.json
uint16_t Intevalo_entre_tentativas = 5000;    // No arquivo outros.json (em ms)
uint8_t dormir = 0;                           // No arquivo outros.json

//==================================================================


TaskHandle_t Tarefa_LTE_Sensores;
void Loop2(void);


void setup() {
   // Configura Pinos de entrada e saida
  Configurar_pinos();
  // Liga energia do modulo 4G LTE
  energia(lte, HIGH);
  // Liga energia dos sensores
  energia(sensores, HIGH); 
  // Inicia comunicação serial com computador
  Serial.begin(115200);               //961600, 460800 , 230400, 115200
  // Inicia comunicação serial com modulo 4G
  sim.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  // Inicia o sensor DS18B20
  sensors.begin();
  // Inicia o sensor de Pressão
  Sensor_pressao.initialize(SCK_Pressao , DOUT_Pressao);
  // Inicia particao SPIFFS (pode demorar dependendo do clock do processador e tamanho da partição)
  Iniciar_SPIFFS();
  // Carrega valores da SPIFFS na memoria RAM (pode demorar dependendo do clock do processador e tamanho da partição)
  Carregar_constantes_SPIFFS();  
  // CPU opera a 10MHz para economizar energia 
  mudar_Clock(80);     
  // Inicia com wifi ligado (usado para teste, comfiguração no arquivo outros.json)
  if(iniciar_com_wifi) iniciarWifi_OTA();
  // Pequeno delay
  delay(100);
  // Escreve o nome do hidra na serial (com estilo)
  texto_hidra_serial();
  
  // Cria a task de sensores no CORE 1
  xTaskCreatePinnedToCore(
    Loop2,                  // Função da tarefa
    "Loop2",                // Nome
    20000,                   // Stack
    NULL,                   // Parâmetro
    1,                      // Prioridade
    &Tarefa_LTE_Sensores,   // Handle
    1                       // Núcleo (0 = Wi-Fi, comandos serial, 1 = livre)
  );
  
 
}



void loop() {
  // Verifica se a clientes wifi para atualizar o sistema via OTA
  if (OTA_Wifi) server.handleClient();
  // Verifica os comandos recebidos da porta serial
  if (Serial.available()) acao_serial();

  delay(10);
}


// ------------------- TAREFA DE LEITURA E ENVIO -------------------
void Loop2(void *pvParameters) {
  for (;;) {
    
    // Liga energia do modulo 4G LTE
    energia(lte, HIGH);
    // Liga energia dos sensores
    energia(sensores, HIGH);
    
    //Tempo para inicialização dos sensores e do modulo LTE
    delay(5000);

    //garante que a primeira leitura do sensores digitais seja correta
    if(dormir == 0){
      // Inicia o sensor DS18B20
      sensors.begin();
      // Inicia o sensor de Pressão
      Sensor_pressao.initialize(SCK_Pressao , DOUT_Pressao);

      delay(500);
      sensors.requestTemperatures();       // Solicita leitura do sensor de temperatura
      Sensor_pressao.readAndSelectNextData( HX710_DIFFERENTIAL_INPUT_40HZ ); // Solicita leitura do sensor de pressão (profundidade)
      delay(500);
      sensors.getTempCByIndex(0);
      Sensor_pressao.getLastDifferentialInput();
      delay(500);
      sensors.requestTemperatures();       // Solicita leitura do sensor de temperatura
      Sensor_pressao.readAndSelectNextData( HX710_DIFFERENTIAL_INPUT_40HZ ); // Solicita leitura do sensor de pressão (profundidade)
      delay(500);
      sensors.getTempCByIndex(0);
      Sensor_pressao.getLastDifferentialInput();
      delay(500);
      sensors.requestTemperatures();       // Solicita leitura do sensor de temperatura
      Sensor_pressao.readAndSelectNextData( HX710_DIFFERENTIAL_INPUT_40HZ );// Solicita leitura do sensor de pressão (profundidade)
      delay(500);
      sensors.getTempCByIndex(0);
      Sensor_pressao.getLastDifferentialInput();
      delay(1950);
    }
    else{
      delay(5000);
    }

    // Lê todos os sensores e armazena na variavel Leituta
    leitura_de_sensores(&Leitura[0]);        
    // Desliga a energia do sensores após a leitura 
    energia(sensores, LOW); 
    
    // :-)
    inicializaModem();
    // Estima parametros de qualidade com base na leitura dos sensores do mais antigo ao mais recente
    estimar_parametros(&Leitura[Leituras_nao_enviadas]);
    // Criação do JSON com os dados de leitura e valores estimados do mais antigo ao mais recente
    String json = criarJSON(Leitura[Leituras_nao_enviadas], Estimado);
    // Envia o JSON ao servidor, ou armazena a leitura no vetor de dados em caso de erro no envio.
    tratar_vetor_de_dados(enviar_dados(json));  
    
    // Tenta enviar os outros dados da fila caso exista
    while ((Leituras_nao_enviadas > 0) && (Codigo_erro == 0)){
      Serial.printf("Leituras na fila: %i\n", Leituras_nao_enviadas);
      estimar_parametros(&Leitura[Leituras_nao_enviadas]);
      String json = criarJSON(Leitura[Leituras_nao_enviadas], Estimado);
      tratar_vetor_de_dados(enviar_dados(json));
    }
    
    // Desliga energia do modulo 4G LTE
    energia(lte, LOW);
    // Mostra quantidade de leituras não enviadas
    Serial.printf("Leituras na fila: %i\n", Leituras_nao_enviadas);

    // decide se o sistema vai ou não hibernar dependo do arquivo de configuração
    if(dormir){
      LightSleep(tempo_entre_leituras, 's');
    } else{
      Serial.printf("Modo de Espera %i s\n", tempo_entre_leituras);
      vTaskDelay(1000*tempo_entre_leituras / portTICK_PERIOD_MS); 
    }

    //LightSleep(1, 's'); // LightSleep por 1 segundo
    //DeepSleep (2, 'm'); // DeepSleep  por 2 minutos
    //Hibernacao(3, 'h'); // Hibernacao por 3 horas

  }
}


