#include <WiFi.h> 
#include <ESPAsyncWebServer.h> // crear seridor web en la ESP32
#include <Adafruit_NeoPixel.h> // controlar tiras led NeoPixel

#define NUM_LEDS 12 // Número de LEDs tira LED
#define MIC_PIN A0   // Pin  micrófono
#define LED_PIN 2    // Pin  tira LED

const char *ssid = "MiFibra-5EC3-5G";
const char *password = "hi3fdJzR";

AsyncWebServer server(80);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800); // creamos el objeto de tira led "strip"

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  strip.begin();
  strip.show(); // mostramos el estado actual de la tira

  // Conectar a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a la red WiFi");

  // Rutas de la API web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "¡Conexión exitosa con el ESP32!");
  });

  // Definir ruta para cambiar el color de la tira LED
  server.on("/color", HTTP_POST, [](AsyncWebServerRequest *request){
    // Leer el cuerpo de la solicitud POST
    String body = request->arg("plain");
    request->send(200, "application/json", "{\"message\":\"Color cambiado\"}");
  });

  // Iniciar servidor web
  server.begin();
}

void loop() {
  // Leer los niveles de decibelios del micrófono
  int dB = analogRead(MIC_PIN);

  // Ajustar la intensidad de la tira LED basada en los niveles de decibelios
  int brightness = map(dB, 0, 1023, 0, 255); // map(valor, valorMinimoEntrada, valorMaximoEntrada, valorMinimoSalida, valorMaximoSalida);
  strip.setBrightness(brightness);

  // Actualizar la tira LED
  strip.show();

  delay(100); // Ajusta el intervalo según sea necesario para tu aplicación
}
