/********************************************************/
//Order_versionC.h
//Clase para la captación de órdenes desde teclado
//Esteban del Castillo / Ernest Gil				9/9/2020
/********************************************************/

#ifndef ORDER_H
#define ORDER_H

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>

#define MAX_ARGUMENTS 5  //máx número de argumentos
#define MAX_ORDER_LEN 50 //longitud máx del string

enum MOTOR_LIST  //ordenes atendidas
{
	MOTOR_EXIT,
	MOTOR_KM,
	MOTOR_TIME_CONSTANT,
	MOTOR_VOLTAJE
};

typedef struct sORDER //estructura de una orden
{
	char cmd,
		argC;
	float argV[MAX_ARGUMENTS];
}sORDER;

int Order_getOrder(sORDER* order_p); // obtiene una orden de teclado

#endif
#pragma once
