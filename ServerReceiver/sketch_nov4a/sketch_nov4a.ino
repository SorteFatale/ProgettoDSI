/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-many-to-one-esp32/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

//Per la comunicazione 
#include <esp_now.h>
#include <WiFi.h>
//Per la gestione dellla variabile condivisa
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define posti_totale 10


esp_now_peer_info_t peerInfo;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  int x; //x=1 è apri, x=0 sarà chiudi.
  uint8_t macSender[6]; //Indirizzo mac del sender
}struct_message;

typedef struct ack_structure { //Ack message structure
  int value;
} ack_structure;

// Create a struct_message called myData
struct_message myData; 
ack_structure myAck;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;


uint8_t macMatrics[1][6] = {
  {0xac, 0x15, 0x18, 0xe9, 0x8d, 0xf0}
}; //Abbiamo una matrice 4x6, una riga per ogni scheda 



SemaphoreHandle_t xMutex;  // Definire il mutex

//Posti del parcheggio
int posti_parcheggio = posti_totale;

// Create an array with all the structures
struct_message boardsStruct[2] = {board1, board2}; //Saranno 4 board alla fine


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

    if(posti_parcheggio > 0){ //Faccio entrare solo se i posti ci sono

        if(message->x == 1){
          
          myAck.value=1;

          //Stampa di debug per mac address ricevuto
          Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  message->macSender[0], message->macSender[1], message->macSender[2],
                  message->macSender[3], message->macSender[4], message->macSender[5]);
          
          //Qui dobbiamo inviare un'ack di posto disponibile per aprire la sbarra

          
          esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));
        
          reducePlaces(result); //riduciamo i posti
          
        } 

        


      } else {
        //Qui invece inviamo il messaggio di non disponibilità di posto
        myAck.value=0;
        
        esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));

        if (result == ESP_OK) {
          Serial.println("Ack=0 sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }

      }


      if(message->x == 0 && posti_parcheggio < posti_totale ){
         //ack alzata sbarra di uscita
         myAck.value=1;
         esp_err_t result = esp_now_send(macMatrics[message->id-1], (uint8_t *) &myAck, sizeof(myAck));
         increasePlaces(result);
      } 

}


void reducePlaces(esp_err_t result ){

  if (result == ESP_OK) {
      Serial.println("Ack=1 sent with success");
      Serial.println("Decremento il numero di posti!");
      posti_parcheggio = posti_parcheggio - 1; 
      Serial.println(posti_parcheggio);
    }
    else {
      Serial.println("Error sending the data");
    }

}


void increasePlaces(esp_err_t result){
  if(result==ESP_OK){
   Serial.println("Incremento il numero di posti"); //Incremento posti per uscire
   posti_parcheggio = posti_parcheggio + 1; 
   Serial.println(posti_parcheggio);

  } else {
    Serial.println("Error sending the data");

  }

}


void registrationPeer(struct struct_message* message){
      

      memcpy(peerInfo.peer_addr, macMatrics[message->id-1], 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      

      // Aggiungi il peer se non esiste già
      if (!esp_now_is_peer_exist(message->macSender)) {
          if (esp_now_add_peer(&peerInfo) != ESP_OK) {
              Serial.println("Errore nell'aggiungere il peer");
              return;
          }
      }
}



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
  if (myData.id > 0 && myData.id <= 2) {  // Assicurati che l'ID sia valido per evitare errori
    boardsStruct[myData.id - 1].x = myData.x;
    Serial.printf("Valore x: %d", boardsStruct[myData.id - 1].x);
  } else {
    Serial.println("ID non valido, pacchetto ignorato.");
  }

  
  xTaskCreate(Task1, "Task1", 1000, &myData, 1, NULL); //Creazione del Task 


  Serial.println();
}



void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);
  xMutex = xSemaphoreCreateMutex(); //Creazione semaforo
  
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();


  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  //Ci mettiamo in ascolto
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv)); 
}
 
void loop() {
  delay(10000);  
}