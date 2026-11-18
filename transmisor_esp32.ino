#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Pantalla OLED integrada en Ideaspark ESP32 (SDA=21, SCL=22)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pines analógicos conectados a los potenciómetros y fuente auxiliar
const int PIN_POWER_3V3  = 13; // Pin D13 configurado como fuente de 3.3V
const int PIN_ADC_FUENTE = 34; // Potenciómetro 1: Señal analógica
const int PIN_ADC_SNR    = 35; // Potenciómetro 2: Nivel SNR (dB)

// Modulaciones disponibles
int modIndex = 3; // Inicia en 4-QAM
const char* modNames[] = {"ASK", "FSK", "PSK", "4-QAM", "16-QAM", "64-QAM", "256-QAM", "1024-QAM"};
const int numMods = 8;

bool modoTexto = false;
String textoBuffer = "UNP";
int bitPointer = 0;
uint8_t bitStream[256];
int totalBits = 0;

unsigned long lastSampleTime = 0;
const unsigned long Ts_us = 20000; // Periodo de muestreo Ts = 20 ms (Fs = 50 Hz)

// Generador de ruido Gaussiano (Método Box-Muller)
float generarGaussiano() {
  float u1 = (random(1, 10000)) / 10000.0;
  float u2 = (random(1, 10000)) / 10000.0;
  return sqrt(-2.0 * log(u1)) * cos(2.0 * PI * u2);
}

// Convertir texto ASCII a flujo de bits
void textoABits(String txt) {
  totalBits = 0;
  for (int i = 0; i < txt.length() && totalBits < 240; i++) {
    char c = txt[i];
    for (int b = 7; b >= 0; b--) {
      bitStream[totalBits++] = (c >> b) & 1;
    }
  }
  bitPointer = 0;
}

// Leer siguiente bit del flujo activo
uint8_t leerSiguienteBit() {
  if (totalBits == 0) return random(0, 2);
  uint8_t b = bitStream[bitPointer];
  bitPointer = (bitPointer + 1) % totalBits;
  return b;
}

// Agrupar k bits en un símbolo entero
int leerSimboloBits(int k) {
  int val = 0;
  for (int i = 0; i < k; i++) {
    val = (val << 1) | leerSiguienteBit();
  }
  return val;
}

// Mapeo matemático de bits a coordenadas I/Q normalizadas
void mapearIQ(int esquema, float &I, float &Q) {
  if (esquema == 0) { // ASK (OOK / 2-ASK)
    int b = leerSimboloBits(1);
    I = (b == 1) ? 1.0 : 0.2;
    Q = 0.0;
  } 
  else if (esquema == 1) { // FSK (Representación ortogonal en banda base)
    int b = leerSimboloBits(1);
    I = (b == 0) ? 1.0 : 0.0;
    Q = (b == 1) ? 1.0 : 0.0;
  } 
  else if (esquema == 2) { // QPSK (4 fases)
    int sym = leerSimboloBits(2);
    float angulo = (sym * PI / 2.0) + (PI / 4.0);
    I = cos(angulo);
    Q = sin(angulo);
  } 
  else { // M-QAM cuadrada: 4, 16, 64, 256, 1024-QAM
    int k = 2;
    if (esquema == 4) k = 4;       // 16-QAM
    else if (esquema == 5) k = 6;  // 64-QAM
    else if (esquema == 6) k = 8;  // 256-QAM
    else if (esquema == 7) k = 10; // 1024-QAM

    int bitsEje = k / 2;
    int L = 1 << bitsEje;
    int symI = leerSimboloBits(bitsEje);
    int symQ = leerSimboloBits(bitsEje);

    float maxNivel = (float)(L - 1);
    I = (2.0 * symI - maxNivel) / maxNivel;
    Q = (2.0 * symQ - maxNivel) / maxNivel;
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // ADC de 12 bits (0 - 4095)
  
  // Encender pin D13 como fuente estable de 3.3V para los potenciómetros
  pinMode(PIN_POWER_3V3, OUTPUT);
  digitalWrite(PIN_POWER_3V3, HIGH);
  
  Wire.begin(21, 22);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.println("UNP - COMUNICACIONES II");
    display.println("Simulador Digital I/Q");
    display.display();
    delay(1500);
  }
  textoABits(textoBuffer);
}

void loop() {
  // Comandos desde Python: 'M' cambia modulación, 'A' modo analógico, 'T:texto' modo texto
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "M" || cmd == "m") {
      modIndex = (modIndex + 1) % numMods;
    } else if (cmd == "A" || cmd == "a") {
      modoTexto = false;
      } else if (cmd == "T" || cmd == "t" || cmd.startsWith("T:")) {
      modoTexto = true;
      static int wordIdx = 0;
      const char* palabras[] = {"UNP_2026", "TELECOM", "PIURA_PE", "ESP32_IQ"};
      textoBuffer = cmd.startsWith("T:") ? cmd.substring(2) : palabras[wordIdx];
      wordIdx = (wordIdx + 1) % 4;
      textoABits(textoBuffer);
    }

  }

  // Muestreo periódico (Teorema de Nyquist)
  if (micros() - lastSampleTime >= Ts_us) {
    lastSampleTime = micros();

    // 1. Adquisición y cuantización PCM de 8 bits
    int rawADC = analogRead(PIN_ADC_FUENTE);
    float voltaje = (rawADC / 4095.0) * 3.3;
    int nivelCuantizado = map(rawADC, 0, 4095, 0, 255);

    if (!modoTexto) {
      totalBits = 8;
      for (int b = 7; b >= 0; b--) {
        bitStream[7 - b] = (nivelCuantizado >> b) & 1;
      }
    }

    // 2. Mapeo matemático I/Q
    float I_ideal, Q_ideal;
    mapearIQ(modIndex, I_ideal, Q_ideal);

    // 3. Canal AWGN con SNR variable (2 dB a 35 dB)
    int rawSNR = analogRead(PIN_ADC_SNR);
    float snr_dB = map(rawSNR, 0, 4095, 2, 35);
    float snr_lineal = pow(10.0, snr_dB / 10.0);
    float sigma = sqrt(1.0 / (2.0 * snr_lineal));

    float I_rx = I_ideal + sigma * generarGaussiano();
    float Q_rx = Q_ideal + sigma * generarGaussiano();

    // 4. Envío de datos a Python por USB: MOD,SNR,VOLT,I_RX,Q_RX,BITS
    Serial.print(modNames[modIndex]); Serial.print(",");
    Serial.print(snr_dB, 1);          Serial.print(",");
    Serial.print(voltaje, 2);         Serial.print(",");
    Serial.print(I_rx, 4);            Serial.print(",");
    Serial.print(Q_rx, 4);            Serial.print(",");
    for (int i = 0; i < 8; i++) Serial.print(bitStream[i]);
    Serial.println();

    // 5. Actualizar pantalla OLED cada 10 muestras
    static int counter = 0;
    if (++counter >= 10) {
      counter = 0;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.print("MOD: "); display.println(modNames[modIndex]);
      display.print("SNR: "); display.print(snr_dB, 1); display.println(" dB");
      display.print("SRC: "); display.println(modoTexto ? "TXT:" + textoBuffer : "ADC " + String(voltaje, 2) + "V");
      display.print("BITS: ");
      for (int i = 0; i < 8; i++) display.print(bitStream[i]);
      display.display();
    }
  }
}