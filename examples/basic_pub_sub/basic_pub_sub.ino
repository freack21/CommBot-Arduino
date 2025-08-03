#include <CommBot.h>
#include <ArduinoJson.h>

CommBot bot(Serial);

void setup() {
  bot.begin(115200);

  bot.advertise("led", [](JsonObject& msg) {
    int state = msg["value"];
    digitalWrite(13, state);
  });

  pinMode(13, OUTPUT);
}

void loop() {
  bot.spinOnce();

  static unsigned long last = 0;
  if (millis() - last > 1000) {
    last = millis();

    StaticJsonDocument<64> doc;
    doc["value"] = analogRead(A0);

    bot.publish("sensor", doc.as<JsonObject>());
  }
}
