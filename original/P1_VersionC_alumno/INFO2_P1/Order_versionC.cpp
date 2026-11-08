/********************************************************/
//Order_versionC.cpp
//Clase para la captación de órdenes desde teclado
//Esteban del Castillo / Ernest Gil				9/9/2020
/********************************************************/

#include "Order_versionC.h"

int aa=0;
extern sORDER g_sOrder; //estructura de órdenes (no publica)

//consigue una línea de orden desde teclado
//formato: command arg_1, arg_2...
//el elemento separador es el espacio y sólo de aceptan valores numéricos
//Retorna 0 si todo OK; 1 si fallo
int Order_getOrder(sORDER *order_p)
{
    char c;  //caracter leido en teclado
    char str[MAX_ORDER_LEN], index = 0; //cadena de entrada desde teclado
    BOOL noNext = FALSE; //elimina teclas de doble caracter
    int count = 0;
	int i;

    do {
        c = _getch();
        if (c == 0) { noNext = TRUE; continue; }
        else if (noNext)
        {
            noNext = FALSE;
            printf("...%d\n", c);
            continue;
        }
        //si tecla retroceso
        else if (c == 8)
        {
            CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
            COORD coordinates;

            //manejador de la ventana de consola
            HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
            // info de la ventana
            if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
            {
                printf("GetConsoleScreenBufferInfo (%d)\n", GetLastError());
                return 0;
            }
            //se actualiza la coordenada X (un lugar hacia atrás)
            coordinates.X = csbiInfo.dwCursorPosition.X;
            coordinates.Y = csbiInfo.dwCursorPosition.Y;

            if (coordinates.X) coordinates.X--;
            SetConsoleCursorPosition(hStdout, coordinates);
            _putch(' '); //borra y avanza una posición
            SetConsoleCursorPosition(hStdout, coordinates);

            if (index) index--; //mantiene la posición en str
        }
        //si tecla CR
        else if (c == '\r')
        {
            _putch('\r'); _putch('\n');
            str[index] = 0;
            break;
        }
        //si tecla espacio
        else if (c == ' ') //separador de campos
        {
            _putch(c);
            str[index++] = 0; //se sustituye por cero
            if (index >= MAX_ORDER_LEN)
            {
                printf("max command line length reached\n");
                return 1;
            }

            continue;
        }
        //si no es tecla válida (valor numérico)
        else if ((c < 48 || c>58) && c != '.' && c != '+' && c != '-')
            continue;
        //si tecla válida
        else
        {
            _putch(c);
            str[index++] = c;
            if (index >= MAX_ORDER_LEN)
            {
                printf("max command line length reached\n");
                return 1;
            }
        }
        //   printf("%0x\n", c);
    } while (c != '\r');

    //extracción de argumentos
    //se inicializa la structura ORDER
    for (i = 0; i < index; i++) //búsqueda del primer caracter válido
        if (str[i] != 0) break;

    if (i <= index) //uno o más caracteres
    {
        order_p->cmd = atoi(str + i);  //conversión a numérico
        i += strlen(str + i) + 1; //posiciona el índice
        for (; i < index; i++)
        {
            if (str[i] != 0)
            {
                order_p->argV[count++] = atof(str + i);
                i += strlen(str + i); //posiciona el índice
                if (count >= MAX_ARGUMENTS)
                {
                    printf("Too many arguments\n");
                    return 1;
                }
            }
        }
    }
    else
        return 1; //no order

    order_p->argC = count;
    return 0;
}
   