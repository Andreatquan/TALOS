#define SENSOR_PIN 42

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  int irValue = digitalRead(SENSOR_PIN);

  if (irValue) {
    Serial.println("high");
  } 
  else {
    Serial.println("low");
  }

  delay(500);
}
