#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
UE5_ROOT="${UE5_ROOT:?UE5_ROOT environment variable must be set}"

"$UE5_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" \
    "$PROJECT_ROOT/ClimbingSystem.uproject" \
    -unattended -nop4 -nosplash -NullRHI \
    -ExecCmds="Automation RunTests ClimbingSystem;Quit" \
    -TestExit="Automation Test Queue Empty" \
    -log="$PROJECT_ROOT/TestResults/TestLog.txt"

echo "Test results written to $PROJECT_ROOT/TestResults/"
