@echo off
echo Launching both servers in separate windows...
start "llama-server" "%~dp0start_llama.bat"
start "tts-proxy" "%~dp0start_proxy.bat"
echo.
echo Two new windows opened. Wait for the ready messages:
echo   [llama-server] server is listening on http://127.0.0.1:8080
echo   [tts-proxy]    Application startup complete + [tts] MeloTTS Korean loaded
echo.
echo When both windows are ready, play in PIE. You can close THIS window.
pause
