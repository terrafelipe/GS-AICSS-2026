// Integrantes
// Felipe Piccolo - RM569324
// Leonardo Bueno - RM572152
// Joao Veiga - RM569874


#include <Arduino.h>
#include <DHT.h>
#include <EasyUltrasonic.h>


#define PIN_DHT     26
#define DHTMODEL DHT22
DHT dht(PIN_DHT, DHTMODEL);   
#define PIN_TRIG    23   
#define PIN_ECHO    22
EasyUltrasonic sensorDist;   
#define PIN_LED     21   
#define PIN_BUZZER  19  
#define FREQ 400


int INTERVALO_COLETA = 2100;
uint64_t tempoAnterior = 0;


float TEMP_ATENCAO  = 30.0;
float TEMP_CRITICO  = 38.0;

float UMID_ATENCAO  = 70.0;
float UMID_CRITICO  = 85.0;

float DIST_ATENCAO  = 40.0;
float DIST_CRITICO  = 20.0;

// --------- NIVEIS DE ALERTA ---------
enum NivelAlerta { NOMINAL = 0, ATENCAO = 1, CRITICO = 2 };

// --------- ESTADO DO SISTEMA ---------
bool sistemaAtivo = false;
int  totalLeituras = 0;
int  contadorCritico = 0;


// =======================================================
//  SETUP
// =======================================================
void setup() {
  Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    sensorDist.attach(PIN_TRIG, PIN_ECHO);
    dht.begin();

    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);

    Serial.println("====================================");
    Serial.println(" MONITORAMENTO - MODULO ESPACIAL");
    Serial.println("====================================");
    Serial.println(" Comandos (digite e ENTER):");
    Serial.println("   STATUS  -> contadores");
    Serial.println("   PARAR   -> pausa");
    Serial.println("   INICIAR -> retoma");
    Serial.println("   TESTE   -> testa LED e buzzer");
    Serial.println("------------------------------------");
}

// =======================================================
//  LOOP PRINCIPAL
// =======================================================
void loop() {
  // 1) Entrada do usuario (sempre responsiva)
    processarComandoSerial();


    if (sistemaAtivo && millis() - tempoAnterior >= INTERVALO_COLETA) {
        tempoAnterior = millis();
        float temp = dht.readTemperature();
        float umid = dht.readHumidity();
        float dist = sensorDist.getDistanceCM();
        Serial.println(dist);
        if (isnan(temp) || isnan(umid)) {
            Serial.println("Falha na leitura");
            return;
        }

        totalLeituras++;

        NivelAlerta nivel = avaliarNivel(temp, umid, dist);
        if (nivel == CRITICO) contadorCritico++;

        acionarAtuadores(nivel);
        imprimirRelatorio(temp, umid, dist, nivel);
    }
}

// Retorna o PIOR nivel entre os tres parametros
NivelAlerta avaliarNivel(float temp, float umid, float dist) {
    NivelAlerta nivel = NOMINAL;

    if (temp >= TEMP_CRITICO)      nivel = max(nivel, CRITICO);
    else if (temp >= TEMP_ATENCAO) nivel = max(nivel, ATENCAO);

    if (umid >= UMID_CRITICO)      nivel = max(nivel, CRITICO);
    else if (umid >= UMID_ATENCAO) nivel = max(nivel, ATENCAO);

    if (dist <= DIST_CRITICO)      nivel = max(nivel, CRITICO);
    else if (dist <= DIST_ATENCAO) nivel = max(nivel, ATENCAO);

    return nivel;
}

// LED + buzzer com padrao distinto por nivel
void acionarAtuadores(NivelAlerta nivel) {
    switch (nivel) {
        case NOMINAL:
            digitalWrite(PIN_LED, LOW);
            noTone(PIN_BUZZER);
            break;

        case ATENCAO:
            for (int i = 0; i < 2; i++) {
                digitalWrite(PIN_LED, HIGH);
                tone(PIN_BUZZER, FREQ);
                delay(120);
                digitalWrite(PIN_LED, LOW);
                noTone(PIN_BUZZER);
                delay(120);
            }
            break;

        case CRITICO:
            for (int i = 0; i < 5; i++) {
                digitalWrite(PIN_LED, HIGH);
                tone(PIN_BUZZER, FREQ * 2);
                delay(60);
                digitalWrite(PIN_LED, LOW);
                delay(60);
            }
            noTone(PIN_BUZZER);
            break;
    }
}

// Entrada de texto via Serial
void processarComandoSerial() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "PARAR") {
        sistemaAtivo = false;
        digitalWrite(PIN_LED, LOW);
        noTone(PIN_BUZZER);
        Serial.println(">> Monitoramento PAUSADO.");
    }
    else if (cmd == "INICIAR") {
        sistemaAtivo = true;
        Serial.println(">> Monitoramento RETOMADO.");
    }
    else if (cmd == "STATUS") {
        Serial.print(">> Leituras: ");
        Serial.print(totalLeituras);
        Serial.print(" | Criticos: ");
        Serial.println(contadorCritico);
    }
    else if (cmd == "TESTE") {
        Serial.println(">> Testando atuadores...");
        digitalWrite(PIN_LED, HIGH);
        tone(PIN_BUZZER, FREQ);
        delay(500);
        digitalWrite(PIN_LED, LOW);
        noTone(PIN_BUZZER);
        Serial.println(">> Teste concluido.");
    }
    else {
        Serial.print(">> Comando desconhecido: ");
        Serial.println(cmd);
    }
}

// Relatorio no Serial Monitor
void imprimirRelatorio(float temp, float umid, float dist, NivelAlerta nivel) {
    Serial.println("------------------------------------");
    Serial.print("Temp: "); Serial.print(temp, 1); Serial.print(" C | ");
    Serial.print("Umid: "); Serial.print(umid, 1); Serial.print(" % | ");
    Serial.print("Dist: "); Serial.print(dist, 1); Serial.println(" cm");
    Serial.print("ESTADO: [ ");
    Serial.print(nomeNivel(nivel));
    Serial.println(" ]");
}

const char* nomeNivel(NivelAlerta n) {
    switch (n) {
        case NOMINAL: return "NOMINAL";
        case ATENCAO: return "ATENCAO";
        case CRITICO: return "CRITICO";
        default:      return "?";
    }
}