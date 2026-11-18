import serial
import serial.tools.list_ports
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.widgets import Button
from collections import deque

# Detección automática del puerto USB del ESP32
puertos = list(serial.tools.list_ports.comports())
if not puertos:
    print("No se detectó ningún ESP32 conectado. Revisa el cable USB.")
    exit()

puerto_esp = next((p.device for p in puertos if "CH340" in p.description or "CP210" in p.description or "USB" in p.description), next(iter(puertos)).device)
BAUDIOS = 115200

try:
    ser = serial.Serial(puerto_esp, BAUDIOS, timeout=0.1)
    print(f"Conectado exitosamente a {puerto_esp}")
except Exception as e:
    print(f"Error al abrir {puerto_esp}: {e}")
    exit()

MAX_PUNTOS = 400
hist_I = deque(maxlen=MAX_PUNTOS)
hist_Q = deque(maxlen=MAX_PUNTOS)
hist_ojo = deque(maxlen=60)

estado = {"mod": "4-QAM", "snr": "35.0", "volt": "1.65", "bits": "00000000"}

plt.style.use('dark_background')
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 6))
plt.subplots_adjust(top=0.84, bottom=0.18, wspace=0.25)
fig.canvas.manager.set_window_title('Receptor Digital I/Q - UNP Comunicaciones II')

# Funciones de control (funcionan con clic en botón o con teclado)
def cambiar_mod(event=None):
    ser.write(b'M\n')
    hist_I.clear()
    hist_Q.clear()
    print("Cambiando esquema de modulación...")

def modo_analogico(event=None):
    ser.write(b'A\n')
    print("Modo fuente Analógica (Potenciómetro) activado")

def modo_texto(event=None):
    ser.write(b'T\n')
    hist_I.clear()
    hist_Q.clear()
    print("Modo fuente Texto ASCII activado")
    
def on_key(event):
    if not event.key:
        return
    tecla = event.key.lower()
    if tecla == 'm':
        cambiar_mod()
    elif tecla == 'a':
        modo_analogico()
    elif tecla == 't':
        modo_texto()

fig.canvas.mpl_connect('key_press_event', on_key)

# Botones interactivos en la parte inferior de la ventana
ax_btn_mod = plt.axes([0.15, 0.04, 0.22, 0.06])
ax_btn_ana = plt.axes([0.41, 0.04, 0.22, 0.06])
ax_btn_txt = plt.axes([0.67, 0.04, 0.22, 0.06])

btn_mod = Button(ax_btn_mod, 'Cambiar Modulación [M]', color='#1f3b4d', hovercolor='#00ffcc')
btn_ana = Button(ax_btn_ana, 'Fuente Analógica [A]', color='#1f3b4d', hovercolor='#00ffcc')
btn_txt = Button(ax_btn_txt, 'Texto ASCII [T]', color='#1f3b4d', hovercolor='#00ffcc')

btn_mod.on_clicked(cambiar_mod)
btn_ana.on_clicked(modo_analogico)
btn_txt.on_clicked(modo_texto)

t_ojo = np.linspace(-1, 1, 50)

def actualizar(frame):
    while ser.in_waiting:
        try:
            linea = ser.readline().decode('utf-8', errors='ignore').strip()
            partes = linea.split(',')
            if len(partes) == 6:
                mod_str, snr_str, volt_str, i_str, q_str, bits_str = partes
                estado["mod"]  = mod_str
                estado["snr"]  = snr_str
                estado["volt"] = volt_str
                estado["bits"] = bits_str

                i_val = float(i_str)
                q_val = float(q_str)

                hist_I.append(i_val)
                hist_Q.append(q_val)
                hist_ojo.append(i_val)
        except ValueError:
            pass

    if len(hist_I) < 5:
        return

    # 1. Diagrama de Constelación I/Q
    ax1.clear()
    ax1.scatter(list(hist_I), list(hist_Q), c='#00ffcc', alpha=0.65, s=18, edgecolors='none')
    ax1.axhline(0, color='gray', linestyle='--', linewidth=0.8)
    ax1.axvline(0, color='gray', linestyle='--', linewidth=0.8)
    ax1.set_xlim(-1.6, 1.6)
    ax1.set_ylim(-1.6, 1.6)
    ax1.set_title(f"Constelación: {estado['mod']} | SNR: {estado['snr']} dB", fontsize=12, fontweight='bold', color='#00ffcc', pad=12)
    ax1.set_xlabel("Componente en Fase (I)")
    ax1.set_ylabel("Componente en Cuadratura (Q)")
    ax1.grid(True, alpha=0.2)

    # 2. Diagrama de Ojo (Componente I)
    ax2.clear()
    datos_ojo = list(hist_ojo)
    for k in range(len(datos_ojo) - 2):
        y0 = datos_ojo[k]
        y1 = datos_ojo[k + 1]
        y2 = datos_ojo[k + 2]
        traza = y1 * np.cos(np.pi * t_ojo / 2)**2 + 0.5 * (y0 + y2) * np.sin(np.pi * t_ojo / 2)**2
        ax2.plot(t_ojo, traza, color='#ffaa00', alpha=0.35, linewidth=1.2)

    ax2.axhline(0, color='gray', linestyle='--', linewidth=0.8)
    ax2.axvline(0, color='cyan', linestyle=':', linewidth=0.9)
    ax2.set_xlim(-1, 1)
    ax2.set_ylim(-1.6, 1.6)
    ax2.set_title(f"Diagrama de Ojo | ADC: {estado['volt']}V | Bits: {estado['bits']}", fontsize=11, fontweight='bold', color='#ffaa00', pad=12)
    ax2.set_xlabel("Tiempo normalizado (t / Ts)")
    ax2.set_ylabel("Amplitud (I)")
    ax2.grid(True, alpha=0.2)

    fig.suptitle("Simulador Transmisor/Receptor Digital I/Q - UNP", fontsize=13, fontweight='bold', color='white', y=0.96)

ani = FuncAnimation(fig, actualizar, interval=50, cache_frame_data=False)
plt.show()
ser.close()