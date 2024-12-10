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
  String html = "";
html += "<!DOCTYPE html>\n";
html += "<html lang=\"en\">\n";
html += "<head>\n";
html += "    <meta charset=\"UTF-8\">\n";
html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
html += "    <title>DI.P.S Admin Server</title>\n";
html += "    <style>\n";
html += "        html, body {\n";
html += "            margin: 0;\n";
html += "            padding: 0;\n";
html += "            min-height: 100vh;\n";
html += "            font-family: 'Roboto', Arial, sans-serif;\n";
html += "            color: #333;\n";
html += "            display: flex;\n";
html += "            justify-content: center;\n";
html += "            align-items: center;\n";
html += "            text-align: center;\n";
html += "            --s: 100px;\n";
html += "            --c1: #4ecdc4;\n";
html += "            --c2: #556270;\n";
html += "            --c3: #d9d9d9;\n";
html += "            background: conic-gradient(from -60deg at 50% calc(100%/3), var(--c3) 0 120deg, #0000 0),\n";
html += "                        conic-gradient(from 120deg at 50% calc(200%/3), var(--c3) 0 120deg, #0000 0),\n";
html += "                        conic-gradient(from 60deg at calc(200%/3), var(--c3) 60deg, var(--c2) 0 120deg, #0000 0),\n";
html += "                        conic-gradient(from 180deg at calc(100%/3), var(--c1) 60deg, var(--c3) 0 120deg, #0000 0),\n";
html += "                        linear-gradient(90deg, var(--c1) calc(100%/6), var(--c2) 0 50%, var(--c1) 0 calc(500%/6), var(--c2) 0);\n";
html += "            background-size: calc(1.732*var(--s)) var(--s);\n";
html += "            background-attachment: fixed;\n";
html += "        }\n";
html += "        body {\n";
html += "            display: flex;\n";
html += "            flex-direction: column;\n";
html += "            gap: 20px;\n";
html += "            padding: 20px;\n";
html += "        }\n";
html += "        .container {\n";
html += "            max-width: 600px;\n";
html += "            width: 100%;\n";
html += "            background: #ffffff;\n";
html += "            border-radius: 15px;\n";
html += "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);\n";
html += "            padding: 20px;\n";
html += "        }\n";
html += "        h1 {\n";
html += "            margin: 0;\n";
html += "            font-size: 24px;\n";
html += "            color: #444;\n";
html += "        }\n";
html += "        .section {\n";
html += "            margin: 20px 0;\n";
html += "            padding: 15px;\n";
html += "            border: 1px solid #ddd;\n";
html += "            border-radius: 10px;\n";
html += "            background-color: #fafafa;\n";
html += "        }\n";
html += "        input[type='text'] {\n";
html += "            width: calc(100% - 40px);\n";
html += "            padding: 10px;\n";
html += "            font-size: 16px;\n";
html += "            margin-top: 10px;\n";
html += "            margin-bottom: 15px;\n";
html += "            border: 1px solid #ddd;\n";
html += "            border-radius: 5px;\n";
html += "        }\n";
html += "        button {\n";
html += "            padding: 10px 20px;\n";
html += "            font-size: 16px;\n";
html += "            color: white;\n";
html += "            background-color: #56c8c8;\n";
html += "            border: none;\n";
html += "            border-radius: 5px;\n";
html += "            cursor: pointer;\n";
html += "            transition: background-color 0.3s ease;\n";
html += "        }\n";
html += "        button:hover {\n";
html += "            background-color: #45a3a3;\n";
html += "        }\n";
html += "        .confirmation {\n";
html += "            margin-top: 10px;\n";
html += "            color: #4caf50;\n";
html += "            font-size: 14px;\n";
html += "        }\n";
html += "        .text {\n";
html += "            font-size: 18px;\n";
html += "            color: #555;\n";
html += "        }\n";
html += "        strong {\n";
html += "            color: #333;\n";
html += "        }\n";
html += "        .icon {\n";
html += "            width: 100px;\n";
html += "            height: 100px;\n";
html += "            margin: 10px auto;\n";
html += "        }\n";
html += "    </style>\n";
html += "</head>\n";
html += "<body>\n";
html += "    <div class=\"container\">\n";
html += "        <svg version=\"1.0\" xmlns=\"http://www.w3.org/2000/svg\"\n";
html += "            width=\"100\" height=\"100\" viewBox=\"0 0 189.000000 189.000000\"\n";
html += "            preserveAspectRatio=\"xMidYMid meet\" style=\"display: block; margin: 0 auto;\">\n";
html += "            <g transform=\"translate(0.000000,189.000000) scale(0.100000,-0.100000)\"\n";
html += "            fill=\"#000000\" stroke=\"none\">\n";
html += "                <path d=\"M1256 1767 c-44 -25 -76 -77 -76 -123 0 -27 -4 -32 -32 -39 -18 -3\n";
html += "                -60 -8 -93 -10 -69 -3 -124 -22 -210 -70 -86 -48 -95 -61 -107 -152 -5 -43\n";
html += "                -13 -100 -16 -128 l-7 -50 -145 -5 c-158 -5 -179 -12 -210 -63 -19 -30 -20\n";
html += "                -54 -20 -434 l0 -403 -88 0 c-116 0 -124 -8 -120 -111 l3 -74 290 0 290 0 3\n";
html += "                74 c4 103 -4 111 -120 111 l-88 0 0 348 c0 420 -13 386 142 378 139 -8 173 -8\n";
html += "                242 0 l58 6 -7 -34 c-13 -55 -8 -114 14 -178 19 -55 26 -90 18 -90 -2 0 -21 8\n";
html += "                -43 18 -21 10 -90 31 -153 47 -105 26 -115 27 -126 12 -7 -9 -34 -69 -60 -133\n";
html += "                -46 -112 -48 -117 -30 -130 41 -30 123 -2 173 59 12 14 24 8 116 -49 114 -72\n";
html += "                212 -104 267 -88 27 8 35 19 61 83 35 88 74 243 82 326 17 162 14 155 50 155\n";
html += "                66 0 66 2 66 -383 l0 -347 -88 0 c-116 0 -124 -8 -120 -111 l3 -74 290 0 290\n";
html += "                0 3 74 c4 103 -4 111 -120 111 l-88 0 0 400 c0 435 0 435 -56 479 -22 17 -41\n";
html += "                21 -104 21 l-76 0 18 88 c9 48 20 118 24 155 6 57 11 70 28 77 76 29 111 142\n";
html += "                64 209 -27 40 -81 71 -123 71 -16 0 -48 -10 -69 -23z m114 -42 c59 -30 68 -98\n";
html += "                21 -146 -78 -77 -200 14 -147 110 15 26 51 50 79 51 10 0 31 -7 47 -15z m-144\n";
html += "                -185 c19 -16 46 -33 59 -36 32 -8 32 -22 3 -186 l-23 -128 -108 0 c-92 0 -108\n";
html += "                2 -103 15 3 8 11 51 18 96 12 75 16 84 45 103 49 32 71 104 41 134 -10 10 -10\n";
html += "                15 2 22 23 15 30 12 66 -20z m-124 -7 c57 -51 -18 -115 -172 -146 l-65 -14 -5\n";
html += "                -59 c-3 -32 -14 -81 -24 -108 -16 -38 -17 -52 -8 -67 7 -10 12 -32 12 -49 0\n";
html += "                -23 -5 -31 -24 -36 -33 -8 -66 4 -66 24 -1 32 42 337 50 352 11 23 129 88 190\n";
html += "                104 72 19 90 19 112 -1z m-73 -185 c0 -7 -6 -44 -12 -83 l-12 -70 -62 -3 -62\n";
html += "                -3 14 35 c8 20 15 51 15 70 0 31 4 36 28 42 15 3 38 10 52 14 35 12 40 12 39\n";
html += "                -2z m-344 -204 c17 -4 20 -12 17 -39 l-4 -33 -90 -4 c-87 -3 -91 -4 -119 -36\n";
html += "                l-29 -33 2 -377 3 -377 103 -3 102 -3 0 -44 0 -45 -245 0 -245 0 0 45 0 44\n";
html += "                103 3 102 3 5 427 c6 492 -2 459 103 469 87 8 168 10 192 3z m706 -2 c39 -1\n";
html += "                64 -8 80 -21 l24 -19 5 -429 5 -428 103 -3 102 -3 0 -44 0 -45 -246 0 -245 0\n";
html += "                3 46 3 46 100 2 100 1 5 153 c3 85 2 156 -1 158 -7 4 -6 376 2 389 11 17 -10\n";
html += "                70 -37 96 -25 24 -27 24 -265 27 l-239 3 0 37 0 37 223 -1 c122 0 247 -1 278\n";
html += "                -2z m-221 -122 c66 0 67 0 62 -25 -5 -30 -4 -30 -105 -4 -45 12 -97 21 -114\n";
html += "                20 -20 -1 -33 3 -33 10 0 9 17 10 61 5 34 -3 92 -6 129 -6z m-123 -55 c21 -8\n";
html += "                67 -21 102 -29 34 -8 65 -17 68 -19 16 -16 -43 -270 -86 -371 l-21 -48 -47 7\n";
html += "                c-26 4 -66 16 -88 27 -32 16 -176 101 -194 115 -8 5 33 73 44 73 38 0 122 -36\n";
html += "                147 -62 23 -25 31 -28 43 -18 21 18 19 103 -5 170 -20 58 -27 170 -10 170 5 0\n";
html += "                27 -7 47 -15z m-304 -274 c-42 -75 -110 -137 -129 -118 -2 2 7 28 20 58 13 30\n";
html += "                31 70 39 90 l15 37 40 -11 41 -11 -26 -45z\"/>\n";
html += "            </g>\n";
html += "        </svg>\n";
html += "        <br/>\n";
html += "        <h1>DI.P.S Admin Server</h1>\n";
html += "        <div class=\"section\">\n";
html += "            <p class=\"text\" id=\"pos\">Posti disponibili adesso: <strong id=\"postiCount\">0</strong></p>\n";
html += "            <input id=\"posti_update\" type=\"text\" placeholder=\"Aggiorna il numero di posti disponibili\">\n";
html += "            <button id=\"bottone\">Invia</button>\n";
html += "            <p id=\"confirmation\" class=\"confirmation\" style=\"display: none;\">Numero di posti aggiornato con successo!</p>\n";
html += "        </div>\n";
html += "    </div>\n";
html += "    <script>\n";
html += "        const postiCount = document.getElementById('postiCount');\n";
html += "        const postiAdmin = document.getElementById('posti_update');\n";
html += "        const bottone = document.getElementById('bottone');\n";
html += "        const confirmation = document.getElementById('confirmation');\n";
html += "        bottone.onclick = function() {\n";
html += "            const placesAdmin = postiAdmin.value.trim();\n";
html += "            if (placesAdmin) {\n";
html += "                const xhr = new XMLHttpRequest();\n";
html += "                xhr.open('POST', 'http://192.168.11.89/change_places', true);\n";
html += "                xhr.setRequestHeader('Content-Type', 'text/plain');\n";
html += "                xhr.onload = function() {\n";
html += "                    if (xhr.status === 200) {\n";
html += "                        confirmation.style.display = 'block';\n";
html += "                        postiAdmin.value = '';\n";
html += "                        setTimeout(() => {\n";
html += "                            confirmation.style.display = 'none';\n";
html += "                        }, 3000);\n";
html += "                    }\n";
html += "                };\n";
html += "                xhr.send(placesAdmin);\n";
html += "            }\n";
html += "        };\n";
html += "        setInterval(function() {\n";
html += "            const xhr = new XMLHttpRequest();\n";
html += "            xhr.open('GET', 'http://192.168.11.89/places', true);\n";
html += "            xhr.onload = function() {\n";
html += "                if (xhr.status === 200) {\n";
html += "                    postiCount.textContent = xhr.responseText;\n";
html += "                }\n";
html += "            };\n";
html += "            xhr.send();\n";
html += "        }, 3000);\n";
html += "    </script>\n";
html += "</body>\n";
html += "</html>\n";

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
  initLCD();

  lcd.setCursor(0, 0);
  lcd.print("DI.P.S by B&B");
  delay(1000);


  // Configurazione Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connessione alla rete Wi-Fi...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connessione...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnesso al Wi-Fi!");
  Serial.print("Indirizzo IP: ");
  Serial.println(WiFi.localIP());

  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Posti liberi:");
  lcd.setCursor(0, 1);
  lcd.print(posti_liberi);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Configura le route del server
  setupServerRoutes();
}

void loop() {
  server.handleClient();
}