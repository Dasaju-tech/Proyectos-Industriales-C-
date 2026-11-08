#ifndef ORDER_VERSION_C_H
#define ORDER_VERSION_C_H

#define MAX_ARGUMENTS 5
#define MAX_ORDER_LEN 50

enum MOTOR_LIST
{
    MOTOR_EXIT = 0,
    MOTOR_KM = 1,
    MOTOR_TIME_CONSTANT = 2,
    MOTOR_VOLTAJE = 3
};

struct sORDER
{
    int cmd;
    int argC;
    double argV[MAX_ARGUMENTS];
};

// Return 0 on success, 1 on invalid input. Output is unchanged on failure.
int Order_parse(const char* line, sORDER* order);
// Return 2 on EOF, 1 on invalid input, 0 on success.
int Order_getOrder(sORDER* order);

#endif
