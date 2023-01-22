#define echoPin 4
#define trigPin 5
#define RST_PIN 6
#define SS_PIN 7

#include <MFRC522.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);

int LED = 8;
int Switch = 2;
int Alarm = 3;

const unsigned long alarm_after_ms_unauthnticated = 5000;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(Alarm, OUTPUT);
  pinMode(Switch, INPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
  pinMode(8, OUTPUT);
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
}

int readDistance() 
{
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 1 Second
  digitalWrite(trigPin, HIGH);
  delay(100);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  unsigned long duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  return duration * 0.01331 / 2; // Speed of sound wave divided by 2 (go and back)
}

bool tryReadCard()
{
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
  return true;
}

bool tryAuthenticate()
{
  String cardContent;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    cardContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    cardContent.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  cardContent.toUpperCase();
  Serial.print("Card content: ");
  Serial.println(cardContent);

  if (cardContent.substring(1) == "0C AF 46 30") 
  {
    Serial.println("Authorized access");
    return true;
  }

  Serial.println("Access denied");
  return false;
}

bool activated = false;
bool distanceSensorTriggered = false;
bool alarmTriggered = false;
int cardDetectTries = 0;
unsigned long triggerTime = 0;
void loop()
{
  if (!activated)
  {
    if (digitalRead(Switch) == LOW)
      delay(5000);
      activated = true;
    
    distanceSensorTriggered = false;
    alarmTriggered = false;
    cardDetectTries = 0;
    triggerTime = 0;
    digitalWrite(LED, LOW);
  }
  else
  {
    if (!distanceSensorTriggered) 
    {
      int distance = readDistance();

      if (distance < 30)
      {
        digitalWrite(LED, HIGH);
        distanceSensorTriggered = true;
        triggerTime = millis();
      }
    }

    if (distanceSensorTriggered)
    {
      bool cardRead = tryReadCard();

      if (cardRead)
      {
        Serial.println("Card detected, trying to authenticate...");

        bool authenticationSuccess = tryAuthenticate();

        if (authenticationSuccess)
          activated = false;
        else
          alarmTriggered = true;
      }
      else if (!alarmTriggered && (millis() - triggerTime > alarm_after_ms_unauthnticated))
      {
        Serial.println("Not authenticated for too long, triggering alarm...");
        alarmTriggered = true;
      }
    }

    if (alarmTriggered)
    {
      Serial.println("ALARM. . . ");
      digitalWrite(Alarm, HIGH);
      delay(500);
      digitalWrite(Alarm, LOW);
      delay(250);
    }
  }
}
