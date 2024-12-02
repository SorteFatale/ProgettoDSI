#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// ------ CONFIGURAZIONI E VARIABILI ------

// Configurazione Wi-Fi
const char* ssid = "Chihiro Fushimi";          // Sostituisci con il nome della tua rete Wi-Fi
const char* password = "giuv1911";            // Sostituisci con la password della tua rete Wi-Fi

// Numero totale di posti disponibili
#define POSTI_TOTALI 10
int posti_liberi = POSTI_TOTALI;

// Variabili per il controllo del LED
String ledState = "OFF";
//const int LED_BUILTIN = 2; // Pin del LED integrato sulla ESP32

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

// Route per gestire richieste POST al parcheggio
void handlePost() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\":\"Bad Request\"}");
    return;
  }

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

  int x = doc["x"]; // x=1 per ingresso, x=0 per uscita
  StaticJsonDocument<100> response;
  int ack = 0;

  if (x == 1) { // Ingresso
    if (posti_liberi > 0) {
      posti_liberi--;
      ack = 1;
      Serial.println("Ingresso consentito. Posti liberi: " + String(posti_liberi));
    } else {
      Serial.println("Posti esauriti. Ingresso negato.");
    }
  } else if (x == 0) { // Uscita
    if (posti_liberi < POSTI_TOTALI) {
      posti_liberi++;
      ack = 1;
      Serial.println("Uscita consentita. Posti liberi: " + String(posti_liberi));
    } else {
      Serial.println("Nessuna macchina in uscita. Uscita negata.");
    }
  }

  aggiornaLCD();

  response["ack"] = ack;
  String responseBody;
  serializeJson(response, responseBody);
  server.send(200, "application/json", responseBody);
}

// Route per richieste GET (monitoraggio posti)
void handleGet() {
  StaticJsonDocument<100> response;
  response["posti_liberi"] = posti_liberi;

  String responseBody;
  serializeJson(response, responseBody);
  server.send(200, "application/json", responseBody);
}



void handleWebPage() {
  String html = "<!DOCTYPE html><html>";
html += "<head>";
html += "<style>";
html += "html, body {";
html += "    margin: 0;";
html += "    padding: 0;";
html += "    min-height: 100vh;";
html += "    font-family: 'Roboto', Arial, sans-serif;";
html += "    color: #333;";
html += "    display: flex;";
html += "    justify-content: center;";
html += "    align-items: center;";
html += "    text-align: center;";
html += "    --s: 100px;";
html += "    --c1: #4ecdc4;";
html += "    --c2: #556270;";
html += "    --c3: #d9d9d9;";
html += "    background: conic-gradient(from -60deg at 50% calc(100%/3), var(--c3) 0 120deg, #0000 0),";
html += "                conic-gradient(from 120deg at 50% calc(200%/3), var(--c3) 0 120deg, #0000 0),";
html += "                conic-gradient(from 60deg at calc(200%/3), var(--c3) 60deg, var(--c2) 0 120deg, #0000 0),";
html += "                conic-gradient(from 180deg at calc(100%/3), var(--c1) 60deg, var(--c3) 0 120deg, #0000 0),";
html += "                linear-gradient(90deg, var(--c1) calc(100%/6), var(--c2) 0 50%, var(--c1) 0 calc(500%/6), var(--c2) 0);";
html += "    background-size: calc(1.732*var(--s)) var(--s);";
html += "    background-attachment: fixed;";
html += "}";
html += "body {";
html += "    display: flex;";
html += "    flex-direction: column;";
html += "    gap: 20px;";
html += "    padding: 20px;";
html += "}";
html += ".container {";
html += "    max-width: 600px;";
html += "    width: 100%;";
html += "    background: #ffffff;";
html += "    border-radius: 15px;";
html += "    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);";
html += "    padding: 20px;";
html += "}";
html += "h1 {";
html += "    margin: 0;";
html += "    font-size: 24px;";
html += "    color: #444;";
html += "}";
html += ".section {";
html += "    margin: 20px 0;";
html += "    padding: 15px;";
html += "    border: 1px solid #ddd;";
html += "    border-radius: 10px;";
html += "    background-color: #fafafa;";
html += "}";
html += "input[type='text'] {";
html += "    width: calc(100% - 40px);";
html += "    padding: 10px;";
html += "    font-size: 16px;";
html += "    margin-top: 10px;";
html += "    margin-bottom: 15px;";
html += "    border: 1px solid #ddd;";
html += "    border-radius: 5px;";
html += "}";
html += "button {";
html += "    padding: 10px 20px;";
html += "    font-size: 16px;";
html += "    color: white;";
html += "    background-color: #56c8c8;";
html += "    border: none;";
html += "    border-radius: 5px;";
html += "    cursor: pointer;";
html += "    transition: background-color 0.3s ease;";
html += "}";
html += "button:hover {";
html += "    background-color: #45a3a3;";
html += "}";
html += ".confirmation {";
html += "    margin-top: 10px;";
html += "    color: #4caf50;";
html += "    font-size: 14px;";
html += "}";
html += ".text {";
html += "    font-size: 18px;";
html += "    color: #555;";
html += "}";
html += "strong {";
html += "    color: #333;";
html += "}";
html += "</style>";
html += "</head><body>";
html += "<div class='container'>";
html += "<h1>DI.P.S Admin Server</h1>";
html += "<div class='section'>";
html += "<p class='text' id='pos'>Posti disponibili adesso: <strong id='postiCount'>0</strong></p>";
html += "<input id='posti_update' type='text' placeholder='Aggiorna il numero di posti disponibili'>";
html += "<button id='bottone'>Invia</button>";
html += "<p id='confirmation' class='confirmation' style='display: none;'>Numero di posti aggiornato con successo!</p>";
html += "</div>";
html += "</div>";
html += "<script>";
html += "const postiCount = document.getElementById('postiCount');";
html += "const postiAdmin = document.getElementById('posti_update');";
html += "const bottone = document.getElementById('bottone');";
html += "const confirmation = document.getElementById('confirmation');";
html += "bottone.onclick = function() {";
html += "    const placesAdmin = postiAdmin.value.trim();";
html += "    if (placesAdmin) {";
html += "        const xhr = new XMLHttpRequest();";
html += "        xhr.open('POST', 'http://192.168.182.89/change_places', true);";
html += "        xhr.setRequestHeader('Content-Type', 'text/plain');";
html += "        xhr.onload = function() {";
html += "            if (xhr.status === 200) {";
html += "                confirmation.style.display = 'block';";
html += "                postiAdmin.value = '';";
html += "                setTimeout(() => {";
html += "                    confirmation.style.display = 'none';";
html += "                }, 3000);";
html += "            }";
html += "        };";
html += "        xhr.send(placesAdmin);";
html += "    }";
html += "};";
html += "setInterval(function() {";
html += "    const xhr = new XMLHttpRequest();";
html += "    xhr.open('GET', 'http://192.168.182.89/places', true);";
html += "    xhr.onload = function() {";
html += "        if (xhr.status === 200) {";
html += "            postiCount.textContent = xhr.responseText;";
html += "        }";
html += "    };";
html += "    xhr.send();";
html += "}, 3000);";
html += "</script>";
html += "</body></html>";
  server.send(200, "text/html", html);
}
// Route per accendere il LED
void handleLedOn() {
  digitalWrite(LED_BUILTIN, HIGH);
  ledState = "ON";
  handleWebPage();
}

void handlePlaces(){
  server.send(200, "text", String(posti_liberi));

}

// Route per spegnere il LED
void handleLedOff() {
  digitalWrite(LED_BUILTIN, LOW);
  ledState = "OFF";
  handleWebPage();
}

void handleChangePlaces(){
  //mi prendo il nuovo numero di posti dal payload della risposta
  String body = server.arg("plain");
  Serial.println("Richiesta ricevuta: " + body);
  posti_liberi = atoi(body.c_str());
  aggiornaLCD();
  server.send(200,"","");
}

// Configura le route del server
void setupServerRoutes() {

  server.on("/", HTTP_GET, handleWebPage);       // Web page per il controllo LED
  server.on("/places", HTTP_GET, handlePlaces);  //Ritorna i posti disponibili
  server.on("/led/on", HTTP_GET, handleLedOn);   // Accende il LED
  server.on("/led/off", HTTP_GET, handleLedOff); // Spegne il LED
  server.on("/parking", HTTP_POST, handlePost);  // Gestisce ingressi/uscite parcheggio
  server.on("/status", HTTP_GET, handleGet);     // Stato dei posti liberi
  server.on("/change_places",HTTP_POST,handleChangePlaces); //Aggiornamento posti da parte dell'admin
  server.begin();
  Serial.println("Server HTTP avviato");
}

// ------ SETUP E LOOP ------

void setup() {
  Serial.begin(115200);

  // Configurazione Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connessione alla rete Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnesso al Wi-Fi!");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  // Configurazione LCD e LED
  initLCD();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Configura le route del server
  setupServerRoutes();
}

void loop() {
  server.handleClient();
}