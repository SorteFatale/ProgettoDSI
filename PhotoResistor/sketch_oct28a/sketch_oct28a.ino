
int pinInput = 2; 


void setup() {
  // put your setup code here, to run once:
  pinMode(pinInput, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
  int valueState = digitalRead(pinInput);
  Serial.println(buttonState);

  
  if(valueState==HIGH){
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  
  
  //Add optional delay to prevent rapid repeated detections
  // delay(100);



  



}
