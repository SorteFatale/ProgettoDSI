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
  html += "html, body { margin: 5px; padding: 5px; min-height: 100vh; text-align: left; font-family: Arial; }";
  html += "body { background-color: #efefef; opacity: 1; background-image: repeating-radial-gradient(circle at 0 0, transparent 0, #efefef 26px), repeating-linear-gradient(#a1dfdc55, #a1dfdc); }";
  html += ".button { padding: 20px; font-size: 20px; border: none; border-radius: 30px; cursor: pointer; transition: background-color 0.3s ease; }";
  html += ".button:hover { filter: brightness(1.2); } .text {font-size= 20px; }"; // Per intensificare il colore
  html += "</style>";
  html += "</head><body>";
  html += "<h1>DI.P.S Admin Server</h1>";
  html += "<strong> <p class=\" text \" id=\"pos\">   Posti disponibili adesso: <strong>" + String(posti_liberi) + "</strong></p> </strong>";
  html += "<input id=\"posti_update\" type=\"text\" placeholder=\"Aggiorna il numero di posti disponibili:\">";
  html += "<button id=\"bottone\" value=\"Invia\"> Invia";
  html += "</button>";
  html += "<script>";
  html += "const posti = document.getElementById(\"pos\");";
  html += "const posti_admin = document.getElementById(\"posti_update\");";
  html += "const bottone = document.getElementById(\"bottone\");";
  html += "bottone.onclick = function(){";
  html += "var places_admin = posti_admin.value; ";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.open('POST', 'http://192.168.182.89/change_places',true);";
  html += "xhr.setRequestHeader(\"Content-Type\",\"text\");";
  html += "var params = escape(places_admin);";
  html += "xhr.send(params);";
  html += "};";
  html += "setInterval(function() {";
  html += "  const xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', 'http://192.168.182.89/places', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status === 200) {";
  html += "      posti.innerHTML = \" <strong> Posti disponibili adesso: </strong> \" + xhr.responseText; ";
  html += "    }";
  html += "  }; ";
  html += "  xhr.send();";
  html += "}, 3000);";
  html += "</script>";
  html += "<p class=\" text \">   <strong> LED State: " + ledState + "</strong></p>";
  html += "<p><a href=\"/led/on\"><button class=\"button\" style=\"background-color: #a1dfdc;\">Turn ON</button></a> ";
  html += "<a href=\"/led/off\"><button class=\"button\" style=\"background-color: #ee8b7b;\">Turn OFF</button></a></p>";
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