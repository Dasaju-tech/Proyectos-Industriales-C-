/********************************************************/
//Motor_versionC.h
//Fichero de cabeceras (header) para la simulación de un motor DC
//Esteban del Castillo / Ernest Gil				9/9/2020
/********************************************************/
#pragma once
#ifndef MOTOR_H
#define MOTOR_H

#include <windows.h>
#include <stdio.h>

typedef struct{	// estructura
	float
		tao,	//constante de tiempo [s]
		Km,		//constante de motor [rpm/V]
		T_ms,	//tiempo de muestreo [ms]
		Vi,		//tensión de entrada
		Vo,		//tensión interna[V]: internalV*Km = wo
		Vel,	//velocidad de salida [rpm]
		Pos;	//posición de salida [grados]
	void (*TimFun)();	// funcion de atencion al timer periodico
	HANDLE WIN_hTimerQueue;
	HANDLE WIN_hTimer;
}sMotorSim;

int MotorSim_Init(sMotorSim *s,float tao, float Km, float T_ms, void (*TimFun)(void));
int MotorSim_End(sMotorSim *s);
void MotorSim_SetVi(sMotorSim *s,float Vi);	// Ajusta la entrada Vi a un valor [V]
float MotorSim_GetVel(sMotorSim *s);	// Obtiene la velocidad actual [rpm]
float MotorSim_GetPos(sMotorSim *s); // Obtiene la posición actual
void MotorSim_SetPos(sMotorSim *s,float Pos);	// fuerza posicion actual a un valor [grados]

//#pragma once
#endif
