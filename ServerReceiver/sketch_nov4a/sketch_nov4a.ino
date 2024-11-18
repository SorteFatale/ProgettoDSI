
//Per la comunicazione 
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "./manageAcc.h"
#include <LiquidCrystal_I2C.h>


//------VARIABILI e STRUTTURE--------

#define posti_totale 10

//Struttura utile per ESP_NOW
esp_now_peer_info_t peerInfo;

//Struttura del messaggio che riceverà il Server
typedef struct struct_message {
  int id;
  int x; //x=1 è apri, x=0 sarà chiudi.
  uint8_t macSender[6]; //Indirizzo mac del sender
}struct_message;

//Struttura del messaggio di ACK
typedef struct ack_structure { 
  int value;
} ack_structure;

//Instanziamo le strutture
struct_message myData; 
ack_structure myAck;

//Strutture per mantenere i valori inviati dalle varie board 
struct_message board1;
struct_message board2;
struct_message board3;
struct_message board4;

//Matrice di indirizzi MAC, utile per inviare gli ACK ai rispettivi sender. E' una 4x6.
uint8_t macMatrics[3][6] = {
  {0xac, 0x15, 0x18, 0xe9, 0x8d, 0xf0},
  {0xac, 0x15, 0x18, 0xe9, 0xb4, 0x24},
  {0xac, 0x15, 0x18, 0xe9, 0x9f, 0x4c}
}; 


//Semaforo, utile per lo  Strict2PL implementato nella transazione
SemaphoreHandle_t xMutex;  

//Variabile globale dei posti liberi
int posti_liberi = posti_totale;

//Array di strutture di messaggio.
struct_message boardsStruct[2] = {board1, board2, board3}; //Saranno 4 board alla fine


//oggetto lcd
LiquidCrystal_I2C lcd(0x27, 16,2); 



//------FUNZIONI------

void init_lcd(){
    // inizializzo lo schermo LCD
  lcd.init();
  // accendo la backlight del LCD                      
  lcd.backlight();
}


//funzione per stampare il numero di posti attualmente disponibili
void stampa_posti_schermo(LiquidCrystal_I2C lcd, int posti_liberi){
  lcd.setCursor(0,0);
  lcd.print("Posti liberi:");

  lcd.setCursor(0,1);
  lcd.print(posti_liberi);
  lcd.print("  ");

}

//stampa che mostra la non disponibilità di posti
void stampa_non_disp_schermo(LiquidCrystal_I2C lcd){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Posti finiti!");
}


void Task1(void *myData){ //Task da eseguire all'accesso alla risorsa condivisa, posti disponibili del parcheggio.
  struct struct_message* message = (struct struct_message *) myData; //Casting da void* 

  if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){ //Acquire mutex

    Serial.println("Task sta accedendo alla risorsa condivisa.");
    delay(10);

    checkMessagePosti(message);    //facciamo check di ingresso o uscita
    
    xSemaphoreGive(xMutex); //Release
  }

  vTaskDelete(NULL); //Eliminazione Task, come se fosse un commit.
}



void checkMessagePosti(struct struct_message* message){

    registrationPeer(message);

    if(message->x == 1){
      //Se ho un messaggio di ingresso...
        
        //Se ci sono posti liberi...
        if(posti_liberi > 0){
          

          esp_err_t result = openBar(message);
          //Aggiorno variabile posti_liberi

          posti_liberi = reducePlaces(result, posti_liberi); //riduciamo i posti


          //stampo posti su lcd
          if(posti_liberi!=0){
            stampa_posti_schermo(lcd,posti_liberi);
          } else {
            stampa_non_disp_schermo(lcd);
          }
          //funzione stampaPosti
          stampaPosti(result, &myAck, 0);
          
        } else {

            notOpenBar(message);
        }

    //Richiesta di uscita dal parcheggio
    } else if(message->x == 0 && posti_liberi < posti_totale ){
        //ACK per alzata sbarra di uscita
        esp_err_t result = openBar(message);
        posti_liberi = increasePlaces(result, posti_liberi);
        stampa_posti_schermo(lcd,posti_liberi);
        stampaPosti(result, &myAck, 1);
    } 

}


esp_err_t openBar(struct struct_message* message){

    myAck.value=1; 

    //Stampa di debug per mac address ricevuto
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
            message->macSender[0], message->macSender[1], message->macSender[2],
            message->macSender[3], message->macSender[4], message->macSender[5]);
          
    //Invio ACK=1 al client per farlo entrare.
    esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));

    return result;
}


esp_err_t notOpenBar(struct struct_message* message){
  //Qui invece inviamo il messaggio di non disponibilità di posto
    myAck.value=0;
    esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));
    stampaPosti(result, &myAck, 0);

    return result;
} 


//Questa è una funzione che stampa l'avvenuto invio del messaggio di Ack. 
void stampaPosti(esp_err_t result, struct ack_structure* myAck, int sel){

  if (myAck->value == 1 && result == ESP_OK){

    Serial.println("Ack=1 sent with success");
    sel == 0 ? Serial.println("Decremento il numero di posti!") : Serial.println("Incremento il numero di posti!");
    Serial.println(posti_liberi);
            
  } else if (myAck->value == 0 && result == ESP_OK) {
    Serial.println("Ack=0 sent with success");

  } else {
    Serial.println("Error sending the data");
  }

}




//Registrazione al Peer se non è stato già fatto.
void registrationPeer(struct struct_message* message){
      // Aggiungi il peer se non esiste già

      memcpy(peerInfo.peer_addr, macMatrics[message->id-1], 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      if (!esp_now_is_peer_exist(message->macSender)) {
          
          
          if (esp_now_add_peer(&peerInfo) != ESP_OK) {
              Serial.println("Errore nell'aggiungere il peer");
              return;
          }
      }
}

//CallBack di Ricezione
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {

  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);


  
  // Copia i dati ricevuti nella struttura myData, 
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("ID della Board: %d, Dimensione pacchetto: %d byte\n", myData.id, len);
  
  // Aggiorna i dati nella corrispondente struttura della board
  if (myData.id > 0 && myData.id <= 3) {  // Assicurati che l'ID sia valido per evitare errori
    boardsStruct[myData.id - 1].x = myData.x;
    Serial.printf("Valore x: %d", boardsStruct[myData.id - 1].x);
  } else {
    Serial.println("ID non valido, pacchetto ignorato.");
  }

  xTaskCreate(Task1, "Task1", 10000, &myData, 1, NULL); //Creazione del Task 


  Serial.println();
}


void setup() {
  //Initializza Seriale
  Serial.begin(115200);


  //Creazione semaforo
  xMutex = xSemaphoreCreateMutex(); 
  
  //Set del Device come modalità station
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();

  //Inizializzazione schermo 
  init_lcd();
  stampa_posti_schermo(lcd,posti_liberi);


  //Inizializzazione ESP-Now
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  //Inizializzazione schermo 
  
  //Ci mettiamo in ascolto
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv)); 
}

void loop() {

  delay(10000);  
}