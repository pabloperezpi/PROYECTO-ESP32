#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 144
#define LED_PIN 14

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "iPhone de Pablo";
const char* password = "pabloperez";

WebServer server(80);

void handleToggle();
void handleColor();
void handleBrightness();
void handleMode();
void handleDirection();
void handleWaveControl();
void handleSpeed();
void handleRoot();
uint32_t parseColor(String color);
void showRainbow();
void showBlink(uint32_t color, int brightness, int delayTime);
void showRandomRainbowBlink(int brightness, int delayTime);
void showWave();
void showBreathing();
void showFireworks();

enum LedMode { OFF, STATIC_COLOR, RAINBOW, BLINK, RAINBOW_BLINK, WAVE, BREATHING, FIREWORKS };
LedMode currentMode = OFF;

bool isOn = false;
String selectedColor = "#ff0000";
int brightness = 11;
bool waveDirectionRight = true;
bool wavePaused = false;
int blinkSpeed = 500;
int waveSpeed = 100;

struct Firework {
  int position;
  int velocity;
  uint32_t color;
  int lifetime;
};

Firework fireworks[10];

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(brightness);
  strip.show();

  Serial.println("Intentando conectar a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.print("IP obtenida: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/color", HTTP_GET, handleColor);
  server.on("/brightness", HTTP_GET, handleBrightness);
  server.on("/mode", HTTP_GET, handleMode);
  server.on("/direction", HTTP_GET, handleDirection);
  server.on("/waveControl", HTTP_GET, handleWaveControl);
  server.on("/speed", HTTP_GET, handleSpeed);

  // Manejador para rutas no encontradas
  server.onNotFound([]() {
      server.send(404, "text/plain", "Not found");
  });

  server.begin();

  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();

  if (isOn) {
    strip.setBrightness(brightness);
    uint32_t color = parseColor(selectedColor);

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
        showBlink(color, brightness, blinkSpeed);
        break;
      case RAINBOW_BLINK:
        showRandomRainbowBlink(brightness, blinkSpeed);
        break;
      case WAVE:
        if (!wavePaused) {
          showWave();
        }
        break;
      case BREATHING:
        showBreathing();
        break;
      case FIREWORKS:
        showFireworks();
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

  delay(10);
}

uint32_t parseColor(String color) {
  color.remove(0, 1);
  long number = strtol(color.c_str(), NULL, 16);
  return strip.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
}

void showRainbow() {
  static uint16_t j = 0;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((i * 65536L / strip.numPixels() + j) & 65535)));
  }
  strip.show();
  j++;
}

void showBlink(uint32_t color, int brightness, int delayTime) {
  strip.setBrightness(brightness);
  strip.fill(color);
  strip.show();
  delay(delayTime);
  strip.clear();
  strip.show();
  delay(delayTime);
}

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

void showWave() {
  static uint16_t wavePosition = 0;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    int brightness = (sin((waveDirectionRight ? i : (strip.numPixels() - i)) + wavePosition / 10.0) * 127.5) + 127.5;
    strip.setPixelColor(i, strip.Color(
      (strip.gamma32(parseColor(selectedColor)) >> 16 & 0xFF) * brightness / 255,
      (strip.gamma32(parseColor(selectedColor)) >> 8 & 0xFF) * brightness / 255,
      (strip.gamma32(parseColor(selectedColor)) & 0xFF) * brightness / 255
    ));
  }
  strip.show();
  wavePosition += waveSpeed / 100;
}

void showBreathing() {
  static int breatheStep = 0;
  static int direction = 1; // 1: increasing brightness, -1: decreasing brightness
  int breatheSpeed = 7; // Adjust this value to change breathing speed

  breatheStep += direction * breatheSpeed;
  if (breatheStep >= 255 || breatheStep <= 0) {
    direction = -direction;
    breatheStep = constrain(breatheStep, 0, 255);
  }
  
  strip.fill(parseColor(selectedColor));
  strip.setBrightness(breatheStep);
  strip.show();
  delay(30); // Adjust this delay to control breathing smoothness
}

void showFireworks() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  
  if (now - lastUpdate > 100) { // Ajustar la velocidad cambiando este valor
    lastUpdate = now;
    strip.clear();
    
    // Generar estallidos de "fuegos artificiales" aleatorios
    for (int i = 0; i < 10; i++) {
      int pixel = random(NUM_LEDS);
      uint32_t color = strip.Color(random(0, 255), random(0, 255), random(0, 255));
      strip.setPixelColor(pixel, color);
    }
    
    strip.show();
  }
}


String HTML = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Control de luces LED</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }

        .container {
            text-align: center;
            background-color: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }

        .btn {
            display: inline-block;
            padding: 10px 20px;
            font-size: 16px;
            margin: 10px;
            color: #fff;
            background-color: #007BFF;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }

        .btn:active {
            background-color: #0056b3;
        }

        .color-box {
            width: 50px;
            height: 50px;
            display: inline-block;
            margin: 5px;
            cursor: pointer;
            border-radius: 50%;
        }

        #brightness,
        #speed {
            width: 100%;
            margin: 10px 0;
        }

        .section {
            margin-bottom: 20px;
        }

        .section h2 {
            margin-bottom: 10px;
            font-size: 18px;
            color: #333;
        }

        #speedSection, #waveControls, #colorPickerSection {
            display: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Control de luces LED</h1>
        <div class="section">
            <h2>Encender/Apagar</h2>
            <button class="btn" id="toggleBtn">Encender</button>
        </div>
        <div class="section" id="colorPickerSection">
            <h2>Seleccionar Color</h2>
            <div id="colorPicker">
                <div class="color-box" data-color="#ff0000" style="background-color: #ff0000;"></div>
                <div class="color-box" data-color="#00ff00" style="background-color: #00ff00;"></div>
                <div class="color-box" data-color="#0000ff" style="background-color: #0000ff;"></div>
                <div class="color-box" data-color="#ffff00" style="background-color: #ffff00;"></div>
                <div class="color-box" data-color="#ff00ff" style="background-color: #ff00ff;"></div>
                <div class="color-box" data-color="#00ffff" style="background-color: #00ffff;"></div>
                <div class="color-box" data-color="#800000" style="background-color: #800000;"></div>
                <div class="color-box" data-color="#808000" style="background-color: #808000;"></div>
                <div class="color-box" data-color="#008080" style="background-color: #008080;"></div>
                <div class="color-box" data-color="#800080" style="background-color: #800080;"></div>
                <div class="color-box" data-color="#ffa500" style="background-color: #ffa500;"></div>
                <div class="color-box" data-color="#add8e6" style="background-color: #add8e6;"></div>
                <div class="color-box" data-color="#ffc0cb" style="background-color: #ffc0cb;"></div>
                <div class="color-box" data-color="#000080" style="background-color: #000080;"></div>
            </div>
        </div>
        <div class="section">
            <h2>Brillo</h2>
            <input type="range" id="brightness" min="0" max="255" value="100">
        </div>
        <div class="section">
            <h2>Modos</h2>
            <button class="btn mode-btn" data-mode="STATIC_COLOR">Color Estático</button>
            <button class="btn mode-btn" data-mode="RAINBOW">Arcoíris</button>
            <button class="btn mode-btn" data-mode="BLINK">Parpadeo</button>
            <button class="btn mode-btn" data-mode="RAINBOW_BLINK">Parpadeo Arcoíris</button>
            <button class="btn mode-btn" data-mode="WAVE">Ola</button>
            <button class="btn mode-btn" data-mode="BREATHING">Respiración</button>
            <button class="btn mode-btn" data-mode="FIREWORKS">Fuegos Artificiales</button>
        </div>
        <div class="section" id="waveControls">
            <h2>Control de Ola</h2>
            <button class="btn" id="toggleDirectionBtn">Cambiar Dirección</button>
            <button class="btn" id="pauseWaveBtn">Pausar/Reanudar Ola</button>
        </div>
        <div class="section" id="speedSection">
            <h2>Velocidad</h2>
            <input type="range" id="speed" min="50" max="1000" value="500">
        </div>
    </div>
    <script>
        const brightnessValues = [0, 36, 72, 109, 145, 182, 218, 255];
        const speedValues = [50, 188, 325, 463, 600, 738, 875, 1000];

        function closestValue(value, valuesArray) {
            return valuesArray.reduce((prev, curr) => Math.abs(curr - value) < Math.abs(prev - value) ? curr : prev);
        }

        document.getElementById('toggleBtn').addEventListener('click', function () {
            fetch('/toggle').then(response => response.text()).then(data => {
                this.textContent = (this.textContent === 'Encender') ? 'Apagar' : 'Encender';
            });
        });

        document.querySelectorAll('.color-box').forEach(box => {
            box.addEventListener('click', function () {
                fetch(`/color?value=${encodeURIComponent(this.dataset.color)}`);
            });
        });

        document.getElementById('brightness').addEventListener('change', function () {
            let closest = closestValue(this.value, brightnessValues);
            this.value = closest;
            fetch(`/brightness?value=${closest}`);
        });

        const speedSection = document.getElementById('speedSection');
        const waveControls = document.getElementById('waveControls');
        const colorPickerSection = document.getElementById('colorPickerSection');

        document.querySelectorAll('.btn.mode-btn').forEach(button => {
            button.addEventListener('click', function () {
                fetch(`/mode?mode=${this.dataset.mode}`).then(response => response.text()).then(data => {
                    // Show or hide speed control based on mode
                    if (["BLINK", "RAINBOW_BLINK", "WAVE"].includes(this.dataset.mode)) {
                        speedSection.style.display = 'block';
                    } else {
                        speedSection.style.display = 'none';
                    }

                    // Show or hide wave controls based on mode
                    if (this.dataset.mode === "WAVE") {
                        waveControls.style.display = 'block';
                    } else {
                        waveControls.style.display = 'none';
                    }

                    // Show or hide color picker based on mode
                    if (["RAINBOW", "RAINBOW_BLINK", "FIREWORKS"].includes(this.dataset.mode)) {
                        colorPickerSection.style.display = 'none';
                    } else {
                        colorPickerSection.style.display = 'block';
                    }
                });
            });
        });

        document.getElementById('toggleDirectionBtn').addEventListener('click', function () {
            fetch('/direction');
        });

        document.getElementById('pauseWaveBtn').addEventListener('click', function () {
            fetch('/waveControl');
        });

        document.getElementById('speed').addEventListener('change', function () {
            let closest = closestValue(this.value, speedValues);
            this.value = closest;
            fetch(`/speed?value=${closest}`);
        });
    </script>
</body>
</html>

)=====";





void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleToggle() {
  isOn = !isOn;
  server.send(200, "text/plain", isOn ? "Encendido" : "Apagado");
}

void handleColor() {
  if (server.hasArg("value")) {
    selectedColor = server.arg("value");
    server.send(200, "text/plain", "Color cambiado");
  } else {
    server.send(400, "text/plain", "Falta el valor del color");
  }
}

void handleBrightness() {
  if (server.hasArg("value")) {
    brightness = server.arg("value").toInt();
    server.send(200, "text/plain", "Brillo cambiado");
  } else {
    server.send(400, "text/plain", "Falta el valor del brillo");
  }
}

void handleMode() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode == "STATIC_COLOR") currentMode = STATIC_COLOR;
    else if (mode == "RAINBOW") currentMode = RAINBOW;
    else if (mode == "BLINK") currentMode = BLINK;
    else if (mode == "RAINBOW_BLINK") currentMode = RAINBOW_BLINK;
    else if (mode == "WAVE") currentMode = WAVE;
    else if (mode == "BREATHING") currentMode = BREATHING;
    else if (mode == "FIREWORKS") currentMode = FIREWORKS;
  }
  server.send(200, "text/plain", "Modo cambiado");
}

void handleDirection() {
  waveDirectionRight = !waveDirectionRight;
  server.send(200, "text/plain", "Dirección de la ola cambiada");
}

void handleWaveControl() {
  wavePaused = !wavePaused;
  server.send(200, "text/plain", wavePaused ? "Ola pausada" : "Ola reanudada");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    blinkSpeed = server.arg("value").toInt();
    waveSpeed = server.arg("value").toInt();
    server.send(200, "text/plain", "Velocidad cambiada");
  } else {
    server.send(400, "text/plain", "Falta el valor de la velocidad");
  }
}

