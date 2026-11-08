@echo off
setlocal
rem Python is not needed: the generated C++ model is checked into Git.
if not defined VS90COMNTOOLS (
    echo Visual Studio 2008 C++ is required. VS90COMNTOOLS is not set.
    exit /b 1
)
call "%VS90COMNTOOLS%..\..\VC\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
pushd "%~dp0.."
vcbuild INFO2_P1.sln "Release|Win32"
if errorlevel 1 goto fail
if not exist build mkdir build
cl /nologo /EHsc /W4 /I INFO2_P1 /Fo.\build\ /Fe.\build\test_motor_vs2008.exe tests\test_motor.cpp INFO2_P1\Motor_versionC.cpp INFO2_P1\Order_versionC.cpp
if errorlevel 1 goto fail
build\test_motor_vs2008.exe
if errorlevel 1 goto fail
cl /nologo /EHsc /W4 /D_CRT_SECURE_NO_WARNINGS /I INFO2_P1 /Fo.\build\ /Fe.\build\test_win32_timer_vs2008.exe tests\test_win32_timer.cpp INFO2_P1\Motor_versionC.cpp INFO2_P1\Order_versionC.cpp
if errorlevel 1 goto fail
build\test_win32_timer_vs2008.exe
if errorlevel 1 goto fail
Release\INFO2_P1.exe --simulate 20 > build\response_vs2008.csv
if errorlevel 1 goto fail
echo Build, unit tests and simulation completed. CSV: build\response_vs2008.csv
popd
exit /b 0
:fail
echo Visual Studio 2008 validation failed.
popd
exit /b 1
