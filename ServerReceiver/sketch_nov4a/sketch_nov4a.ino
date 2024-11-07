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

#define posti 10


// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  int x; //x=1 è apri, x=0 sarà chiudi.
}struct_message;

// Create a struct_message called myData
struct_message myData; 

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;

SemaphoreHandle_t xMutex;  // Definire il mutex

//Posti del parcheggio
int posti_parcheggio = posti;

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

    if(posti_parcheggio > 0){ //Faccio entrare solo se i posti ci sono

        if(message->x == 1){
          
          Serial.println("Decremento il numero di posti!");
          posti_parcheggio = posti_parcheggio - 1; 
          Serial.println(posti_parcheggio);
        } 

        //Qui dobbiamo inviare un'ack di posot disponibile per aprire la sbarra

      } else {
        //Qui invece inviamo il messaggio di non disponibilità di posto
      }

      if(message->x == 0 && posti_parcheggio < posti ){
          Serial.println("Incremento il numero di posti"); //Incremento posti per uscire
          posti_parcheggio = posti_parcheggio + 1; 
          Serial.println(posti_parcheggio);

          //ack alzata sbarra di uscita
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