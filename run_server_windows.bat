@echo off
cd /d "%~dp0"
if exist build\Release\ChessServerWin.exe (
  build\Release\ChessServerWin.exe 5555
) else (
  echo Once projeyi derleyin: cmake -S . -B build -G "Visual Studio 17 2022" -A x64
  echo Sonra: cmake --build build --config Release
  pause
)
