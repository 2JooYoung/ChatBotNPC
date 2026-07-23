# python_server — ChattingNPC 프록시 서버

UE와 `llama-server` 사이에 두는 얇은 프록시(FastAPI, 기본 `:8000`).
채팅 요청을 llama-server로 그대로 중계하고, 향후 STT/TTS/요약을 붙일 자리를 모은다.

## 엔드포인트
- `GET  /health` → `{"status": "ok"}`
- `POST /v1/chat/completions` → llama-server로 **변형 없이** 중계
- `POST /stt` → 501 `{"error": "미구현"}` (향후 음성→텍스트용 스텁)

## 실행
```
cd C:\Work\Projects\ChatBot\python_server
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
uvicorn main:app --host 127.0.0.1 --port 8000
```

사전 조건: Python 3.10+, llama-server 기동
```
llama-b10038-bin-win-cuda-12.4-x64\llama-server.exe -m ..\model\gemma-4-E2B-it-Q8_0.gguf --host 127.0.0.1 --port 8080
```

## 설정
- `LLAMA_UPSTREAM` 환경변수로 중계 대상 변경 (기본 `http://127.0.0.1:8080`).

## UE 연결
Project Settings > Game > Local LLM Settings의 **ServerUrl**을
`http://127.0.0.1:8000/v1/chat/completions`로 변경하면 UE가 프록시를 경유한다.
