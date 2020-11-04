#include "Error.h"

Error::Error(int _code, std::string _msg) : code(_code), msg(_msg) {}

int Error::getCode() { return code; }

std::string Error::getMessage() { return msg; }