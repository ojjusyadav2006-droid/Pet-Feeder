#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Servo.h>
#include "arduino_secrets.h"


//WiFi
const char* ssid = "Ollie";
const char* password = "1109197416";

String BOT_TOKEN = SECRET_BOT_TOKEN;

#define SERVO_PIN 26
#define BUZZER_PIN 25


#define CLOSED_ANGLE 86
#define OPEN_ANGLE   130


WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);


Servo feederServo;

// Auto feed vbls
unsigned long feedInterval = 15 * 60 * 1000UL; // default 15 min
unsigned long lastFeedTime = 0;
bool autoFeedEnabled = false;


void dispenseFood() {
  Serial.println("Dispensing Food");

  // Buzzer alert
  digitalWrite(BUZZER_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_PIN, LOW);

  delay(1000);

  feederServo.write(OPEN_ANGLE);
  delay(300);

  feederServo.write(CLOSED_ANGLE);

  Serial.println("Done feeding");
}

void handleAutoFeed() {
  if (!autoFeedEnabled) return;

  if (millis() - lastFeedTime >= feedInterval) {
    dispenseFood();
    lastFeedTime = millis();
  }
}


void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {

    String text = bot.messages[i].text;
    String chat_id = bot.messages[i].chat_id;

    Serial.println("Command: " + text);

    if (text == "/start") {
      bot.sendMessage(chat_id,
        "Smart Feeder Ready!\n\n"
        "/feed - Feed now\n"
        "/set <minutes> - Auto feed\n"
        "/off - Stop auto feed\n"
        "/status - Feeder status",
        "");
    }

    else if (text == "/feed") {
      dispenseFood();
      lastFeedTime = millis();
      bot.sendMessage(chat_id, "Fed completed", "");
    }

    else if (text.startsWith("/set")) {
      int minutes = text.substring(5).toInt();

      if (minutes > 0) {
        feedInterval = minutes * 60 * 1000UL;
        autoFeedEnabled = true;
        lastFeedTime = millis();

        bot.sendMessage(chat_id,
          "Auto feeding set to every " + String(minutes) + " minutes","");
      } else {
        bot.sendMessage(chat_id,
          "Invalid format\nUse: /set 15",
          "");
      }
    }

    else if (text == "/off") {
      autoFeedEnabled = false;
      bot.sendMessage(chat_id, "Auto feeding stopped", "");
    }

    else if (text == "/status") {
      if (autoFeedEnabled) {
        bot.sendMessage(chat_id,
          "Auto feed ON\nInterval: " +
          String(feedInterval / 60000) + " minutes",
          "");
      } else {
        bot.sendMessage(chat_id, "Auto feed OFF", "");
      }
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);

  feederServo.attach(SERVO_PIN);

  feederServo.write(CLOSED_ANGLE);

  // WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  client.setInsecure(); 
}

void loop() {

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }

  handleAutoFeed();
}