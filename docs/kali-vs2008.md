# Probar desde Kali GNU/Linux

Puedes desarrollar y probar este proyecto en Kali. Para comprobar específicamente
Visual Studio 2008 hace falta ejecutar su compilador; elegir `-std=c++03` en GCC
no reproduce todos sus límites ni su SDK.

| Entorno | Qué comprueba |
| --- | --- |
| GCC nativo en Kali | Modelo, grafo, parser, CLI y sanitizadores de Linux. |
| MinGW de 32 bits + Wine | Compilación de las fuentes Win32 y ejecución del `.exe` sobre la implementación de Windows de Wine. |
| MinGW de 64 bits + Wine64 | Ejecución de la misma implementación de la API de Windows, compilada para otro destino. |
| Windows + Visual Studio 2008 C++ | El compilador, proyecto y SDK del destino original. |

## Prueba rápida en Kali

Desde la raíz del checkout, instala las herramientas nativas y ejecuta:

```sh
sudo apt update
sudo apt install python3 g++
python3 tools/dev.py test
python3 tools/dev.py test --sanitize
./build/motor --simulate 20
```

Este flujo comprueba la simulación sin esperar tiempo real. No ejecuta la rama
del temporizador Windows.

## Compilar para Windows y ejecutar con Wine

En Kali **amd64**, habilita los paquetes de 32 bits y prepara MinGW y Wine:

```sh
dpkg --print-architecture
sudo dpkg --add-architecture i386
sudo apt update
apt-cache policy g++-mingw-w64-i686 wine wine32:i386 wine64
sudo apt install python3 g++ g++-mingw-w64-i686 wine wine32:i386 wine64
```

Comprueba que `apt-cache policy` ofrece candidatos: la disponibilidad puede variar
según la versión y los repositorios de Kali. Estas instrucciones presuponen una
máquina x86 de 64 bits; Kali en ARM necesita además un entorno x86.

Ahora ejecuta, como tu usuario habitual:

```sh
python3 tools/test_win32.py
```

El script genera los ejecutables en `build/win32/`, usa MinGW i686 para compilar
código Windows de 32 bits y ejecuta las pruebas C++, la CLI y una comprobación
del temporizador bajo Wine. Utiliza un prefijo propio en `build/wine32/`, sin
modificar tu prefijo habitual; puedes seleccionar otro con `WINEPREFIX`.
**No ejecutes Wine con `sudo`.**

Si sólo tienes el compilador o quieres separar la compilación de la ejecución:

```sh
python3 tools/test_win32.py --compile-only
```

Esto construye los tres ejecutables sin exigir Wine; no declara que las pruebas
se hayan ejecutado.

Para abrir después la consola interactiva:

```sh
WINEPREFIX="$PWD/build/wine32" WINEARCH=win32 wine build/win32/motor.exe
```

Introduce `3 1` para aplicar un voltio, espera unos segundos para observar la
velocidad y termina con `0`. Para obtener CSV sin interacción:

```sh
WINEPREFIX="$PWD/build/wine32" WINEARCH=win32 wine build/win32/motor.exe --simulate 20
```

### Alternativa de 64 bits

Si tu instalación permite Wine64 pero no el runtime de 32 bits, puedes comprobar
la ruta de código Windows con ejecutables de 64 bits:

```sh
sudo apt install python3 g++-mingw-w64-x86-64 wine64
python3 tools/test_win32.py --arch win64
```

Esta variante utiliza `build/win64/` y un prefijo separado, `build/wine64/`, con
`WINEARCH=win64`. Si ya exportaste `WINEARCH=win32`, elimínalo de tu entorno antes
de elegir 64 bits. Puedes añadir `--compile-only` para comprobar sólo su
compilación.

En algunas instalaciones Debian sin escritorio está disponible el binario de
Wine64 pero falta el lanzador `wine`. Si existe `/usr/lib/wine/wine64`, selecciónalo:

```sh
python3 tools/test_win32.py --arch win64 --wine /usr/lib/wine/wine64
WINEPREFIX="$PWD/build/wine64" WINEARCH=win64 /usr/lib/wine/wine64 build/win64/motor.exe
```

El proyecto original exige **Win32 de 32 bits**. La alternativa de 64 bits prueba
la lógica y las llamadas a la API de Windows, pero no ejecuta los binarios de 32
bits ni sustituye la compilación con VS2008.

MinGW/Wine permite probar mucho más de la ruta Windows que GCC nativo. Su
resultado sigue siendo una prueba con **GCC para Windows y Wine**, y no acredita
que `cl.exe` de 2008 acepte el proyecto ni que el comportamiento de Wine sea
idéntico al de Windows. La validación específica con MSVC 2008 sigue pendiente.

### Resultados comprobados el 8 de noviembre de 2026

Se ejecutó el script en un contenedor Debian 12 con MinGW GCC 12-posix y
Wine 8.0 (paquete Debian `8.0~repack-4`).

| Comprobación | Resultado |
| --- | --- |
| Compilar aplicación, núcleo de pruebas y prueba del temporizador para i686 | Los tres ejecutables PE de 32 bits compilan en C++03 estricto. |
| Ejecutar esos binarios con Wine32 | No ejecutados: el kernel de nube rechaza el runtime Linux ELF32 con `Exec format error`. |
| Compilar los tres ejecutables para x86-64 | Aprobado en C++03 estricto. |
| Ejecutar el núcleo bajo Wine64 | 1536 comprobaciones, 0 fallos. |
| Ejecutar temporizadores bajo Wine64 | 29 comprobaciones, 0 fallos: callbacks, cambios, parada, reinicio y destrucción. |
| Ejecutar la CLI bajo Wine64 | 7 pruebas aprobadas y sesión de órdenes con salida correcta. |
| Pruebas nativas Linux tras los cambios | 1536 comprobaciones C++ y 26 pruebas Python aprobadas, también con sanitizadores. |
| Compilar con MSVC 2008 / ejecutar en Windows nativo | Pendiente; no están disponibles su compilador ni su SDK. |

La ejecución fue sin escritorio: no se comprobó visualmente el repintado de la
fila de estado. El avance del estado por los callbacks sí se comprobó, sin
simular las funciones de Windows. Wine emitió un aviso de inicialización sobre
`syswow64/rundll32.exe` por la limitación de 32 bits; las pruebas de 64 bits
terminaron correctamente. Los paquetes se instalaron dentro del contenedor,
sin modificar los privilegios ni los paquetes del sistema anfitrión.

## Usar el Visual Studio 2008 real desde Kali

La opción más fiel es una máquina virtual de Windows en QEMU/KVM o VirtualBox,
con los medios y licencias de Windows y Visual Studio 2008 que tengas disponibles.
Usa la edición, service pack y SDK exigidos por la práctica; mantén ese Windows
antiguo aislado de Internet si no necesita acceso.

1. Instala Windows compatible con esa edición de VS2008 y el componente Visual C++.
2. Copia o comparte el checkout completo con la máquina virtual. La cabecera
   generada está versionada: no hace falta instalar Python en Windows.
3. Abre `INFO2_P1.sln` **de la raíz**, selecciona `Release | Win32` y compila.
4. Ejecuta `tools\build_vs2008.cmd` para construir, pasar las pruebas C++ y
   guardar `build\response_vs2008.csv`. Si falta `VS90COMNTOOLS`, revisa la
   instalación de Visual C++ 2008.
5. Ejecuta la consola con Ctrl+F5 y prueba las órdenes y actualizaciones periódicas.

También se puede intentar ejecutar `cl.exe` y las herramientas de una instalación
legítima de VS2008 bajo Wine. Requiere sus cabeceras, bibliotecas y SDK completos;
la instalación del IDE, .NET y `vcbuild` añade más compatibilidad por comprobar.
No es una sustitución garantizada de la máquina virtual. Copiar únicamente
`cl.exe` o renombrar GCC no reproduce el toolchain.

Consulta [el README](../README.md) para las ecuaciones, órdenes y límites del
modelo.
