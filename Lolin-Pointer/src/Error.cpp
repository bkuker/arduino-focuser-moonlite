#include <Arduino.h>
#include "Error.h"

Error::Error(int _code, std::string _msg) : code(_code), msg(_msg) {
    Serial.print("Error: ");
    Serial.println(msg.c_str());
}

int Error::getCode() { return code; }

std::string Error::getMessage() { return msg; }