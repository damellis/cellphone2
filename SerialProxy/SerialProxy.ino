int pwrkey = 19;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  
  delay(1000);
  digitalWrite(pwrkey, LOW);
  pinMode(pwrkey, OUTPUT);
  delay(2000);
  pinMode(pwrkey, INPUT);
  delay(3000);

  Serial.println("READY");
  Serial1.print("AT\r");
}

void loop() {
  // read from port 1, send to port 0:
  if (Serial1.available()) {
    int inByte = Serial1.read();
    Serial.write(inByte); 
  }
  
  // read from port 0, echo to port 0, send to port 1:
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte); 
    Serial.write(inByte);
  }
}
