/********************************************************/
//MotorMain_versionC.cpp
//Simula la operación de un motor DC
//Esteban del Castillo / Ernest Gil				9/9/2020
/********************************************************/

#include "Motor_versionC.h"
#include "Order_versionC.h"

sORDER g_sOrder;
sMotorSim g_sMotorSim;

//Función para la presentación en patalla del estado del motor
void MotorTimFun()
{
	static int count = 0;
	COORD coord, oldCoord;

	float deltaVo;
	
	//--- Calculo simulacion
	





	//--- Windows
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);//manejador de la ventana de consola
	// info de la ventana
	if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
	{
		printf("GetConsoleScreenBufferInfo (%d)\n", GetLastError());
		return;
	}

	// se posiciona el cursor para info de estado
	// guardar antiguas coordenadas del cursor



	// mover cursor a x=20, y=1



	// obtener e imprimir tensión y velocidad del motor



	// mover cursor a antiguas cordenadas



}

int  main()
{
	printf("Hola, mundo\n");

	//se crea el motor: estructura, tao[s], Km[rpm/V], tSample[ms], timerFun
	if (!MotorSim_Init(&g_sMotorSim,10, 100, 1000, MotorTimFun)) {
		printf("MotorSim failed\n");
		return 1;
	}
	
	MotorSim_SetVi(&g_sMotorSim,0.0); //excitación del motor [V]
	MotorSim_SetPos(&g_sMotorSim,0.0); //grados en origen
	
	while (true)
	{
		if (Order_getOrder(&g_sOrder) != 0) break; //captura de orden
		else if (g_sOrder.cmd == MOTOR_EXIT)
		{
			printf("\tMOTOR_EXIT\n");
			break;
		}
		switch (g_sOrder.cmd)
		{
		case MOTOR_KM:
			g_sMotorSim.Km=g_sOrder.argV[0];
			printf("\tMOTOR_KM=%.1f\n", g_sOrder.argV[0]);
			break;
		case MOTOR_TIME_CONSTANT:
			g_sMotorSim.tao=g_sOrder.argV[0];
			printf("\tMOTOR_TIME_CONSTANT=%.1f\n", g_sOrder.argV[0]);
			break;
		case MOTOR_VOLTAJE:
			g_sMotorSim.Vi=g_sOrder.argV[0];
			printf("\tMOTOR_VOLTAJE=%.1f\n", g_sOrder.argV[0]);
			break;
		}
	}
	
	MotorSim_End(&g_sMotorSim);
	
	return 0;
}

