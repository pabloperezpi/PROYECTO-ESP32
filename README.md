# Proyecto de Procesadores digitales:
## Pablo Pérez y Eric Ayala
---

# Código de Control de Luces LED con ESP32
## Descripción General

Este código está diseñado para controlar una tira de LEDs NeoPixel utilizando un ESP32. Permite a los usuarios encender y apagar las luces, cambiar el color, ajustar el brillo y seleccionar diferentes modos de iluminación a través de una interfaz web. Además, incluye la capacidad de ajustar el brillo de los LEDs en función del sonido ambiental detectado por un micrófono.

## Bibliotecas Utilizadas

- `WiFi.h`: Para la conexión del ESP32 a una red WiFi.
- `WebServer.h`: Para la creación de un servidor web que maneje las solicitudes HTTP.
- `Adafruit_NeoPixel.h`: Para el control de la tira de LEDs NeoPixel.

## Definición de Constantes

```cpp
#define NUM_LEDS 144
#define LED_PIN 14
#define MICROPHONE_PIN 34
#define NUM_SAMPLES 10
```
Estas constantes definen el número de LEDs, los pines utilizados para la tira LED y el micrófono, y el número de muestras para el suavizado del valor del micrófono.

## Configuración de Objetos
```cpp
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);
```
- Strip: Objeto para controlar la tira de LEDs.
- Server: Objeto para manejar el servidor web.

## Credenciales WiFi
``` cpp
const char* ssid = "iPhone de AyalaKT";
const char* password = "12345678";
```
Credenciales para la conexión WiFi.
## Enumeración de Modos de Iluminación
```cpp
enum LedMode { OFF, STATIC_COLOR, RAINBOW, BLINK, RAINBOW_BLINK };
LedMode currentMode = OFF;
```
Define los modos de operación de la tira de LEDs.
## Variables Globales
```cpp
bool isOn = false;
String selectedColor = "#ff0000";
bool microphoneActive = false;
int brightness = 11;
```
Estas variables controlan el estado de las luces, el color seleccionado, el estado del micrófono y el brillo de las luces.
## Setup
``` cpp
void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
  strip.show();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/color", HTTP_GET, handleColor);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/microphone", HTTP_GET, handleMicrophone);
  server.on("/micValue", HTTP_GET, handleMicValue);
  server.begin();

  long sum = 0;
  for (int i = 0; i < calibrationSamples; i++) {
    int reading = analogRead(MICROPHONE_PIN);
    sum += reading;
    delay(10);
  }
  noiseBase = sum / calibrationSamples;

  for (int i = 0; i < smoothingSamples; i++) {
    micValues[i] = noiseBase;
    total += micValues[i];
  }
}
```
Configura la conexión WiFi, inicializa la tira de LEDs, calibra el micrófono y configura las rutas del servidor web.
## Bucle Principal
```cpp
void loop() {
  server.handleClient();

  if (isOn) {
    if (microphoneActive) {
      int adjustedMicValue = getSmoothedMicValue();
      int mappedBrightness = map(adjustedMicValue, 0, 1023 - noiseBase, 1, 255);
      strip.setBrightness(mappedBrightness);
    } else {
      strip.setBrightness(brightness);
    }

    uint32_t color = parseColor(selectedColor);
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();

    switch (currentMode) {
      case STATIC_COLOR:
        for (int i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, color);
        }
        strip.show();
        break;
      case RAINBOW:
        showRainbow();
        break;
      case BLINK:
        showBlink(color, brightness, 500);
        break;
      case RAINBOW_BLINK:
        showRandomRainbowBlink(brightness, 500);
        break;
      case OFF:
      default:
        strip.clear();
        strip.show();
        break;
    }
  } else {
    strip.clear();
    strip.show();
  }

  delay(100);
}
```
Maneja las solicitudes del servidor y actualiza la tira de LEDs según el modo y estado actual.
## Funciones de Manejo del Servidor Web
### HandleRoot
```cpp
void handleRoot() {
  server.send(200, "text/html", HTML);
}
Envía la página HTML de control al cliente.

### HandleToggle
```cpp

void handleToggle() {
  isOn = !isOn;
  server.send(200, "text/plain", isOn ? "on" : "off");
}
```
Enciende o apaga las luces.

### HandleColor
``` cpp

void handleColor() {
  selectedColor = server.arg("value");
  server.send(200, "text/plain", selectedColor);
}
```
Cambia el color de las luces.

### HandleBrightness
```cpp
void handleBrightness() {
  brightness = server.arg("value").toInt();
  server.send(200, "text/plain", String(brightness));
}
```
Ajusta el brillo de las luces.

### HandleMode
```cpp
void handleMode() {
  String mode = server.arg("value");
  if (mode == "fixed") {
    currentMode = STATIC_COLOR;
  } else if (mode == "rainbow") {
    currentMode = RAINBOW;
  } else if (mode == "blink") {
    currentMode = BLINK;
  } else if (mode == "rainbowblink") {
    currentMode = RAINBOW_BLINK;
  }
  server.send(200, "text/plain", mode);
}
```
Cambia el modo de operación de las luces.

### HandleMicrophone
```cpp
void handleMicrophone() {
  microphoneActive = !microphoneActive;
  server.send(200, "text/plain", microphoneActive ? "on" : "off");
}
```
Activa o desactiva el ajuste de brillo basado en el micrófono.

### HandleMicValue
``` cpp
void handleMicValue() {
  int micValue = getSmoothedMicValue();
  server.send(200, "text/plain", String(micValue));
}
```
Envía el valor actual del micrófono al cliente.

### Funciones de Control de LEDs
``` cpp
uint32_t parseColor(String color) {
  color.remove(0, 1);
  long number = strtol(color.c_str(), NULL, 16);
  return strip.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
}
```
Convierte un color en formato hexadecimal a un valor RGB.

### showRainbow
```cpp
void showRainbow() {
  static uint16_t j = 0;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((i * 65536L / strip.numPixels() + j) & 65535)));
  }
  strip.show();
  j++;
}
```
Muestra un efecto de arcoíris en la tira de LEDs.

### showBlink
``` cpp
void showBlink(uint32_t color, int brightness, int delayTime) {
  strip.setBrightness(brightness);
  strip.fill(color);
  strip.show();
  delay(delayTime);
  strip.clear();
  strip.show();
  delay(delayTime);
}
```
Muestra un efecto de parpadeo con un color fijo.

### showRandomRainbowBlink
```cpp
void showRandomRainbowBlink(int brightness, int delayTime) {
  strip.setBrightness(brightness);
  for (int i = 0; i < strip.numPixels(); i++) {
    uint32_t color = strip.gamma32(strip.ColorHSV(random(0, 65536)));
    strip.setPixelColor(i, color);
  }
  strip.show();
  delay(delayTime);
  strip.clear();
  strip.show();
  delay(delayTime);
}
```
Muestra un efecto de parpadeo con colores aleatorios del arcoíris.

## Funciones de Procesamiento del Micrófono
### getSmoothedMicValue
```cpp
int getSmoothedMicValue() {
  int micValue = analogRead(MICROPHONE_PIN);

  total -= micValues[currentIndex];
  micValues[currentIndex] = micValue;
  total += micValues[currentIndex];

  currentIndex++;
  if (currentIndex >= smoothingSamples) {
    currentIndex = 0;
  }

  int smoothedMicValue = total / smoothingSamples;
  int adjustedMicValue = smoothedMicValue - noiseBase;
  if (adjustedMicValue < 0) {
   adjustedMicValue = 0;
  }

  return adjustedMicValue;
}
```
Obtiene un valor suavizado del micrófono para reducir el ruido.

## Interfaz Web
### Página HTML
```html
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>Control de Luces LED</title>
    <style>
        /* Estilos omitidos para brevedad */
    </style>
</head>
<body>
    <!-- Contenido HTML omitido para brevedad -->
</body>
</html>
```
Define la interfaz de usuario para el control de las luces LED.

## Manejo de Eventos
``` javascript
document.getElementById('toggleButton').addEventListener('click', function() {
    fetch('/toggle').then(response => response.text()).then(state => {
        document.getElementById('toggleButton').innerText = state === 'on' ? 'Apagar' : 'Encender';
    });
});

document.querySelectorAll('.color-box').forEach(function(element) {
    element.addEventListener('click', function() {
        let color = this.style.backgroundColor;
        color = rgbToHex(color);
        fetch('/color?value=' + color).then(response => response.text());
    });
});

document.getElementById('brightnessSlider').addEventListener('input', function() {
    let brightness = this.value;
    fetch('/brightness?value=' + brightness).then(response => response.text());
});

document.getElementById('fixedModeButton').addEventListener('click', function() {
    fetch('/mode?value=fixed').then(response => response.text());
});

document.getElementById('blinkModeButton').addEventListener('click', function() {
    fetch('/mode?value=blink').then(response => response.text());
});

document.getElementById('rainbowModeButton').addEventListener('click', function() {
    fetch('/mode?value=rainbow').then(response => response.text());
});

document.getElementById('rainbowBlinkModeButton').addEventListener('click', function() {
    fetch('/mode?value=rainbowblink').then(response => response.text());
});

document.getElementById('microphoneButton').addEventListener('click', function() {
    fetch('/microphone').then(response => response.text()).then(state => {
        document.getElementById('microphoneButton').innerText = state === 'on' ? 'Desactivar' : 'Activar';
    });
});

function rgbToHex(rgb) {
    let result = rgb.match(/\d+/g);
    let r = parseInt(result[0]).toString(16).padStart(2, '0');
    let g = parseInt(result[1]).toString(16).padStart(2, '0');
    let b = parseInt(result[2]).toString(16).padStart(2, '0');
    return '#' + r + g + b;
}
```
Maneja los eventos de la interfaz web para interactuar con el servidor.

## Conclusión
Este código permite controlar una tira de LEDs NeoPixel utilizando un ESP32 a través de una interfaz web. Incluye funciones para encender/apagar las luces, cambiar colores, ajustar el brillo, seleccionar modos de iluminación y utilizar un micrófono para ajustar el brillo según el sonido ambiental. El uso de bibliotecas como WiFi.h, WebServer.h y Adafruit_NeoPixel.h facilita la implementación de estas funcionalidades.

``` mermaid
graph TD
    A[Start] --> B[Initialize Serial]
    B --> C[Configure Pins]
    C --> D[Initialize NeoPixel]
    D --> E[Connect to WiFi]
    E --> F[Configure Server Routes]
    F --> G[Calibrate Microphone]
    G --> H{Main Loop}
    
    H --> I[Handle Client]
    I --> J{Lights On?}
    J -->|Yes| K{Microphone Active?}
    K -->|Yes| L[Adjust Brightness with Microphone]
    K -->|No| M[Adjust Fixed Brightness]
    J -->|No| N[Turn Off Lights]
    
    L --> O{Operation Mode}
    M --> O
    
    O --> P[STATIC_COLOR]
    O --> Q[RAINBOW]
    O --> R[BLINK]
    O --> S[RAINBOW_BLINK]
    O --> T[OFF]

    P --> U[Show Fixed Color]
    Q --> V[Show Rainbow]
    R --> W[Show Blink]
    S --> X[Show Rainbow Blink]
    T --> N
    
    U --> Y[Update LEDs]
    V --> Y
    W --> Y
    X --> Y
    Y --> Z[Delay 100 ms]
    Z --> H
   ````
``` mermaid
graph TD 
    subgraph Server Handlers
        A1[handleRoot] --> A2[Send HTML]
        B1[handleToggle] --> B2[Toggle Lights]
        C1[handleColor] --> C2[Change Color]
        D1[handleBrightness] --> D2[Adjust Brightness]
        E1[handleMode] --> E2[Change Mode]
        F1[handleMicrophone] --> F2[Toggle Microphone]
        G1[handleMicValue] --> G2[Get Mic Value]
    end


```
# Montaje
