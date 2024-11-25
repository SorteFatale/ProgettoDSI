#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// ------ VARIABILI E CONFIGURAZIONI ------

// Configurazione Wi-Fi
const char* ssid = "Vodafone-E96273651";          // Sostituisci con il nome della tua rete Wi-Fi
const char* password = "GattoCane8@2020";  // Sostituisci con la password della tua rete Wi-Fi

// Numero totale di posti disponibili
#define POSTI_TOTALI 10
int posti_liberi = POSTI_TOTALI;

// Oggetto LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Server HTTP su porta 80
WebServer server(80);

// ------ FUNZIONI ------

// Inizializza lo schermo LCD
void initLCD() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Posti liberi:");
  lcd.setCursor(0, 1);
  lcd.print(posti_liberi);
}

// Aggiorna lo schermo LCD con i posti disponibili
void aggiornaLCD() {
  lcd.clear();
  if (posti_liberi > 0) {
    lcd.setCursor(0, 0);
    lcd.print("Posti liberi:");
    lcd.setCursor(0, 1);
    lcd.print(posti_liberi);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Posti finiti!");
  }
}

// Gestisce le richieste POST
void handlePost() {
  // Controlla se il payload è presente
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\":\"Bad Request\"}");
    return;
  }

  // Legge il payload della richiesta
  String body = server.arg("plain");
  Serial.println("Richiesta ricevuta: " + body);

  // Parsing del payload JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    Serial.println("Errore nel parsing del JSON");
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Estrae i dati dal JSON
  int id = doc["id"];
  int x = doc["x"]; // x=1 per ingresso, x=0 per uscita

  // Prepara la risposta JSON
  StaticJsonDocument<100> response;
  int ack = 0; // Di default, la sbarra non si alza

  if (x == 1) { // Ingresso
    if (posti_liberi > 0) {
      posti_liberi--;
      ack = 1; // Sbarra si alza
      Serial.println("Ingresso consentito. Posti liberi: " + String(posti_liberi));
    } else {
      Serial.println("Posti esauriti. Ingresso negato.");
    }
  } else if (x == 0) { // Uscita
    if (posti_liberi < POSTI_TOTALI) {
      posti_liberi++;
      ack = 1; // Sbarra si alza per uscita
      Serial.println("Uscita consentita. Posti liberi: " + String(posti_liberi));
    } else {
      Serial.println("Nessuna macchina in uscita. Uscita negata.");
    }
  }

  // Aggiorna l'LCD con i posti disponibili
  aggiornaLCD();

  // Configura la risposta JSON
  response["ack"] = ack;
  String responseBody;
  serializeJson(response, responseBody);

  // Invia la risposta al client
  server.send(200, "application/json", responseBody);
}

// Gestisce le richieste GET (opzionale, per debug o monitoraggio)
void handleGet() {
  // Prepara una risposta JSON con lo stato del parcheggio
  StaticJsonDocument<100> response;
  response["posti_liberi"] = posti_liberi;

  String responseBody;
  serializeJson(response, responseBody);

  server.send(200, "application/json", responseBody);
}

// Configura le route del server
void setupServerRoutes() {
  server.on("/", HTTP_GET, handleGet);   // Route GET per monitoraggio
  server.on("/parking", HTTP_POST, handlePost); // Route POST per gestire ingressi/uscite
  server.begin();
  Serial.println("Server HTTP avviato");
}

void setup() {
  // Inizializza la seriale
  Serial.begin(115200);

  // Connessione alla rete Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connessione alla rete Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnesso al Wi-Fi!");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  // Inizializza lo schermo LCD
  initLCD();

  // Configura il server HTTP
  setupServerRoutes();
}

void loop() {
  // Gestisce le richieste HTTP
  server.handleClient();
}