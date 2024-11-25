#include <WiFi.h> // Libreria per la comunicazione Wi-Fi
#include <HTTPClient.h> // Libreria per inviare richieste HTTP
#include <ESP32Servo.h> // Libreria per il servomotore

#define PIN_SG90 16 // Pin output servomotore
#define pinInput 5  // Pin di input per il photo-interrupter

// Oggetto servomotore
Servo sg90;

// Variabile di stato per capire se ho già inviato il messaggio al server
int valido = 0;

// Configurazione Wi-Fi
const char* ssid = "Vodafone-E96273651";          // Sostituisci con il nome della tua rete Wi-Fi
const char* password = "GattoCane8@2020";  // Sostituisci con la password della tua rete Wi-Fi

// URL del server centrale
const char* serverURL = "http://192.168.1.39/parking"; // Sostituisci con l'indirizzo del tuo server

// Funzione per alzare e abbassare la sbarra
void activate_parking_bar() {
  // Questo for alza la sbarra
  for (int pos = 90; pos <= 180; pos += 1) {
    sg90.write(pos);
    delay(10);
  }
  delay(2000);
  // Questo for la abbassa
  for (int pos = 180; pos >= 90; pos -= 1) {
    sg90.write(pos);
    delay(10);
  }
}

// Funzione per inviare i dati al server
void send_data_to_server(int id, int x) {
  HTTPClient http;

  // Specifica l'URL del server
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json"); // Header per la richiesta HTTP

  // Crea il payload JSON
  String payload = "{\"id\": " + String(id) + ", \"x\": " + String(x) + "}";

  // Invia la richiesta POST
  int httpResponseCode = http.POST(payload);

  // Controlla la risposta del server
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Risposta del server: " + response);

    // Se il server risponde con ACK=1, alza la sbarra
    if (response.indexOf("\"ack\":1") != -1) {
      Serial.println("LA SBARRA SI ALZA");
      activate_parking_bar();
    } else {
      Serial.println("PARCHEGGIO PIENO!");
    }
  } else {
    Serial.println("Errore durante la richiesta HTTP");
  }

  // Chiude la connessione HTTP
  http.end();
}

// Setup del servomotore
void setup_servo() {
  sg90.setPeriodHertz(50); // Frequenza della PWM per il servomotore SG90
  sg90.attach(PIN_SG90, 500, 2400); // Durata minima e massima dell'impulso: 500-2400
  sg90.write(90); // Settiamo la sbarra a 90°, in modo da bloccare inizialmente l'accesso al parcheggio
}

void setup() {
  // Inizializza il monitor seriale
  Serial.begin(115200);

  // Configurazione iniziale del servomotore
  setup_servo();

  pinMode(pinInput, INPUT); // Configurazione del pin del photo-interrupter come pin di input

  // Connessione alla rete Wi-Fi
  Serial.println("Connessione alla rete Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connessione in corso...");
  }

  Serial.println("Connesso alla rete Wi-Fi!");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(200);
  int valueState = digitalRead(pinInput); // Legge il valore digitale dal pin del photo-interrupter
  Serial.println(valueState);

  int id = 1; // ID univoco per questa scheda
  int x = 1;  // Questa scheda invierà sempre 1 perché gestirà solo gli ingressi al parcheggio

  if (valido == 0 && valueState==HIGH) {
    // Invia i dati al server tramite richiesta HTTP
    send_data_to_server(id, x);
    valido = 1; // Salvo nella variabile di stato il fatto che ho inviato il messaggio
  }

  if (valido == 1 && valueState==LOW) {
    // Ripristino il valore della variabile di stato
    valido = 0;
  }
}