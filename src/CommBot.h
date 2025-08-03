#ifndef COMMBOT_H
#define COMMBOT_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define COMMBOT_BUFFER_SIZE 512
#define COMMBOT_MAX_SUB_COUNT 10
#define COMMBOT_MAX_PUB_COUNT 10
#define HANDSHAKE_INTERVAL 2000

class CommBot {
 public:
  using Callback = void (*)(JsonObject&);

  CommBot(HardwareSerial& serial, unsigned long baudrate = 115200);

  void begin(unsigned long baudrate = 115200);
  void spinOnce();
  void advertise(const String& topic, Callback cb);
  void publish(const String& topic, JsonObject& msg);
  void publishRaw(const String& json);
  bool isConnected();
  void setHeartbeatInterval(unsigned long interval);


 private:
  HardwareSerial& _serial;
  unsigned long _baudrate;
  char _buffer[COMMBOT_BUFFER_SIZE];
  size_t _bufferIndex;

  struct Subscriber {
    String topic;
    Callback callback;
  };

  Subscriber _subs[COMMBOT_MAX_SUB_COUNT];
  int _subCount;

  bool _connected = false;
  unsigned long _lastHelloTime = 0;
  void _sendHello();

  void handleMessage(const JsonObject& msg);
  void sendAck(const String& id);
};

#endif
