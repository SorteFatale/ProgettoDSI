#include <esp_now.h> //libreria della scheda ESP32

//librerie per permettere la comunicazione Wi-Fi alle schede 
#include <WiFi.h> 
#include <esp_wifi.h>

#include <ESP32Servo.h> //libreria per il servomotore

#define PIN_SG90 16 // pin output servomotore
#define pinInput 5 //pin di input per il photo-interrupter

//oggetto servomotore
Servo sg90;

//Indirizzo MAC del ricevitore (il server centrale)
uint8_t broadcastAddress[] = {0x88, 0x13, 0xbf, 0x6f, 0xa0, 0xf0};

// Struttura del messaggio l'invio dei dati
// Deve corrispondere alla struttura del ricevitore
typedef struct struct_message {
    int id; // deve essere univoco per ogni scheda mittente
    int x;//contenuto del messaggio da inviare (in questo caso, "una macchina deve uscire")
    uint8_t mac_address[6]; //mac address dek mittente
} struct_message;

//struttura dati per ricevere l'ack dal server (ack per alzare la sbarra o no)
typedef struct ack_struct{
  int value;
} ack_struct;

//variabile di stato per capire se ho già inviato il messaggio al server
int valido = 0;


// Creo una variabile di tipo struct_message chiamata myData e un'altra per conservare il messaggio di ACK che manda il server 
struct_message myData;
ack_struct myAck;

// Creo l'interfaccia peer
esp_now_peer_info_t peerInfo;

// Callback chiamata quando i dati vengono inviati
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nStato dell'ultimo pacchetto inviato:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Consegna riuscita" : "Consegna fallita");
}

//alzo la sbarra, aspetto 2 secondi, la riabbasso
void activate_parking_bar(){
  //questo for alza la sbarra
  for (int pos = 90; pos <= 180; pos += 1) {
    sg90.write(pos);
    delay(10);
  }
  delay(2000);
  //questo for la abbassa
  for (int pos = 180; pos >= 90; pos -= 1) {
    sg90.write(pos);
    delay(10);
  }
}

//Callback chiamata quando ricevo l'ack dal server
void OnAckRecv(const uint8_t * mac, const uint8_t *myAck_incoming, int len) {
  memcpy(&myAck, myAck_incoming, sizeof(myAck));
  if(myAck.value==1){ //se il server mi ha risposto con ACK=1, allora la macchina può uscire
    Serial.println("LA SBARRA SI ALZA");
    //alzo la sbarra, aspetto 2 secondi, poi la riabbasso
    activate_parking_bar();
  }else{
     Serial.println("PARCHEGGIO PIENO!");
  }
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

void setup_servo(){
  sg90.setPeriodHertz(50); // frequenza della PWM per il servomotore SG90
  sg90.attach(PIN_SG90, 500, 2400); // definisco la durata minima e massima dell'impulso da inviare al servomotore: 500 e 2400 corrispondono all'angolo minimo e massimo che può raggiungere il servomotore
  sg90.write(90); //settiamo la sbarra a 90°, in modo da bloccare inizialmente l'accesso al parcheggio
}
 
void setup() {

  // Imposta il dispositivo come stazione Wi-Fi
  WiFi.mode(WIFI_STA);

  //invoco la configurazione iniziale del servomotore
  setup_servo();

  pinMode(pinInput, INPUT); //configurazione del pin del photo-interrupter come pin di input

  // Inizializza il monitor seriale
  Serial.begin(115200);

  // Inizializza ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore durante l'inizializzazione di ESP-NOW");
    return;
  }

  // Una volta che ESP-NOW è stato inizializzato con successo, registriamo la callback
  // per ottenere lo stato del pacchetto trasmesso
  esp_now_register_send_cb(OnDataSent);
  
  //la scheda 1 si registra come peer del server, in modo da inviargli messaggi
  peer_registration();

 //registro la callback per ricevere il messaggio di ack dal server
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnAckRecv));

}
 
void loop() {
  
  delay(200);
  int valueState = digitalRead(pinInput); //mi leggo il valore digitale dal pin 5 (quello del photo-interrupter)
  Serial.println(valueState);
  // Imposta i valori da inviare
  myData.id = 2;  // ID univoco per questa scheda
  myData.x = 0; //questa scheda invierà sempre 1 perché gestirà solo le uscite dal parcheggio

  if(valido==0 && valueState == HIGH){ //se il sensore rileva la carta nella fotocellula, allora invia la richiesta

    // Invia il messaggio tramite ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    valido = 1; //e quindi salvo nella variabile di stato il fatto che ho inviato il messaggio (così da non inviarlo più volte)
    if (result == ESP_OK) {
      Serial.println("Inviato con successo");
      Serial.println("Dati inviati:");
      Serial.println(myData.x);
    } 
    else {
      Serial.println("Errore durante l'invio dei dati");
    }
  }

  if(valido==1 && valueState==LOW){//se ho tolto la carta dalla fotocellula e il messaggio è stato già inviato, ripristino il valore della variabile di stato
    valido = 0;
  }

}