@echo off
echo Building ConfigCreator with GCC 15.2.0...
taskkill /F /IM ConfigCreator.exe 2>nul
"C:\Users\Hawai\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe" -std=c++17 -I. ConfigCreator.cpp -o ConfigCreator.exe

if %errorlevel% == 0 (
    echo Build successful! ✓
    echo Executable: ConfigCreator.exe
) else (
    echo Build failed! ✗
)
