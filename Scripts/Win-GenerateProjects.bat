@echo off
pushd %~dp0\..\
call dependencies\Premake\bin\5.0.0-beta1\premake5.exe vs2022
popd
PAUSE
