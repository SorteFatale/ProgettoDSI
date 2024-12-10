#include <WiFi.h> // Libreria per la comunicazione Wi-Fi
#include <HTTPClient.h> // Libreria per inviare richieste HTTP
#include <ESP32Servo.h> // Libreria per il servomotore

#define PIN_SG90 16 // Pin output servomotore
#define pinInput 5  // Pin di input per il photo-interrupter
int x=0; //valore da cambiare in base alla scheda (0: uscita, 1: ingresso)

// Oggetto servomotore
Servo sg90;

// Variabile di stato per capire se ho già inviato il messaggio al server
int valido = 0;

// Configurazione Wi-Fi
const char* ssid = "Chihiro Fushimi";          // Sostituisci con il nome della tua rete Wi-Fi
const char* password = "giuv1911";  // Sostituisci con la password della tua rete Wi-Fi

// URL del server centrale
const char* serverURL = "http://192.168.11.89/parking"; // Sostituisci con l'indirizzo del tuo server

// Timeout massimo per la richiesta HTTP (in millisecondi)
const unsigned long httpTimeout = 5000;

// Funzione per alzare e abbassare la sbarra
void activate_parking_bar() {
  // Questo for alza la sbarra
  for (int pos = 90; pos >= 0; pos -= 1) {
    sg90.write(pos);
    delay(10);
  }
  delay(2000);
  // Questo for la abbassa
  for (int pos = 0; pos <= 90; pos += 1) {
    sg90.write(pos);
    delay(10);
  }
}

// Funzione per inviare i dati al server con logica di timeout
void send_data_to_server(int id, int x) {
  HTTPClient http;

  // Specifica l'URL del server
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json"); // Header per la richiesta HTTP

  // Crea il payload JSON
  String payload = "{\"id\": " + String(id) + ", \"x\": " + String(x) + "}";

  // Avvia il timer per il timeout
  unsigned long startMillis = millis();

  // Invia la richiesta POST
  int httpResponseCode = http.POST(payload);

  // Controlla se la richiesta ha ricevuto una risposta entro il timeout
  if (millis() - startMillis > httpTimeout) {
    // Tempo massimo superato
    Serial.println("Timeout raggiunto! Nessuna risposta dal server.");
    if(x==0){
      // Questo for alza la sbarra
      Serial.println("SBARRA ALZATA");
      for (int pos = 90; pos >= 0; pos -= 1) {
        sg90.write(pos);
        delay(10);
      }
    }
    http.end(); //termino la connesione col server
    return; // Esci dalla funzione, nessuna risposta valida ricevuta
  }

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
    // Errore durante la richiesta HTTP
    Serial.println("Errore durante la richiesta HTTP: " + String(httpResponseCode));
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

  int id = 4; // ID univoco per questa scheda

  if (valido == 0 && valueState == HIGH) {
    // Invia i dati al server tramite richiesta HTTP
    send_data_to_server(id, x);
    valido = 1; // Salvo nella variabile di stato il fatto che ho inviato il messaggio
  }

  if (valido == 1 && valueState == LOW) {
    // Ripristino il valore della variabile di stato
    valido = 0;
  }
}