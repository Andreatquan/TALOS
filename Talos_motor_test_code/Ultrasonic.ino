#define TRIG_PIN  1    // GPIO pin connected to TRIG of HC-SR04
#define ECHO_PIN  2   // GPIO pin connected to ECHO of HC-SR04
   
long duration;
float distanceCm;
   
void setup() {
  Serial.begin(115200);        // Initialize serial communication
  pinMode(TRIG_PIN, OUTPUT);   // Set TRIG pin as output
  pinMode(ECHO_PIN, INPUT);    // Set ECHO pin as input
}
   
void loop() {
  // Send a 10µs pulse to trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
   
  // Read echo time
  duration = pulseIn(ECHO_PIN, HIGH);
   
  // Convert time to distance (speed of sound = 0.034 cm/µs)
  distanceCm = duration * 0.034 / 2;
   
  // Display distance on serial monitor
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");
   
  delay(500);  // Wait half a second before next reading
}