@echo off
title llama-server (port 8080)
cd /d "%~dp0"
echo === Starting llama-server (port 8080) ===
echo Loading model... Ready when you see: server is listening on http://127.0.0.1:8080
echo.
"llama-b10038-bin-win-cuda-12.4-x64\llama-server.exe" -m "model\gemma-4-E2B-it-Q8_0.gguf" --host 127.0.0.1 --port 8080
echo.
echo === llama-server stopped ===
pause
