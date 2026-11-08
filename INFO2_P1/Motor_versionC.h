#ifndef MOTOR_VERSION_C_H
#define MOTOR_VERSION_C_H

// Value type: no owning pointers, allocation, Windows handles or globals.
// The caller must serialize access when sharing an instance across threads.
struct sMotorSim
{
    double tao;   // time constant [s]
    double Km;    // motor gain [rpm/V]
    double T_ms;  // sampling period [ms]
    double Vi;    // input voltage [V]
    double Vo;    // internal voltage [V]
    double Vel;   // shaft speed [rpm]
    double Pos;   // unwrapped shaft position [degrees]
};

bool MotorSim_Init(sMotorSim* s, double tao, double Km, double T_ms);
bool MotorSim_Step(sMotorSim* s);
bool MotorSim_SetVi(sMotorSim* s, double value);
bool MotorSim_SetPos(sMotorSim* s, double value);
bool MotorSim_SetKm(sMotorSim* s, double value);
bool MotorSim_SetTao(sMotorSim* s, double value);
double MotorSim_GetVel(const sMotorSim* s);
double MotorSim_GetPos(const sMotorSim* s);

#endif
