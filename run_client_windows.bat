@echo off
cd /d "%~dp0"
if exist build\Release\ChessWin.exe (
  start "" build\Release\ChessWin.exe
) else (
  echo Once projeyi derleyin: cmake -S . -B build -G "Visual Studio 17 2022" -A x64
  echo Sonra: cmake --build build --config Release
  pause
)
