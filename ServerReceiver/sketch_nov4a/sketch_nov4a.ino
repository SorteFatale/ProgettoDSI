
//Per la comunicazione 
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "./manageAcc.h"


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
uint8_t macMatrics[2][6] = {
  {0xac, 0x15, 0x18, 0xe9, 0x8d, 0xf0},
  {0xac, 0x15, 0x18, 0xe9, 0xb4, 0x24}
}; 


//Semaforo, utile per lo  Strict2PL implementato nella transazione
SemaphoreHandle_t xMutex;  

//Variabile globale dei posti liberi
int posti_liberi = posti_totale;

//Array di strutture di messaggio.
struct_message boardsStruct[2] = {board1, board2}; //Saranno 4 board alla fine




//------FUNZIONI------


void Task1(void *myData){ //Task da eseguire all'accesso alla risorsa condivisa, posti disponibili del parcheggio.
  struct struct_message* message = (struct struct_message *) myData; //Casting da void* 

  if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){ //Acquire mutex

    Serial.println("Task sta accedendo alla risorsa condivisa.");
    delay(10);

    checkMessagePosti(message, posti_liberi);    //facciamo check di ingresso o uscita
    
    xSemaphoreGive(xMutex); //Release
  }

  vTaskDelete(NULL); //Eliminazione Task, come se fosse un commit.
}



void checkMessagePosti(struct struct_message* message, int posti_liberi){

    registrationPeer(message);

    if(posti_liberi > 0){  //Se ci sono posti liberi...
        
        //Richiesta di ingresso del parcheggioi
        if(message->x == 1){ //Il messaggio è uguale a 1?
          
          myAck.value=1; 

          //Stampa di debug per mac address ricevuto
          Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  message->macSender[0], message->macSender[1], message->macSender[2],
                  message->macSender[3], message->macSender[4], message->macSender[5]);
          
          //Invio ACK=1 al client per farlo entrare.
          esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));
        
          //Aggiorno variabile posti_liberi
          posti_liberi = reducePlaces(result, posti_liberi); //riduciamo i posti

          //funzione stampaPosti
          stampaPosti(result, &myAck, 0);
          
        } 
      
      } else {

        //Qui invece inviamo il messaggio di non disponibilità di posto
        myAck.value=0;
        esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));
        stampaPosti(result, &myAck, 0);

      }


      //Richiesta di uscita dal parcheggio
      if(message->x == 0 && posti_liberi < posti_totale ){
         //ACK per alzata sbarra di uscita
         myAck.value=1;

        //Stampa di debug per mac address ricevuto
        Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                message->macSender[0], message->macSender[1], message->macSender[2],
                message->macSender[3], message->macSender[4], message->macSender[5]);


         esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));
         posti_liberi = increasePlaces(result, posti_liberi);
         stampaPosti(result, &myAck, 1);
      } 

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


/* Si potrebbero eliminare
void reducePlaces(esp_err_t result, int* posti_liberi){

  if (result == ESP_OK) {
      Serial.println("Ack=1 sent with success");
      Serial.println("Decremento il numero di posti!");
      posti_liberi = posti_liberi - 1; 
      Serial.println(posti_liberi);
    }
    else {
      Serial.println("Error sending the data");
    }

}


void increasePlaces(esp_err_t result, int posti_liberi){
  if(result==ESP_OK){
   Serial.println("Incremento il numero di posti"); //Incremento posti per uscire
   posti_liberi = posti_liberi + 1; 
   Serial.println(posti_liberi);

  } else {
    Serial.println("Error sending the data");

  }

}
*/

//Registrazione al Peer se non è stato già fatto.
void registrationPeer(struct struct_message* message){
      // Aggiungi il peer se non esiste già
      if (!esp_now_is_peer_exist(message->macSender)) {
          memcpy(peerInfo.peer_addr, macMatrics[message->id-1], 6);
          peerInfo.channel = 0;
          peerInfo.encrypt = false;
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

  xTaskCreate(Task1, "Task1", 1000, &myData, 1, NULL); //Creazione del Task 


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