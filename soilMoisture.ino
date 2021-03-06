const int AirValue = 854; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int soilMoistureValue = 0;
int soilMoisturePercent = 0;

void setup() {
  Serial.begin(9600);

}

void loop() {
  soilMoistureValue = analogRead(A0);
  Serial.println(soilMoistureValue);
  soilMoisturePercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  if (soilMoisturePercent > 100) {
    Serial.println("100%");
  } else if (soilMoisturePercent < 0) {
    Serial.println("0%");
  } else if (soilMoisturePercent > 0 && soilMoisturePercent < 100) {
    Serial.print(soilMoisturePercent);
    Serial.println("%");
  }
  delay(250);
}
