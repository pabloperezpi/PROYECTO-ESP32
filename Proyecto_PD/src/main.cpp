#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

// Definición de constantes
#define NUM_LEDS 50 // Número de LEDs en la tira LED
#define MIC_PIN A0  // Pin del micrófono
#define LED_PIN 27  // Pin de la tira LED
#define NUM_LEDS 144
#define LED_PIN 14
#define MICROPHONE_PIN 34
#define NUM_SAMPLES 10

// Creación de un objeto para controlar la tira LED NeoPixel
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Credenciales WiFi
const char* ssid = "iPhone de Pablo";
const char* password = "pabloperez";
const char* ssid = "iPhone de AyalaKT";
const char* password = "12345678";

// Configuración del servidor web
WebServer server(80);

// Declaración de las funciones que manejarán las solicitudes HTTP
void handleToggle();
void handleColor();
void handleBrightness();
void handleMode();
void handleMicrophone();
void handleRoot();
void handleMicValue();
uint32_t parseColor(String color);
void showRainbow();
void showBlink(uint32_t color, int brightness, int delayTime);
void showRandomRainbowBlink(int brightness, int delayTime);

// Enumeración para los modos de la tira LED
enum LedMode { OFF, STATIC_COLOR, RAINBOW, BLINK, RAINBOW_BLINK };
LedMode currentMode = OFF;

// Variables para el estado de las luces LED
bool isOn = false;
String selectedColor = "#ff0000"; // Color por defecto
bool microphoneActive = false;
int brightness = 11;

// Variables para el procesamiento del micrófono
const int calibrationSamples = 100;
const int smoothingSamples = 10;
int noiseBase = 0;
int micValues[smoothingSamples];
int currentIndex = 0;
int total = 0;

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  strip.begin();
  strip.show(); // Inicializa la tira LED apagada
  strip.setBrightness(brightness);
  strip.show();

  // Conexión a la red WiFi
  Serial.println("Intentando conectar a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Espera hasta que se conecte a WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando...");
  }

  Serial.println("Conectado a WiFi");

  // Configuración de las rutas del servidor web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/color", HTTP_GET, handleColor);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/microphone", HTTP_GET, handleMicrophone);
  server.on("/micValue", HTTP_GET, handleMicValue);

  // Inicia el servidor web
  server.begin();
  Serial.println("Servidor HTTP iniciado");

  // Calibración inicial del micrófono
  long sum = 0;
  for (int i = 0; i < calibrationSamples; i++) {
    int reading = analogRead(MICROPHONE_PIN);
    sum += reading;
    delay(10);
  }
  noiseBase = sum / calibrationSamples;
  Serial.print("Nivel de ruido ambiental calibrado: ");
  Serial.println(noiseBase);

  // Inicialización del array de valores del micrófono
  for (int i = 0; i < smoothingSamples; i++) {
    micValues[i] = noiseBase;
    total += micValues[i];
  }
}

// Función para obtener el valor suavizado del micrófono
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

void loop() {
  server.handleClient(); // Maneja las solicitudes HTTP entrantes

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

  delay(100); // Intervalo de actualización
}

// Función para parsear un color en formato hexadecimal a RGB
uint32_t parseColor(String color) {
  color.remove(0, 1); // Quitar el '#'
  long number = strtol(color.c_str(), NULL, 16);
  return strip.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
}

// Función para mostrar un efecto arcoíris
void showRainbow() {
  static uint16_t j = 0;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((i * 65536L / strip.numPixels() + j) & 65535)));
  }
  strip.show();
  j++;
}

// Función para mostrar un efecto de parpadeo
void showBlink(uint32_t color, int brightness, int delayTime) {
  strip.setBrightness(brightness);
  strip.fill(color);
  strip.show();
  delay(delayTime);
  strip.clear();
  strip.show();
  delay(delayTime);
}

// Función para mostrar un efecto de parpadeo arcoíris
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

// Código HTML para la interfaz de usuario
String HTML = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Control de Luces LED</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
            text-align: center;
            margin-top: 50px;
        }
        .btn {
            padding: 10px 20px;
            font-size: 16px;
            margin: 10px;
            cursor: pointer;
        }
        .color-box {
            width: 30px;
            height: 30px;
            display: inline-block;
            margin: 5px;
            border-radius: 50%;
            cursor: pointer;
        }
        .brightness-slider {
            margin-top: 20px;
        }
        .slider {
            width: 100%;
        }
    </style>
</head>
<body>
    <div>
        <h1>Control de Luces LED</h1>
        <button id="toggleButton" class="btn">Encender</button>
    </div>
    <div>
        <h2>Modo</h2>
        <button id="fixedModeButton" class="btn">Color fijo</button>
        <button id="blinkModeButton" class="btn">Parpadeo</button>
        <button id="rainbowModeButton" class="btn">Arcoíris</button>
        <button id="rainbowBlinkModeButton" class="btn">Parpadeo Arcoíris</button>
    </div>
    <div>
        <h2>Colores</h2>
        <div class="color-palette">
            <div class="color-box" style="background-color: #ff0000;"></div>
            <div class="color-box" style="background-color: #ff8000;"></div>
            <div class="color-box" style="background-color: #ffff00;"></div>
            <div class="color-box" style="background-color: #00ff00;"></div>
            <div class="color-box" style="background-color: #0000ff;"></div>
            <div class="color-box" style="background-color: #4b0082;"></div>
            <div class="color-box" style="background-color: #9400d3;"></div>
            <div class="color-box" style="background-color: #ffffff;"></div>
        </div>
    </div>
    <div class="brightness-slider">
        <h2>Brillo</h2>
        <input type="range" id="brightnessSlider" class="slider" min="1" max="255" value="11">
    </div>
    <div>
        <h2>Micrófono</h2>
        <button id="microphoneButton" class="btn">Activar</button>
    </div>
    <script>
        document.getElementById('toggleButton').addEventListener('click', function() {
            fetch('/toggle')
                .then(response => response.text())
                .then(state => {
                    document.getElementById('toggleButton').innerText = state === 'on' ? 'Apagar' : 'Encender';
                });
        });

        document.querySelectorAll('.color-box').forEach(function(element) {
            element.addEventListener('click', function() {
                let color = this.style.backgroundColor;
                color = rgbToHex(color);
                fetch('/color?value=' + color)
                    .then(response => response.text());
            });
        });

        document.getElementById('brightnessSlider').addEventListener('input', function() {
            let brightness = this.value;
            fetch('/brightness?value=' + brightness)
                .then(response => response.text());
        });

        document.getElementById('fixedModeButton').addEventListener('click', function() {
            fetch('/mode?value=fixed')
                .then(response => response.text());
        });

        document.getElementById('blinkModeButton').addEventListener('click', function() {
            fetch('/mode?value=blink')
                .then(response => response.text());
        });

        document.getElementById('rainbowModeButton').addEventListener('click', function() {
            fetch('/mode?value=rainbow')
                .then(response => response.text());
        });

        document.getElementById('rainbowBlinkModeButton').addEventListener('click', function() {
            fetch('/mode?value=rainbowblink')
                .then(response => response.text());
        });

        document.getElementById('microphoneButton').addEventListener('click', function() {
            fetch('/microphone')
                .then(response => response.text())
                .then(state => {
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
    </script>
</body>
</html>
)=====";

// Manejo de solicitudes HTTP
void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleToggle() {
  isOn = !isOn;
  server.send(200, "text/plain", isOn ? "on" : "off");
}

void handleColor() {
  selectedColor = server.arg("value");
  server.send(200, "text/plain", selectedColor);
}

void handleBrightness() {
  brightness = server.arg("value").toInt();
  server.send(200, "text/plain", String(brightness));
}

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

void handleMicrophone() {
  microphoneActive = !microphoneActive;
  server.send(200, "text/plain", microphoneActive ? "on" : "off");
}

void handleMicValue() {
  int micValue = getSmoothedMicValue();
  server.send(200, "text/plain", String(micValue));
}
