# Simulador de Transmisor / Receptor Digital con ESP32 y Visualización I/Q

Proyecto Final del curso **Teoría de las Comunicaciones II** — Universidad Nacional de Piura (UNP).

## Descripción del Sistema
Sistema de comunicación digital en banda base implementado sobre una placa **Ideaspark ESP32-WROOM-32** con pantalla OLED SSD1306 integrada y receptor gráfico en **Python (PySerial + Matplotlib)**.

### Características Principales
* **Fuentes de Información:** Adquisición analógica mediante ADC (GPIO 34) con cuantización PCM de 8 bits y transmisión de cadenas de texto ASCII convertidas a flujo binario.
* **Modulaciones Digitales (Mapeo Matemático I/Q):** ASK, FSK, PSK, 4-QAM, 16-QAM, 64-QAM, 256-QAM y 1024-QAM sin uso de librerías prefabricadas.
* **Canal AWGN en Tiempo Real:** Inyección de ruido gaussiano aditivo mediante el algoritmo de Box-Muller controlado por potenciómetro (GPIO 35) entre 2 dB y 35 dB.
* **Visualización:** Telemetría local en pantalla OLED (I2C GPIO 21/22) y graficación en PC de **Diagrama de Constelación** y **Diagrama de Ojo**.

## Archivos del Repositorio
* `transmisor_esp32.ino`: Firmware en C++ para Arduino IDE (muestreo, mapeo I/Q, canal AWGN y pantalla OLED).
* `simulador_rx.py`: Interfaz receptora en Python para visualización en tiempo real y control interactivo.
*
