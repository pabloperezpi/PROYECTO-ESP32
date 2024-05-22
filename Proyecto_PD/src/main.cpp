#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h> // controlar tiras led NeoPixel

#define NUM_LEDS 50 // Número de LEDs en la tira LED
#define MIC_PIN A0   // Pin del micrófono
#define LED_PIN 27  // Pin de la tira LED

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800); // Creamos el objeto de la tira LED "strip"

const char* ssid = "iPhone de Pablo";
const char* password = "pabloperez";

WebServer server(80);

void handleToggle();
void handleColor();
void handleRoot();

// Variables para controlar el estado de las luces LED
bool isOn = false;
String selectedColor = "#ff0000"; // Color por defecto

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  strip.begin();
  strip.show(); // Mostramos el estado actual de la tira

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

  server.begin();

  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();

  if (isOn) {
    // Leer los niveles de decibelios del micrófono
    int dB = analogRead(MIC_PIN);

    // Ajustar la intensidad de la tira LED basada en los niveles de decibelios
    int brightness = map(dB, 0, 1023, 0, 255); // map(valor, valorMinimoEntrada, valorMaximoEntrada, valorMinimoSalida, valorMaximoSalida);
    strip.setBrightness(brightness);

    // Aplicar el color seleccionado
    uint32_t color = parseColor(selectedColor);
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, color);
    }

    // Actualizar la tira LED
    strip.show();
  } else {
    strip.clear();
    strip.show();
  }

  delay(100); // Ajusta el intervalo según sea necesario para tu aplicación
}

uint32_t parseColor(String color) {
  color.remove(0, 1); // Quitar el '#'
  long number = strtol(color.c_str(), NULL, 16);
  return strip.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
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
            <h2>Paleta de colores</h2>
            <div class="color-palette">
                <div class="color-box" style="background-color: #ff0000;"></div>
                <div class="color-box" style="background-color: #ff8000;"></div>
                <div class="color-box" style="background-color: #ffff00;"></div>
                <div class="color-box" style="background-color: #80ff00;"></div>
                <div class="color-box" style="background-color: #00ff00;"></div>
                <div class="color-box" style="background-color: #00ff80;"></div>
                <div class="color-box" style="background-color: #00ffff;"></div>
                <div class="color-box" style="background-color: #0080ff;"></div>
                <div class="color-box" style="background-color: #0000ff;"></div>
                <div class="color-box" style="background-color: #8000ff;"></div>
                <div class="color-box" style="background-color: #ff00ff;"></div>
                <div class="color-box" style="background-color: #ff0080;"></div>
                <!-- Agrega más colores según sea necesario -->
            </div>
        </div>
    </div>

    <script>
        let isOn = false;
        const toggleButton = document.getElementById('toggleButton');
        const colorBoxes = document.querySelectorAll('.color-box');

        toggleButton.addEventListener('click', () => {
            isOn = !isOn;
            sendToggleRequest(isOn);
            toggleButton.textContent = isOn ? 'Apagar' : 'Encender';
        });

        colorBoxes.forEach(box => {
            box.addEventListener('click', () => {
                const color = box.style.backgroundColor;
                sendColorRequest(color);
            });
        });

        function sendToggleRequest(state) {
            const url = '/toggle?state=' + (state ? 'on' : 'off');
            fetch(url)
                .then(response => response.json())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }

        function sendColorRequest(color) {
            const url = '/color?value=' + encodeURIComponent(color);
            fetch(url)
                .then(response => response.json())
                .then(data => console.log(data))
                .catch(error => console.error('Error:', error));
        }
    </script>
</body>
</html>
)=====";

// Manejar la ruta raíz (/)
void handleRoot() {
  server.send(200, "text/html", HTML);
}

// Manejar la solicitud para encender o apagar las luces LED
void handleToggle() {
  String state = server.arg("state");
  if (state == "on") {
    isOn = true;
    Serial.println("Luces LED encendidas");
  } else if (state == "off") {
    isOn = false;
    Serial.println("Luces LED apagadas");
  }
  server.send(200, "application/json", "{\"message\":\"Toggle command received\"}");
}

// Manejar la solicitud para cambiar el color de las luces LED
void handleColor() {
  String colorValue = server.arg("value");
  selectedColor = colorValue;
  Serial.print("Color seleccionado: ");
  Serial.println(selectedColor);
  server.send(200, "application/json", "{\"message\":\"Color command received\"}");
}


