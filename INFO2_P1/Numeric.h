#ifndef MOTOR_NUMERIC_H
#define MOTOR_NUMERIC_H

#include <cfloat>

// Portable to MSVC 9.0: std::isfinite was introduced after C++03.
inline bool Motor_IsFinite(double value)
{
    return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

#endif
