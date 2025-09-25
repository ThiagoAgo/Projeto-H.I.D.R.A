#include <Arduino.h>
#include "prototipos.h"
#include "config.h"
#include "externos.h"


// ===== Configuração do log =====
char log1[tamanho_log];
char log11[tamanho_log];
bool log_select = 0;
uint16_t cont_log = 0;

void log_put(char c) {
  if(log_select){
    if(cont_log < tamanho_log && c != '\r'){
      log1[cont_log] = c;
      cont_log++;
    }
  }
  else{
    if(cont_log < tamanho_log && c != '\r'){
      log11[cont_log] = c;
      cont_log++;
  }
  }
}

// Configura pinos na inicialização
void Configurar_pinos(){
  pinMode(Pino_PH           , INPUT);
  pinMode(Pino_Turbidez     , INPUT);
  pinMode(Pino_Condutividade, INPUT);
  pinMode(Pino_bateria      , INPUT);
  pinMode(Pino_conv_boost   , OUTPUT);
  pinMode(Pino_ON_OFF_3V3   , OUTPUT);
  pinMode(Pino_ON_OFF_5V    , OUTPUT);

}

// Verifica se a partição SPIFFS esta formatada corretamente
void Iniciar_SPIFFS(){
  if(SPIFFS.begin()){
    Serial.println("SPIFFS OK");
  }else{
    Serial.println("Erro no SPIFFS");
  }

  Serial.printf("Bytes totais: %u Usados: %u Livres: %u\n", SPIFFS.totalBytes(), SPIFFS.usedBytes(), SPIFFS.totalBytes()-SPIFFS.usedBytes());
}

void formatar_SPIFFS(){
  // Formata a partição SPIFFS
  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatado com sucesso!");
  } else {
    Serial.println("Falha ao formatar SPIFFS.");
  }

  // Agora é possível montar o sistema de arquivos limpo
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS montado!");
  } else {
    Serial.println("Falha ao montar SPIFFS.");
  }
}

// Carrega cosntantes salvas em arquivos json na partição SPIFFS
void Carregar_constantes_SPIFFS() {
  //==================== Leitura das configurações WiFi ====================

  File configFile = SPIFFS.open("/wifi.json", "r");
  if (!configFile) {
    Serial.println("Erro ao abrir o arquivo wifi.json");
    return;
  }

  // Aloca buffer para leitura do conteúdo JSON do arquivo
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);

  // Faz o parsing do JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.print("Falha ao fazer parsing do JSON de wifi: ");
    Serial.println(error.c_str());
    return;
  }

  // Lê os parâmetros de WiFi (SSID e senha)
  ssid              = doc["ssid"].as<String>();
  password          = doc["senha"].as<String>();
  potencia_wifi     = doc["potencia_tx"].as<uint8_t>();
  iniciar_com_wifi  = doc["wifi_sempre_ligado"].as<uint8_t>();
  APN               = doc["apn"].as<String>();
  FIREBASE_URL      = doc["url"].as<String>();

  //==================== Leitura das configurações de fatores de correção ====================

  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Erro ao abrir config.json");
    return;
  }

  JsonDocument doc_config;
  DeserializationError error_config = deserializeJson(doc_config, file);
  file.close();

  if (error_config) {
    Serial.print("Erro ao fazer parsing do JSON de config: ");
    Serial.println(error_config.c_str());
    return;
  }

  // Lê os fatores de correção (como float)
  Fator_correcao_bateria       = doc_config["fc_bateria"].as<float>();
  Fator_correcao_ph            = doc_config["fc_ph"].as<float>();
  Fator_correcao_turbidez      = doc_config["fc_turbidez"].as<float>();
  Fator_correcao_temperatura   = doc_config["fc_temperatura"].as<float>();
  Fator_correcao_condutividade = doc_config["fc_condutividade"].as<float>();
  Fator_correcao_profundicade  = doc_config["fc_profundicade"].as<float>();


    //==================== Leitura das configurações de fatores de correção ====================

  File file1 = SPIFFS.open("/outros.json", "r");
  if (!file1) {
    Serial.println("Erro ao abrir outros.json");
    return;
  }

  JsonDocument doc_outros;
  DeserializationError error_outros = deserializeJson(doc_outros, file1);
  file1.close();

  if (error_outros) {
    Serial.print("Erro ao fazer parsing do JSON de outros: ");
    Serial.println(error_outros.c_str());
    return;
  }

  // Lê os fatores de correção (como float) e a APN (como String)
  tempo_entre_leituras            = doc_outros["tempo_leitura"].as<uint16_t>();
  Limite_tentativas_sim           = doc_outros["tentativas_sim"].as<uint8_t>();
  Limite_tentativas_sinal_rede    = doc_outros["tentativas_sinal_rede"].as<uint8_t>();
  Limite_tentativas_registro_rede = doc_outros["tentativas_registro_rede"].as<uint8_t>();
  Limite_tentativas_GPS           = doc_outros["tentativas_GPS"].as<uint8_t>();
  Intevalo_entre_tentativas       = doc_outros["Intevalo_entre_tentativas"].as<int16_t>();
  dormir                          = doc_outros["dormir"].as<uint8_t>();

}


//=============================================================================================
bool ModificarParametroJSON(const char* caminho, const char* chave, String novoValor) {
  File file = SPIFFS.open(caminho, "r");
  if (!file) {
    Serial.printf("Erro ao abrir o arquivo %s para leitura\n", caminho);
    return false;
  }

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, file);
  file.close();

  if (erro) {
    Serial.printf("Erro ao fazer parsing do JSON (%s): %s\n", caminho, erro.c_str());
    return false;
  }

  // Atualiza o valor (como String)
  doc[chave] = novoValor;

  file = SPIFFS.open(caminho, "w");
  if (!file) {
    Serial.printf("Erro ao abrir o arquivo %s para escrita\n", caminho);
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Falha ao escrever JSON atualizado.");
    return false;
  }

  file.close();
  Serial.printf("Arquivo %s atualizado com sucesso. %s = %s\n", caminho, chave, novoValor.c_str());
  return true;
}

// Salva fatores de correção dos sensores após alteração via pagina WEB
void salvarFatores() {
  JsonDocument doc;
  doc["fc_bateria"] = Fator_correcao_bateria;
  doc["fc_ph"] = Fator_correcao_ph;
  doc["fc_turbidez"] = Fator_correcao_turbidez;
  doc["fc_temperatura"] = Fator_correcao_temperatura;
  doc["fc_condutividade"] = Fator_correcao_condutividade;
  doc["fc_profundicade"] = Fator_correcao_profundicade;

  File file = SPIFFS.open("/config.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
  }
}

// Função que monta e retorna o JSON como uma string
String criarJSON(const DadosObtidos& leitura, const DadosEstimados& estimado) {

  JsonDocument doc;

  // Adiciona os dados da leitura ao JSON
  doc["temperatura"] = leitura.Temperatura;
  doc["ph"] = leitura.PH;
  doc["turbidez"] = leitura.Turbidez;
  doc["condutividade"] = leitura.Condutividade;
  doc["nivel"] = leitura.Profundidade;
  doc["bateria"] = leitura.Bateria;
  doc["localizacao"] = leitura.Localizacao;
  doc["sinal"] = leitura.Sinal_4g;
  doc["umidade_interna"] = 0;

  // Adiciona os dados estimados ao JSON
  //doc["demanda_quimica_o2"] = estimado.demanda_quimica_o2;
  //doc["demanda_biologica_o2"] = estimado.demanda_biologica_o2;
  //doc["nitritos_e_nitratos"] = estimado.nitritos_e_nitratos;

  // Cria o campo "timestamp" com o formato especial do Firebase
  JsonObject timestampObj = doc["timestamp"].to<JsonObject>();
  timestampObj[".sv"] = "timestamp";


  // Serializa (converte) o JSON em uma String
  String json;
  serializeJson(doc, json);

  return json; // Retorna a string JSON pronta
}


void tratar_vetor_de_dados(bool erro_ao_enviar){
  
  if(erro_ao_enviar){
    // incrementa e restinge o contador para não contar alem do valor maximo
    Leituras_nao_enviadas++;
    if(Leituras_nao_enviadas > max_leituras_memoria){
      Leituras_nao_enviadas = max_leituras_memoria;
    }
    // Faz o deslocamento da última leitura para a próxima posição, indo de trás pra frente 
    // para evitar sobrescrever dados. Exemplo: quando "Leituras_nao_enviadas" e 2; 
    // Leitura[2] = Leitura[1] ... Leitura[1] = Leitura[0]
    for (int i = Leituras_nao_enviadas; i > 0; i--) {
      Leitura[i] = Leitura[i - 1];
    }

  }else{
    // decrementa o valor de leituras não enviadas caso o envio seja concluido com sucesso
    if(Leituras_nao_enviadas != 0){ 
      Leituras_nao_enviadas--;
    }
  }

}

