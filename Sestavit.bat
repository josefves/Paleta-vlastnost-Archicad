@echo off
rem Sestaveni doplnku PaletaVlastnosti pro Archicad 29 (Windows)
rem Potrebuje: Visual Studio 2022 (C++), CMake a Python 3.10+
cd /d "%~dp0"
python Tools\BuildAddOn.py --configFile config.json --acVersion 29 --buildConfig RelWithDebInfo
if errorlevel 1 (
    echo.
    echo Sestaveni selhalo. Chybove hlaseni najdete vyse.
    pause
    exit /b 1
)
for /r Build %%f in (PaletaVlastnosti.apx) do if exist "%%f" copy /y "%%f" "%~dp0PaletaVlastnosti.apx" >nul
echo.
echo Hotovo: %~dp0PaletaVlastnosti.apx
pause
