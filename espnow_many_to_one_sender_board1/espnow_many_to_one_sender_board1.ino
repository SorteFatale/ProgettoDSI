#include <esp_now.h>
#include <WiFi.h>

// Sostituire con l'indirizzo MAC del ricevitore
uint8_t broadcastAddress[] = {0x88, 0x13, 0xbf, 0x6f, 0xa0, 0xf0};

// Struttura di esempio per l'invio dei dati
// Deve corrispondere alla struttura del ricevitore
typedef struct struct_message {
    int id; // deve essere univoco per ogni scheda mittente
    int x;
} struct_message;

// Creare una variabile di tipo struct_message chiamata myData
struct_message myData;

// Creare l'interfaccia peer
esp_now_peer_info_t peerInfo;

// Callback chiamata quando i dati vengono inviati
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nStato dell'ultimo pacchetto inviato:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Consegna riuscita" : "Consegna fallita");
}

void peer_registration(){
  // Registrazione del peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);  // Memorizza l'indirizzo MAC del peer che ha dimensione 6 bytes
  peerInfo.channel = 0;  // Operazione su qualsiasi canale
  peerInfo.encrypt = false;  // Disabilita la crittografia
  
  // Aggiunge il peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Aggiunta del peer fallita");
    return;
  }
}
 
void setup() {
  // Inizializza il monitor seriale
  Serial.begin(115200);

  // Imposta il dispositivo come stazione Wi-Fi
  WiFi.mode(WIFI_STA);

  // Inizializza ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore durante l'inizializzazione di ESP-NOW");
    return;
  }

  // Una volta che ESP-NOW è stato inizializzato con successo, registriamo la callback
  // per ottenere lo stato del pacchetto trasmesso
  esp_now_register_send_cb(OnDataSent);
  
  peer_registration();

}
 
void loop() {
  // Imposta i valori da inviare
  myData.id = 1;  // ID univoco per questa scheda
  myData.x = 1;

  // Invia il messaggio tramite ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Inviato con successo");
    Serial.println("Dati inviati:");
    Serial.println(myData.x);
  } 
  else {
    Serial.println("Errore durante l'invio dei dati");
  }
  
  // Attendi 10 secondi prima del prossimo invio
  delay(10000);
}