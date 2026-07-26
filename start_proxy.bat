@echo off
title tts-proxy (port 8000)
cd /d "%~dp0python_server"
echo === Starting tts-proxy (port 8000) ===
echo Loading TTS... Ready when you see: Application startup complete  and  [tts] MeloTTS Korean loaded
echo.
".venv\Scripts\python.exe" -m uvicorn main:app --host 127.0.0.1 --port 8000
echo.
echo === tts-proxy stopped ===
pause
