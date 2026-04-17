@echo off
SET PROJECT_ROOT=%~dp0
SET UE5_ROOT=%UE5_ROOT%

IF "%UE5_ROOT%"=="" (
    echo ERROR: UE5_ROOT environment variable not set.
    echo Set it to your UE5 engine root, e.g. C:\Program Files\Epic Games\UE_5.7
    exit /b 1
)

"%UE5_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
    "%PROJECT_ROOT%ClimbingSystem.uproject" ^
    -unattended -nop4 -nosplash -NullRHI ^
    -ExecCmds="Automation RunTests ClimbingSystem;Quit" ^
    -TestExit="Automation Test Queue Empty" ^
    -log="%PROJECT_ROOT%TestResults\TestLog.txt"

echo Test results written to %PROJECT_ROOT%TestResults\
