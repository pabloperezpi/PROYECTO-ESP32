#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 144
#define LED_PIN 14
#define MICROPHONE_PIN 34
#define NUM_SAMPLES 10

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "iPhone de AyalaKT";
const char* password = "12345678";

WebServer server(80);

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

enum LedMode { OFF, STATIC_COLOR, RAINBOW, BLINK, RAINBOW_BLINK };
LedMode currentMode = OFF;

bool isOn = false;
bool microphoneActive = false;
String selectedColor = "#ff0000";
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
  server.on("/microphone", HTTP_GET, handleMicrophone);
  server.on("/micValue", HTTP_GET, handleMicValue);

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

  for (int i = 0; i < smoothingSamples; i++) {
    micValues[i] = noiseBase;
    total += micValues[i];
  }
}

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
            background-color: #007bff;
            color: #fff;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }

        .btn:hover {
            background-color: #0056b3;
        }

        .color-palette {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 5px;
            margin-top: 20px;
        }

        .color-box {
            width: 50px;
            height: 50px;
            border-radius: 50%;
            cursor: pointer;
        }

        .color-box.rainbow {
            background: linear-gradient(45deg, red, orange, yellow, green, blue, indigo, violet);
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
    <div class="container">
        <h1>Control de luces LED</h1>
        <div>
            <h2>Encender / Apagar</h2>
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
                <div class="color-box" style="background-color: #FF5733;" onclick="setColor('#FF5733')"></div>
                <div class="color-box" style="background-color: #FFC300;" onclick="setColor('#FFC300')"></div>
                <div class="color-box" style="background-color: #C70039;" onclick="setColor('#C70039')"></div>
                <div class="color-box" style="background-color: #900C3F;" onclick="setColor('#900C3F')"></div>
                <div class="color-box" style="background-color: #581845;" onclick="setColor('#581845')"></div>
                <!-- Aquí van más cajas de colores -->
            </div>
        </div>
        <div class="brightness-slider">
            <h2>Brillo</h2>
            <input type="range" min="1" max="255" value="11" class="slider" id="brightnessSlider">
        </div>
        <div>
            <h2>Micrófono</h2>
            <button id="microphoneButton" class="btn">Activar Micrófono</button>
            <p id="micValue">Valor del Micrófono: </p>
        </div>
    </div>
    <script>
    const toggleButton = document.getElementById('toggleButton');
    const fixedModeButton = document.getElementById('fixedModeButton');
    const blinkModeButton = document.getElementById('blinkModeButton');
    const rainbowModeButton = document.getElementById('rainbowModeButton');
    const rainbowBlinkModeButton = document.getElementById('rainbowBlinkModeButton');
    const brightnessSlider = document.getElementById('brightnessSlider');
    const microphoneButton = document.getElementById('microphoneButton');
    const micValueDisplay = document.getElementById('micValue');

    toggleButton.addEventListener('click', () => {
        const isOn = toggleButton.innerText === 'Encender';
        fetch(`/toggle?state=${isOn ? 'on' : 'off'}`)
            .then(response => response.json())
            .then(data => {
                toggleButton.innerText = isOn ? 'Apagar' : 'Encender';
            })
            .catch(error => console.error('Error:', error));
    });

    fixedModeButton.addEventListener('click', () => {
        fetch('/mode?value=fixed')
            .then(response => response.json())
            .catch(error => console.error('Error:', error));
    });

    blinkModeButton.addEventListener('click', () => {
        fetch('/mode?value=blink')
            .then(response => response.json())
            .catch(error => console.error('Error:', error));
    });

    rainbowModeButton.addEventListener('click', () => {
        fetch('/mode?value=rainbow')
            .then(response => response.json())
            .catch(error => console.error('Error:', error));
    });

    rainbowBlinkModeButton.addEventListener('click', () => {
        fetch('/mode?value=rainbow_blink')
            .then(response => response.json())
            .catch(error => console.error('Error:', error));
    });

    brightnessSlider.addEventListener('input', () => {
        const brightness = brightnessSlider.value;
        fetch(`/brightness?value=${brightness}`)
            .then(response => response.json())
            .catch(error => console.error('Error:', error));
    });

    microphoneButton.addEventListener('click', () => {
        const isActive = microphoneButton.innerText === 'Activar Micrófono';
        fetch(`/microphone?state=${isActive ? 'on' : 'off'}`)
            .then(response => response.json())
            .then(data => {
                microphoneButton.innerText = isActive ? 'Desactivar Micrófono' : 'Activar Micrófono';
                updateMicValue();
            })
            .catch(error => console.error('Error:', error));
    });

    function setColor(color) {
        fetch(`/color?value=${encodeURIComponent(color)}`)
            .then(response => response.json())
            .catch(error => console.error('Error:', error));
    }

    function updateMicValue() {
        if (microphoneButton.innerText === 'Desactivar Micrófono') {
            setInterval(() => {
                fetch('/micValue')
                    .then(response => response.json())
                    .then(data => {
                        micValueDisplay.innerText = `Valor del Micrófono: ${data.value}`;
                    })
                    .catch(error => console.error('Error:', error));
            }, 1000);
        }
    }
    </script>
</body>
</html>
)=====";

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleToggle() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    Serial.print("Toggle state: ");
    Serial.println(state);
    if (state == "on") {
      isOn = true;
    } else if (state == "off") {
      isOn = false;
    }
    server.send(200, "application/json", "{\"message\":\"Toggle command received\"}");
  } else {
    Serial.println("Error: 'state' parameter missing");
    server.send(400, "application/json", "{\"error\":\"Missing 'state' parameter\"}");
  }
}

void handleColor() {
  if (server.hasArg("value")) {
    selectedColor = server.arg("value");
    Serial.print("Color seleccionado: ");
    Serial.println(selectedColor);
    server.send(200, "application/json", "{\"message\":\"Color command received\"}");
  } else {
    Serial.println("Error: 'value' parameter missing");
    server.send(400, "application/json", "{\"error\":\"Missing 'value' parameter\"}");
  }
}

void handleBrightness() {
  if (server.hasArg("value")) {
    brightness = server.arg("value").toInt();
    Serial.print("Brightness set to: ");
    Serial.println(brightness);
    server.send(200, "application/json", "{\"message\":\"Brightness command received\"}");
  } else {
    Serial.println("Error: 'value' parameter missing");
    server.send(400, "application/json", "{\"error\":\"Missing 'value' parameter\"}");
  }
}

void handleMode() {
  if (server.hasArg("value")) {
    String modeValue = server.arg("value");
    Serial.print("Mode value: ");
    Serial.println(modeValue);
    if (modeValue == "fixed") {
      currentMode = STATIC_COLOR;
    } else if (modeValue == "blink") {
      currentMode = BLINK;
    } else if (modeValue == "rainbow") {
      currentMode = RAINBOW;
    } else if (modeValue == "rainbow_blink") {
      currentMode = RAINBOW_BLINK;
    }
    server.send(200, "application/json", "{\"message\":\"Mode command received\"}");
  } else {
    Serial.println("Error: 'value' parameter missing");
    server.send(400, "application/json", "{\"error\":\"Missing 'value' parameter\"}");
  }
}

void handleMicrophone() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    Serial.print("Microphone state: ");
    Serial.println(state);
    if (state == "on") {
      microphoneActive = true;
    } else if (state == "off") {
      microphoneActive = false;
    }
    server.send(200, "application/json", "{\"message\":\"Microphone command received\"}");
  } else {
    Serial.println("Error: 'state' parameter missing");
    server.send(400, "application/json", "{\"error\":\"Missing 'state' parameter\"}");
  }
}

void handleMicValue() {
  int adjustedMicValue = getSmoothedMicValue();
  String json = "{\"value\":" + String(adjustedMicValue) + "}";
  server.send(200, "application/json", json);
}


