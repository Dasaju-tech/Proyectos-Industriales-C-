#include "Motor_versionC.h"
#include "MotorModel_generated.h"
#include "Numeric.h"

namespace
{
bool ValidParameters(double tao, double Km, double T_ms)
{
    const double dt = T_ms / 1000.0;
    return Motor_IsFinite(tao) && Motor_IsFinite(Km) &&
           Motor_IsFinite(T_ms) && tao > 0.0 && Km >= 0.0 &&
           dt > 0.0 && dt <= tao;
}
}

bool MotorSim_Init(sMotorSim* s, double tao, double Km, double T_ms)
{
    if (!s || !ValidParameters(tao, Km, T_ms)) return false;
    sMotorSim initial = {tao, Km, T_ms, 0.0, 0.0, 0.0, 0.0};
    *s = initial;
    return true;
}

bool MotorSim_Step(sMotorSim* s)
{
    if (!s || !ValidParameters(s->tao, s->Km, s->T_ms) ||
        !Motor_IsFinite(s->Vel)) return false;
    double vo, vel, pos;
    if (!MotorModel_Evaluate(s->T_ms / 1000.0, s->tao, s->Km,
                            s->Vi, s->Vo, s->Pos, &vo, &vel, &pos))
        return false;
    s->Vo = vo;
    s->Vel = vel;
    s->Pos = pos;
    return true;
}

bool MotorSim_SetVi(sMotorSim* s, double value)
{
    if (!s || !Motor_IsFinite(value)) return false;
    s->Vi = value;
    return true;
}

bool MotorSim_SetPos(sMotorSim* s, double value)
{
    if (!s || !Motor_IsFinite(value)) return false;
    s->Pos = value;
    return true;
}

bool MotorSim_SetKm(sMotorSim* s, double value)
{
    if (!s || !ValidParameters(s->tao, value, s->T_ms)) return false;
    s->Km = value;
    return true;
}

bool MotorSim_SetTao(sMotorSim* s, double value)
{
    if (!s || !ValidParameters(value, s->Km, s->T_ms)) return false;
    s->tao = value;
    return true;
}

double MotorSim_GetVel(const sMotorSim* s)
{
    return s ? s->Vel : 0.0;
}

double MotorSim_GetPos(const sMotorSim* s)
{
    return s ? s->Pos : 0.0;
}
