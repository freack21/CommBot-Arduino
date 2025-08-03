#include "CommBot.h"

unsigned long _lastHeartbeat = 0;
unsigned long _heartbeatInterval = 1000;

CommBot::CommBot(HardwareSerial& serial, unsigned long baudrate)
  : _serial(serial), _baudrate(baudrate), _bufferIndex(0), _subCount(0) {}

void CommBot::begin(unsigned long baudrate) {
  _baudrate = baudrate;
  _serial.begin(_baudrate);

  _connected = false;
  _lastHelloTime = millis();
}

bool CommBot::isConnected() {
  return _connected;
}

void CommBot::setHeartbeatInterval(unsigned long interval) {
  _heartbeatInterval = interval;
}

void CommBot::_sendHello() {
  if (!_connected && millis() - _lastHelloTime > HANDSHAKE_INTERVAL) {
    StaticJsonDocument<64> doc;
    doc["handshake"] = "hello";
    serializeJson(doc, _serial);
    _serial.println();
    _lastHelloTime = millis();
  }
}

void CommBot::advertise(const String& topic, Callback cb) {
  if (_subCount < COMMBOT_MAX_SUB_COUNT) {
    _subs[_subCount++] = { topic, cb };
  }
}

void CommBot::spinOnce() {
  _sendHello();

  while (_serial.available()) {
    char c = _serial.read();
    if (c == '\n') {
      _buffer[_bufferIndex] = '\0';

      StaticJsonDocument<COMMBOT_BUFFER_SIZE> doc;
      DeserializationError err = deserializeJson(doc, _buffer);
      if (!err) {
        JsonObject obj = doc.as<JsonObject>();
        handleMessage(obj);
      }

      _bufferIndex = 0;
    } else {
      if (_bufferIndex < COMMBOT_BUFFER_SIZE - 1) {
        _buffer[_bufferIndex++] = c;
      }
    }
  }

  unsigned long now = millis();
  if (now - _lastHeartbeat >= _heartbeatInterval) {
    StaticJsonDocument<64> heartbeatDoc;
    heartbeatDoc["heartbeat"] = true;
    serializeJson(heartbeatDoc, _serial);
    _serial.println();

    _lastHeartbeat = now;
  }
}

void CommBot::handleMessage(const JsonObject& msg) {
  if (msg.containsKey("handshake")) {
    String val = msg["handshake"].as<String>();
    if (val == "hello_ack") {
      _connected = true;
    }
    return;
  }

  if (!msg.containsKey("topic")) return;

  String topic = msg["topic"].as<String>();

  for (int i = 0; i < _subCount; i++) {
    if (_subs[i].topic == topic) {
      _subs[i].callback(msg);
      break;
    }
  }

  if (msg.containsKey("id")) {
    sendAck(msg["id"].as<String>());
  }
}

void CommBot::sendAck(const String& id) {
  StaticJsonDocument<128> ackDoc;
  ackDoc["ack"] = id;
  serializeJson(ackDoc, _serial);
  _serial.println();
}

void CommBot::publish(const String& topic, JsonObject& msg) {
  msg["topic"] = topic;

  if (!msg.containsKey("id")) {
    msg["id"] = millis();
  }

  serializeJson(msg, _serial);
  _serial.println();
}

void CommBot::publishRaw(const String& json) {
  _serial.print(json);
  _serial.println();
}
