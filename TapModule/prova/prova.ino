

const int tapSensorPin = 2;

void setup() {
  // put your setup code here, to run once:
  
  pinMode(tapSensorPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);


  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
 

  int tapState = digitalRead(tapSensorPin); // Read the state of the Tap Sensor
  
  if (tapState == LOW) {
    Serial.println("Tap detected!"); // Display a message when a tap is detected
    // Your custom actions or functions can be added here.
    digitalWrite(LED_BUILTIN, HIGH);

  }
  
  



  



}
