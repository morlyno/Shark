@echo off
pushd %~dp0\..\
call dependencies\Premake\bin\premake5.exe vs2019
popd
if NOT "%1"=="nopause" PAUSE