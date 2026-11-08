/********************************************************/
//Motor_versionC.cpp
//Clase para la simulación de un motor DC
//Esteban del Castillo / Ernest Gil				9/9/2020
/********************************************************/

#include "Motor_versionC.h"

extern sMotorSim g_sMotorSim;	// declaramos una estructura global

//Define la simulación del motor DC
//recibe el control cada g_motor.deltaT [s]
VOID CALLBACK TimerMotor(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	if (g_sMotorSim.TimFun) g_sMotorSim.TimFun();
}
//-----------------------------------------------------------------------------
int MotorSim_Init(sMotorSim *s,float tao, float Km, float T_ms, void (*TimFun)(void))
{
	if (!s) return 0;
	s->tao=tao;
	s->Km=Km;
	s->T_ms=T_ms;
	s->TimFun=TimFun;
	s->Vi=0;
	s->Vo=0;
	s->Pos=0;
	s->Vel=0;
	
    // Set a timer to call the timer routine in tSample seconds.
	s->WIN_hTimerQueue = NULL;
	s->WIN_hTimerQueue = CreateTimerQueue();
	if (!s->WIN_hTimerQueue) {
		printf("CreateTimerQueue failed (%d)\n", GetLastError());
		return 0;
	}
    if (!CreateTimerQueueTimer(&s->WIN_hTimer, s->WIN_hTimerQueue,
		(WAITORTIMERCALLBACK) TimerMotor, NULL, (DWORD)T_ms,(DWORD)T_ms, 0)) {
        printf("CreateTimerQueueTimer failed (%d)\n", GetLastError());
        return 0;
        }
	return 1;
}
//-----------------------------------------------------------------------------
int MotorSim_End(sMotorSim *s)
{
	//Se eliminan los timers creados
	if (!DeleteTimerQueueEx(s->WIN_hTimerQueue, INVALID_HANDLE_VALUE))
	{
		printf("DeleteTimerQueue failed (%d)\n", GetLastError());
		return 0;
	}
	return 1;
}
//-----------------------------------------------------------------------------
void MotorSim_SetVi(sMotorSim *s,float Vi)	// Ajusta la entrada Vi a un valor [V]
{ s->Vi=Vi; }
//-----------------------------------------------------------------------------
float MotorSim_GetVel(sMotorSim *s)	// Obtiene la velocidad actual [rpm]
{ return s->Vel; }
//-----------------------------------------------------------------------------
float MotorSim_GetPos(sMotorSim *s) // Obtiene la posición actual
{ return s->Pos; }
//-----------------------------------------------------------------------------
void MotorSim_SetPos(sMotorSim *s,float Pos)	// fuerza posicion actual a un valor [grados]
{ s->Pos=Pos; }
//-----------------------------------------------------------------------------

