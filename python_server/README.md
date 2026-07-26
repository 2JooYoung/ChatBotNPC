# python_server — ChattingNPC 프록시 + TTS 서버

UE와 `llama-server` 사이에 두는 얇은 프록시(FastAPI, 기본 `:8000`).
채팅 요청을 llama-server로 그대로 중계하고, NPC 대사를 한국어 음성으로 합성하는 `/tts`를 제공한다.

## 엔드포인트
- `GET  /health` → `{"status": "ok", "tts": true|false}` (tts=모델 로드 여부)
- `POST /v1/chat/completions` → llama-server로 **변형 없이** 중계
- `POST /tts` → 한국어 음성 합성. body `{"text": "<대사>", "speed": 1.0, "pitch": 0.0}` → `audio/wav`(16-bit PCM mono)
  - `pitch`: 반음 단위 피치 시프트(음수=낮게/굵게, 양수=높게). NPC별 음색 차등용. 범위 ±8.
  - `speed`: 말하기 속도 배수(0.5~2.0).
  - 모델 미로드 시 503, 빈 텍스트 400, 합성 실패 500
- `POST /stt` → 501 `{"error": "미구현"}` (향후 음성→텍스트용 스텁)

## 실행
```
cd python_server
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
uvicorn main:app --host 127.0.0.1 --port 8000
```

사전 조건: Python 3.10+, llama-server 기동
```
llama-b10038-bin-win-cuda-12.4-x64\llama-server.exe -m ..\model\gemma-4-E2B-it-Q8_0.gguf --host 127.0.0.1 --port 8080
```

## TTS 설치 (MeloTTS 한국어) — Windows + Python 3.11 검증됨

> **반드시 Python 3.11**로 venv를 만든다. 3.12는 MeloTTS의 구버전 의존성(tokenizers/fugashi)
> wheel이 없어 설치가 실패한다. (`py -3.11 -m venv .venv`)

`.venv` 활성화 상태에서 **아래 순서대로**:
```
pip install -r requirements.txt                              # fastapi, ..., soundfile, kiwipiepy
pip install git+https://github.com/myshell-ai/MeloTTS.git    # torch(CPU) 등 대용량
python -m unidic download                                    # MeloTTS import 시 필요(일본어 사전)
pip install "setuptools<80"                                  # librosa가 쓰는 pkg_resources 복원
```
- 순서 주의: MeloTTS 설치가 setuptools를 최신으로 올려 `pkg_resources`가 사라지므로 **마지막에** 되돌린다.
- **`python-mecab-ko`는 설치 금지** — mecab-python3와 폴더명(대소문자) 충돌로 import가 깨진다.
  한국어 G2P는 `kiwipiepy`를 `eunjeon`으로 감싸 우회한다(`melo_ko_patch.py`, 서버가 자동 적용).

동작 확인(모델 직접 로드):
```
python tts_smoke.py     # out_kr.wav 생성 + "SMOKE OK". 첫 실행은 모델 가중치 다운로드로 느림.
```
- 성공 시 서버 기동 로그에 `[tts] MeloTTS Korean loaded (sr=44100, ...)`.
- 로드 실패해도 서버는 뜨고 채팅 중계는 정상, `/tts`만 503 반환(로그에 사유 출력).
- 합성 속도: CPU 기준 약 1x 실시간(5초 대사 ≈ 5초). torch는 CPU 빌드라 llama와 VRAM 경합 없음.
  더 빠르게 하려면 CUDA torch 설치 후 `TTS_DEVICE=cuda:0`(단, RTX 3060 VRAM을 llama와 나눠 씀).

## /tts 단독 검증
```
# kor.json 을 UTF-8로 저장: {"text": "안녕하세요, 무슨 일이지?"}
curl -X POST http://127.0.0.1:8000/tts --data-binary @kor.json -H "Content-Type: application/json" -o out.wav
```
→ `out.wav` 재생해 한국어 발음 확인. (한글은 반드시 UTF-8 파일로 전달 — 셸 인코딩 이슈 회피)

## 설정 (환경변수)
- `LLAMA_UPSTREAM` — 중계 대상 llama-server (기본 `http://127.0.0.1:8080`)
- `TTS_DEVICE` — TTS 디바이스 `auto` | `cpu` | `cuda:0` (기본 `auto`)

## UE 연결
Project Settings > Game > Local LLM Settings의 **ServerUrl**을
`http://127.0.0.1:8000/v1/chat/completions`로 변경하면 UE가 프록시를 경유한다.
(TTS는 UE 쪽 작업 2에서 별도 HTTP 호출로 연결한다.)
