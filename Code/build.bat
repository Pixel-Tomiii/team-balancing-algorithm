@echo off
if not exist ..\build mkdir ..\build
pushd ..\build
gcc -w ..\code\*.c -o PointsCreation.exe
popd

PAUSE