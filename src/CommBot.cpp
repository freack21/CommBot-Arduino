#include "CommBot.h"

unsigned long _lastSlaveHeartbeat = 0;
unsigned long _lastMasterHeartbeat = 0;
unsigned long _heartbeatInterval = 1000;
unsigned long _maxHeartbeatLossTime = 10000;

CommBot::CommBot(HardwareSerial& serial, unsigned long baudrate)
  : _serial(serial), _baudrate(baudrate), _bufferIndex(0), _subCount(0) {}

void CommBot::begin(unsigned long baudrate) {
  if (baudrate != -1) _baudrate = baudrate;

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

void CommBot::setMaxHeartbeatLossTime(unsigned long interval) {
  _maxHeartbeatLossTime = interval;
}

void CommBot::log(String msg) {
  JsonDocument doc;
  doc["log"] = "[slave] " + msg;
  serializeJson(doc, _serial);
  _serial.println();
}

void CommBot::_sendHello() {
  if (!_connected && millis() - _lastHelloTime > HANDSHAKE_INTERVAL) {
    JsonDocument doc;
    doc["handshake"] = "slave";
    serializeJson(doc, _serial);
    _serial.println();
    _lastHelloTime = millis();
  }
}

void CommBot::subscribe(const String& topic, Callback cb) {
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

      JsonDocument doc;
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
  if (now - _lastSlaveHeartbeat >= _heartbeatInterval) {
    JsonDocument heartbeatDoc;
    heartbeatDoc["heartbeat"] = "slave";
    serializeJson(heartbeatDoc, _serial);
    _serial.println();

    _lastSlaveHeartbeat = now;
  }
  now = millis();
  if (_connected && now - _lastMasterHeartbeat >= _maxHeartbeatLossTime) {
    _connected = false;
    _lastMasterHeartbeat = now;
  }
}

void CommBot::handleMessage(const JsonObject& msg) {
  if (msg.containsKey("handshake")) {
    String val = msg["handshake"].as<String>();
    if (val == "master") {
      _connected = true;
      _lastMasterHeartbeat = millis();
    }
    return;
  }

  if (msg.containsKey("heartbeat")) {
    String val = msg["heartbeat"].as<String>();
    if (val == "master") {
      _lastMasterHeartbeat = millis();
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
}

void CommBot::publish(const String& topic, JsonDocument& msg) {
  JsonObject _msg = msg.as<JsonObject>();

  _msg["topic"] = topic;

  if (!_msg.containsKey("id")) {
    _msg["id"] = millis();
  }

  serializeJson(_msg, _serial);
  _serial.println();
}

void CommBot::publishRaw(const String& json) {
  _serial.print(json);
  _serial.println();
}
