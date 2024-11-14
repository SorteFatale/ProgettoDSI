#include "manageAcc.h"

int reducePlaces(esp_err_t result, int posti_liberi){

    if (result == ESP_OK) {
      posti_liberi = posti_liberi - 1; 
      
    }
  
    return posti_liberi;
}


int increasePlaces(esp_err_t result, int posti_liberi){

  if(result==ESP_OK){
     posti_liberi = posti_liberi + 1; 

  }
 
  return posti_liberi;

}