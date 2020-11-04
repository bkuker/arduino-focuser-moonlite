#ifndef ERROR_H_
#define ERROR_H_
#include <string>
#include <stdexcept>

#define ASCOM_NOT_IMLEMENTED(P) Error(1024, "Property or Method " #P " is Not Implemented")
#define ASCOM_INVALID(P) Error(1025, "Invalid " #P " Value")
#define ASCOM_UNSET(P) Error(1026, #P " Unset")
#define ASCOM_NOT_CONNECTED Error(1031, "Not Connected")
#define ASCOM_INVALID_WHILE_PARKED(P) Error(1032, #P " is Invalid While Parked" )
#define ASCOM_INVALID_WHILE_SLAVED(P) Error(1033, #P " is Invalid While Slaved" )
#define ASCOM_INVALID_OPERATION(P) Error(1025, "Invalid Operation " #P )
#define ASCOM_ACTION_NOT_IMLEMENTED(P) Error(1036, "Action " #P " is Not Implemented")

class Error : public std::exception {
private:
	int code;
    std::string msg;
public:
    Error(int _code, std::string _msg);
    int getCode();
    std::string getMessage();
};

#endif