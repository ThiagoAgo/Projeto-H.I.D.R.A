#include <Arduino.h>
#include "externos.h"
#include "prototipos.h"

#include "config.h"



float Obter_PH(int leitura){
  float tensao_adc = leitura*(float)3.3/4096;
  // Calcular o valor de pH com base na tensão lida e no valor de calibração
  float ph_atual = -5.70 * tensao_adc + valor_calibracao_ph;
  return ph_atual*Fator_correcao_ph;
}

float Obter_Turbidez(int leitura){
  // A tensao de saida varia entre 2,5 e 4,3 logo e necessario um divisor de tensao  
  float tensao_adc = Fator_correcao_turbidez * leitura * 5.85 * (float)3.3 / 4096;
  v = tensao_adc;
  Serial.println(tensao_adc);
  // Sim, turbudez e invesamente proporcional a tensao
  // fonte da equacao https://wiki.dfrobot.com/Turbidity_sensor_SKU__SEN0189
  // http://www.ufrrj.br/institutos/it/de/acidentes/turb.htm
  float ntu;
  if (tensao_adc > 3.05 && tensao_adc <= 3.936){
    ntu = constrain(353084.68068*tensao_adc*tensao_adc*tensao_adc*tensao_adc - 4561258.81724*tensao_adc*tensao_adc*tensao_adc + 22091800.29248*tensao_adc*tensao_adc - 47545775.64101*tensao_adc + 38366273.45714 , 0, 600);
  }
  else if(tensao_adc >= 2 && tensao_adc <= 3.05){
    ntu = constrain(-2063.16399*tensao_adc*tensao_adc + 11397.54627*tensao_adc - 15133.56515, 0, 600);
  }
  else{
    ntu = -1;
  }
  return ntu;
}

float Obter_Condutividade(int leitura, float temperatura){
  // Fonte: https://randomnerdtutorials.com/esp32-tds-water-quality-sensor/
  float tensao_adc = leitura*(float)3.3 /4096.0;
  float coeficiente_de_compensacao=1.0+0.02*(temperatura-25.0); //temperatura compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float tensao_de_compensacao=tensao_adc/coeficiente_de_compensacao; //temperatura compensation
  float tds=(133.42*tensao_de_compensacao*tensao_de_compensacao*tensao_de_compensacao - 255.86*tensao_de_compensacao*tensao_de_compensacao + 857.39*tensao_de_compensacao)*0.5; //convert voltage value to tds value
  return tds*Fator_correcao_condutividade;
}

float Obter_Profundidade(){
  int32_t valor;
  if(Sensor_pressao.isReady()){
        valor = Sensor_pressao.getLastDifferentialInput();
        return ((valor - 2216532 + 47459)/37459.73)*Fator_correcao_profundicade;  
    }
    else{
        return 0;
    }
}

//retorna tensao da bateria
float Obter_bateria(u_int16_t Leitura){
  return  float(Leitura)*Fator_correcao_bateria/671.1;
}

//Filtragem por media
u_int32_t Filtro(u_int8_t pino, u_int16_t n_leituras){
  u_int32_t acc = 0; //acumulador
  for (u_int16_t i = 0; i < n_leituras; i++) {
    acc += analogRead(pino);
  }
  acc = acc / n_leituras;
  return  acc;
}


void leitura_de_sensores(DadosObtidos* d){
  sensors.requestTemperatures();       // Solicita leitura do sensor de temperatura
  delay((Tempo_inicializacao_sensor + 200)/2); // Tempo para os sensores iniciarem
  Sensor_pressao.readAndSelectNextData( HX710_DIFFERENTIAL_INPUT_40HZ ); // Solicita leitura do sensor de pressao
  delay(Tempo_inicializacao_sensor/2); // Tempo para os sensores iniciarem

  d->PH            = Obter_PH           (Filtro(Pino_PH           , 128));
  d->Turbidez      = Obter_Turbidez     (Filtro(Pino_Turbidez     , 128));
  d->Bateria       = Obter_bateria      (Filtro(Pino_bateria      , 128));
  d->Temperatura   = sensors.getTempCByIndex(0)*Fator_correcao_temperatura;
  d->Condutividade = Obter_Condutividade(Filtro(Pino_Condutividade, 128), d->Temperatura);
  d->Profundidade  = Obter_Profundidade();
  //d->Localizacao   = "-1,0000 -1,0000";
}


void estimar_parametros(DadosObtidos* dado){
  float bat = constrain(dado->Bateria, 3.2, 4.3);
  Estimado.bateria = -113.48*bat*bat*bat + 1303.5*bat*bat -4860.6*bat + 5927.9;
  
  //Estimado.demanda_biologica_o2 = 106.71 - 0.27*((dado->PH -7.57)/0.0022);
  //Estimado.demanda_quimica_o2   = 788.16 - 3.70*((dado->PH -7.57)/0.0022);
  //Estimado.nitritos_e_nitratos  = -0.004*(dado->Condutividade) - 0.043*(dado->Temperatura) + 2.936;
  
}