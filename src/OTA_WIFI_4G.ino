#include <Arduino.h>
#include "prototipos.h"
#include "config.h"
#include "externos.h"


void handleEnviarArquivos() {
  String html = R"rawliteral(
    <!DOCTYPE html><html><head><title>Upload SPIFFS</title><meta charset="UTF-8" name="viewport" content="width=device-width,initial-scale=1"><style>body{font-family:Arial;text-align:center;margin-top:10vh;background:#f2f2f2}form{background:#fff;padding:20px;border-radius:8px;display:inline-block;box-shadow:0 2px 6px #aaa;width:90%%}input[type=submit]{background:#007bff;color:#fff;border:none;padding:10px 20px;border-radius:4px}input[type=submit]:hover{background:#0056b3}.file-list{margin-top:30px;color:#555;font-size:14px}hr{margin:40px auto;width:80%;border:0;border-top:1px solid #ccc}</style></head><body><form method="POST" action="/uploadData" enctype="multipart/form-data"><h2>Upload de Arquivos SPIFFS</h2><input type="file" name="upload" required><br><br><input type="submit" value="Enviar"></form><hr><div class="file-list"><h3>Arquivos:</h3><ul></ul></div></body></html>
  )rawliteral";

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    html += "<li>" + String(file.name()) + " (" + file.size() + " bytes)</li>";
    file = root.openNextFile();
  }

  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}
File uploadFile;

void handleUploadData() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    Serial.print("Recebendo arquivo: ");
    Serial.println(filename);
    uploadFile = SPIFFS.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile)
      uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile)
      uploadFile.close();
    Serial.println("Upload completo");
  }
}

void handleLogAtual() {
  String html = "<html><head><meta charset='UTF-8'><title>Log Serial</title></head><body>";
  html += "<h2>Serial</h2><pre>";
  if (log_select){
    for (uint16_t i = 0; i < tamanho_log; i++) {
      if (log1[i] == '\n'){
        html += "<BR>";
      } 
      else{
        html += log1[i];
      }
    }
  }
  else{
    for (uint16_t i = 0; i < tamanho_log; i++) {
      if (log11[i] == '\n'){
        html += "<BR>";
      } 
      else{
        html += log11[i];
      }
    }
  }
  html += "</pre><h2>Fim</h2></body></html>";
  server.send(200, "text/html", html);
}

void handleLogAnterior() {
  String html = "<html><head><meta charset='UTF-8'><title>Log Serial</title></head><body>";
  html += "<h2>Histórico Serial</h2><pre>";
  if (!log_select){
    for (uint16_t i = 0; i < tamanho_log; i++) {
      if (log1[i] == '\n'){
        html += "<BR>";
      } 
      else{
        html += log1[i];
      }
    }
  }
  else{
    for (uint16_t i = 0; i < tamanho_log; i++) {
      if (log11[i] == '\n'){
        html += "<BR>";
      } 
      else{
        html += log11[i];
      }
    }
  }
  html += "</pre><h2>Fim</h2></body></html>";
  server.send(200, "text/html", html);
}


void handleID() {
  JsonDocument doc;
  doc["id"]       = String(chipID);
  doc["versao"]   = String(versao_software);
  doc["Cod_Erro"] = Codigo_erro;
  doc["tensao_t"] = v;

  String resposta;
  serializeJson(doc, resposta);
  server.send(200, "application/json", resposta);
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Recebendo: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Atualização concluída: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleLeituras() {
  JsonDocument doc;
  doc["leitura_bateria"]       = Leitura[0].Bateria; 
  doc["leitura_ph"]            = Leitura[0].PH;
  doc["leitura_turbidez"]      = Leitura[0].Turbidez;
  doc["leitura_temperatura"]   = Leitura[0].Temperatura;
  doc["leitura_condutividade"] = Leitura[0].Condutividade;
  doc["leitura_profundicade"]  = Leitura[0].Profundidade;

  String resposta;
  serializeJson(doc, resposta);
  server.send(200, "application/json", resposta);
}

void handleReiniciar() {
  server.send(200, "text/plain", "Reiniciando H.I.D.R.A...");
  delay(1000);  
  ESP.restart();
}

void iniciarWifi_OTA(){
  OTA_Wifi = 1;
  mudar_Clock(80);
  delay(100);
  WiFi.mode(WIFI_AP);
  if(ssid == "" ){
    WiFi.softAP("wifi", "12345678", 6, false);
    #ifdef tagarela
      Serial.print("Rede criada: ");
      Serial.println("wifi");
    #endif
  }else{
    WiFi.softAP(ssid, password, 6, false);
    #ifdef tagarela
      Serial.print("Rede criada: ");
      Serial.println(ssid);
    #endif
  }
  // Define largura de banda (WIFI_BW_HT20 = 20 MHz, WIFI_BW_HT40 = 40 MHz)
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

  // limita a potencia de transmição do wifi 
  // (Ao usar a  potencia maxima ao menos no usb o sistema reinica por falta de energia)
  esp_wifi_set_max_tx_power(potencia_wifi);  // potencia de transmição = ( <valor> / 4 ) dBm
  
  IPAddress IP = WiFi.softAPIP();
  
  #ifdef tagarela
    Serial.print("IP: ");
    Serial.println(IP);
  #endif
  
  // Rota para exibir pagina inicial
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/ota.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/log_atual", handleLogAtual);
  server.on("/log_anterior", handleLogAnterior);
  
  // Rota para atualização
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", Update.hasError() ? "Falha" : "Sucesso. Reiniciando...");
    delay(1000);
    ESP.restart();
  }, handleUpdateUpload);

  // Rota para exibir conteúdo de /config.json
  server.on("/config", HTTP_GET, []() {
    File file = SPIFFS.open("/config.json", "r");
    server.streamFile(file, "application/json");
    file.close();
  });

  // Rota para exibir conteúdo de /wifi.json
  server.on("/joutros", HTTP_GET, []() {
    File file = SPIFFS.open("/outros.json", "r");
    server.streamFile(file, "application/json");
    file.close();
  });

  // Rota para exibir conteúdo de /wifi.json
  server.on("/jwifi", HTTP_GET, []() {
    File file = SPIFFS.open("/wifi.json", "r");
    server.streamFile(file, "application/json");
    file.close();
  });

  // Rota para exibir pagina de configuração wifi
  server.on("/wifi", HTTP_GET, []() {
    File file = SPIFFS.open("/wifi.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  // Rota para criar um arquivo json com as ultimas leituras
  server.on("/leituras", handleLeituras);

  // Exibe pagina com sensores e fatores de correção
  server.on("/sensores", HTTP_GET, []() {
    File file = SPIFFS.open("/sensores.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

  // Rota para criar um arquivo json com id e versão
  server.on("/id", handleID);

  // Pagina de apoio para /sensores
  server.on("/atualizar", HTTP_POST, []() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));

    if (!err) {
      Fator_correcao_bateria       = doc["fc_bateria"] | 1.00;
      Fator_correcao_ph            = doc["fc_ph"] | 1.00;
      Fator_correcao_turbidez      = doc["fc_turbidez"] | 1.00;
      Fator_correcao_temperatura   = doc["fc_temperatura"] | 1.00;
      Fator_correcao_condutividade = doc["fc_condutividade"] | 1.00;
      Fator_correcao_profundicade  = doc["fc_profundicade"] | 1.00;

      salvarFatores();
      server.send(200, "text/plain", "Fatores atualizados.");
      return;
    }
  }
  server.send(400, "text/plain", "Erro ao atualizar.");
  });

  //Reinicialização remota
  server.on("/reiniciar", handleReiniciar);

  // Roteamento
  server.on("/enviarArquios", HTTP_GET, handleEnviarArquivos);
  server.on(
    "/uploadData", HTTP_POST,
    []() { server.sendHeader("Location", "/"); server.send(303); },
    handleUploadData
  );

  server.begin();
  #ifdef tagarela
    Serial.println("Servidor Wifi OTA iniciado");
  #endif
}
//===========================================================================================================
//============================================= Comunicação 4G ==============================================
//===========================================================================================================
void inicializaModem() {
  int contador_sinal_rede = 0;
  int Contador_sim = 0;
  int contador_registro_rede = 0;
  int contador_gps = 0;
 
  Serial.println("Inicializando A7670SA...");
  Codigo_erro = 0; // limpa o codigo de erro
  log_select = !log_select; // alterna entre os arquivos de log
  cont_log = 0; // zera o contador de caracteres do log

  // Inicialização rápida do modem
  // ATI AT+CIMI
  //AT+CNMP?  AT+Creset
  // AT+CGNSSPWR=1   AT+CGNSSPWR?   AT+CGNSSINFO
  
  // Testa a comunicação serial com o modem (Sempre deve retonar OK)
  sendAT("AT", 1000, false);

  //#1 Verifica se o SIM está pronto (Respostas possiveis: +CPIN: READY,  SIM PIN, SIM PUK, NOT INSERTED, PH-SIM PIN)
  sendAT("AT+CPIN?", 1000, false);
  // Procura o chip novamente (Erro = 1), ou espera o modulo responder (Erro = 10)
  while ((Codigo_erro == 1 || Codigo_erro == 10) && Contador_sim < Limite_tentativas_sim){  
    Codigo_erro = 0;
    Serial.println("Procurando Chip ...");
    delay(Intevalo_entre_tentativas);
    sendAT("AT+CPIN?", 1000, false); 
    Contador_sim++;
  }//-------------------------------------------------------------------------------------


  if(Codigo_erro == 0){ // Passa para ressa parte somente se um chip for encontrado
    
     //#2 mostra sinal da rede
    sendAT("AT+CSQ", 1000, false);
    // Procura a rede novamente
    while ((Codigo_erro == 2) && (contador_sinal_rede < Limite_tentativas_sinal_rede)){ 
      Codigo_erro = 0;
      Serial.println("Procurando Sinal ...");
      delay(Intevalo_entre_tentativas);
      sendAT("AT+CSQ", 1000, false);
      contador_sinal_rede++; 
    }//-------------------------------------------------------------------------------------
    
    // pula essa parte caso a rede não seja encontrada
    if(Codigo_erro == 0){
      
      //#3,4 Verifica o status do registros na rede
      sendAT("AT+CGREG?",100, false);
      // Procura a rede novamente
       while ((Codigo_erro == 4) && (contador_registro_rede < Limite_tentativas_registro_rede)){ 
        Codigo_erro = 0;
        Serial.println("Verificando Rede ...");
        delay(Intevalo_entre_tentativas);
        sendAT("AT+CGREG?", 1000, false);
        contador_registro_rede++; 
      }//-------------------------------------------------------------------------------------
           

      if (Codigo_erro == 0){
        // Tenta anexar-se à rede: Habilita o GPRS (conexão de dados)
        sendAT("AT+CGATT=1", 1000, false);
        sendAT("AT+CGATT?", 1000, false); 
        // AT+CGDCONT? // pode ser usado para verificar se a configuração abaixo ja foi feita

        // Ativa o contexto de PDP (conexão de dados) - precisa de mais tempo
        sendAT("AT+CGACT=1,1", 1000, false);
        // Define o APN da operadora primaria (aqui, TIM Brasil)
        sendAT("AT+CGDCONT=1,\"IP\",\"" + String(APN) + "\"", 1000, false);
        //AT+CGDCONT=1,"IP"," timbrasil.br"
        // mostra qual o tipo de rede conectada
        sendAT("AT+CPSI?", 1000, false);
        // Recebe data e hora da rede
        sendAT("AT+CCLK?", 1000, false); 
      }
    }
  }
  
  // Envio diario de localização ao meio dia
  if(hora == 12){
    //Liga o GPS
    sendAT("AT+CGNSSPWR=1", 1000, false);
    // Tratamento do gps
    sendAT("AT+CGNSSINFO", 1000, false);
    // Procura localização novamente
    while (Codigo_erro == 5 && contador_gps < Limite_tentativas_GPS){ 
      Codigo_erro = 0;
      Serial.println("Procurando Localizacao");
      delay(Intevalo_entre_tentativas);
      sendAT("AT+CGNSSINFO", 1000, false); 
      contador_gps++;
    }
    sendAT("AT+CGNSSPWR=0", 1000, false);        //Desliga o GPS
  }
  else{Leitura[0].Localizacao = "-1 -1";}
}

// Envia dados e sinaliza se houve erro
bool enviar_dados(String json){
  if(Codigo_erro == 0 || Codigo_erro == 5){ 

    // Encerra qualquer sessão HTTP anterior
    sendAT("AT+HTTPTERM", 1000, false);
    delay(50);
    // Inicializa a funcionalidade HTTP
    sendAT("AT+HTTPINIT", 1000, false);
    // Define a URL de destino
    sendAT("AT+HTTPPARA=\"URL\",\"" + String(FIREBASE_URL) + "\"", 1000, false);
    // Define o tipo de conteúdo
    sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 1000, false);

    // Prepara o envio do JSON com tamanho e tempo limite
    // Apos esse comando o modulo espera ate receber o json com tamnho declarado
    sendAT("AT+HTTPDATA=" + String(json.length()) + ",7200", 1000, false); 
    // Escreve JSON na serial
    Serial.println(json);                      
    // Envia o JSON para o modulo 4g lte
    sim.print(json); 
    // Espera um pouco para garantir envio                          
    delay(150);

    // Executa o POST (HTTPACTION=1) com maior timeout, tambem e possivel HTTP GET (HTTPACTION=0) e HTTP HEAD (HTTPACTION=2)
    sendAT("AT+HTTPACTION=1", 15000, true);
    // Encerra a sessão HTTP
    sendAT("AT+HTTPTERM", 1000, false);
    
    if(Codigo_erro == 0){
      // sinaliza que os dados foram enviados com sucesso
      return 0;
    }
    else{
      Serial.println("Não foi possivel enviar os dados");
      // sinaliza que os dados não foram enviados com sucesso
      return 1;
    }
  }
  else{
    Serial.println("Não foi possivel enviar os dados");
    // sinaliza que os dados não foram enviados com sucesso
    return 1;
  }
}

void sendAT(String cmd, unsigned long timeout, bool Comando_AT_especial) {
  sim.println(cmd);     // Envia o comando recebido por essa função ao modulo 4G

  long start = millis();
  String resposta = "";
  // Espera por uma resposta do modulo durante o tempo recebido pela função.
  while (millis() - start < timeout) {
    while (sim.available()) {
      char c = sim.read();
      resposta += c;
      Serial.write(c);     // Mostra cada caractere recebido
      log_put(c);

      // Se encontrar "OK", sai imediatamente
      if (resposta.indexOf("OK") != -1 && !Comando_AT_especial) {
        Diagnostico_4G(resposta);
        Serial.printf( ", dt = %u ms \n", millis() - start);
        Serial.println("--------------------------------------------------------------------");
        return;
      } 
      // Espera o final da mensagem especial (resposta do servidor ao json enviado)
      else if (resposta.indexOf("+HTTPACTION:") != -1){

        // sai do while assim que recebe o restante da mensagem +HTTPACTION
        if(c == '\n'){
          Diagnostico_4G(resposta);
          Serial.printf( "dt1 = %u ms \n", millis() - start);
          Serial.println("--------------------------------------------------------------------");
          return;
        }
      }
    }
  }
  Diagnostico_4G(resposta);
  Serial.printf( " dt = %u ms (tempo limite)\n", millis() - start);
  Serial.println("--------------------------------------------------------------------");

}


void Diagnostico_4G(String mensagem){
 
  if(mensagem.indexOf("SIM not inserted") != -1   || mensagem.indexOf("SIM failure") != -1){
    Codigo_erro = 1; // Chip ausente ou com defeito
    Serial.println(", Chip ausente ou com defeito");
  } 
  else if(mensagem.indexOf("+CSQ:") != -1){
    
    // Localiza a posição dos dois pontos ":"
    uint8_t posSeparador = mensagem.indexOf(':');
    // Extrai a parte antes da vírgula, ignorando espaços
    String valor = mensagem.substring(posSeparador + 2, mensagem.indexOf(',', posSeparador));
    // Converte para int
    Leitura[0].Sinal_4g = valor.toInt();
    
    //Isso acontece porque o modulo envia o valor 99 como "sem sinal" mesmo sendo o valor maximo
    if(Leitura[0].Sinal_4g == 99){
      Leitura[0].Sinal_4g = 0;
      Codigo_erro = 2; // Sem sinal
      Serial.println(", Sem sinal");
    }
  } 
  else if(mensagem.indexOf("+CGREG: 0,3") != -1){
    Codigo_erro = 3; // Registros na rede negado
    Serial.println(", Registros na rede negado");
  }
   else if(mensagem.indexOf("+CGREG: 0,0") != -1){
    Codigo_erro = 4; // Registrando na rede
    Serial.println(", Registrando na rede");
  }
  else if(mensagem.indexOf("+CGNSSINFO:") != -1){
    
    // Remove o prefixo "+CGNSSINFO: "
    int pos = mensagem.indexOf(":");
    String valores = mensagem.substring(pos + 2); // pula ": "

    // Quebra em vírgulas
    int idxLat = 5; // Latitude está na posição 5 depois de split
    int idxDirLat = 6;
    int idxLon = 7;
    int idxDirLon = 8;

    String partes[20]; 
    int i = 0;
    while (valores.length() > 0 && i < 20) {
      int p = valores.indexOf(",");
      if (p == -1) {
        partes[i++] = valores;
        break;
      } else {
        partes[i++] = valores.substring(0, p);
        valores = valores.substring(p + 1);
      }
    }

    String lat = partes[idxLat];
    String latDir = partes[idxDirLat];
    String lon = partes[idxLon];
    String lonDir = partes[idxDirLon];

    // Converte sinal
    if (latDir == "S") lat = "-" + lat;
    if (lonDir == "W") lon = "-" + lon;

    String loc = lat + " " + lon;
    loc.replace('.', ',');
    Serial.println(loc);

    Leitura[0].Localizacao = loc;

    // SE o dado não e valido
    if(loc.length() < 5){
      Codigo_erro = 5; // Localizacao não obtida
      Serial.println(", Localizacao nao obtida");
      Leitura[0].Localizacao = "-1 -1";
    }   
  }
  else if(mensagem.indexOf("+HTTPACTION: 1,4") != -1){
    Codigo_erro = 6; 
    Serial.println("Mensagem eviada nao pode ser processada");
  }
  else if(mensagem.indexOf("+HTTPACTION: 1,5") != -1){
    Codigo_erro = 7;
    Serial.println("Erro inisperado ocoreu no servidor");
  }
  else if(mensagem.indexOf("+HTTPACTION: 1,714") != -1){
    Codigo_erro = 8;
    Serial.println("processo de handshake de segurança entre o módulo e o servidor falhou");
  }
  else if(mensagem.indexOf("+HTTPACTION: 1,713") != -1){
    Codigo_erro = 9;
    Serial.println("Tempo limite esgotado no handshake SSL/TLS");
  }
  else if(mensagem == ""){
    Codigo_erro = 10;
    Serial.println("Modulo nao respondeu");
  }



  
   
  else if(mensagem.indexOf("+HTTPACTION: 1,2") != -1){
    Codigo_erro = 0;
    Serial.println("Mensagem enviada com sucesso !!!");
  }
  else if(mensagem.indexOf("+CCLK:") != -1){
    int aspas1 = mensagem.indexOf('"');
    int aspas2 = mensagem.lastIndexOf('"');
    String info = mensagem.substring(aspas1 + 1, aspas2);

    // Separar os componentes
    uint8_t ano  = info.substring(0, 2).toInt();  // "25" vira "2025"
    uint8_t mes  = info.substring(3, 5).toInt();
    uint8_t dia  = info.substring(6, 8).toInt();

    hora   = info.substring(9, 11).toInt();
    minuto = info.substring(12, 14).toInt();
    segundo = info.substring(15, 17).toInt();

    String fuso = info.substring(17); // "-12"

    Serial.println("Data e hora recebidas:");

    Serial.printf("%02i/%02i/20%02i \n", dia, mes, ano);
    Serial.printf("%02i:%02i:%02i \n", hora, minuto, segundo);
    Serial.println("Fuso: " + fuso);

  }
}

