"""ChattingNPC 파이썬 프록시 서버.

UE와 llama-server 사이에 두는 얇은 프록시 + 로컬 TTS(음성 합성).
- GET  /health              : 상태 확인
- POST /v1/chat/completions : llama-server로 요청/응답을 변형 없이 그대로 중계
- POST /tts                 : NPC 대사 텍스트 → 한국어 음성(WAV). MeloTTS 로컬 합성.
- POST /stt                 : 향후 음성→텍스트용 자리(현재 미구현 스텁)

주의: 채팅 요청/응답 body는 절대 파싱·변형하지 않는다.
(gemma-4-E2B-it는 thinking 모델 → max_tokens 등을 줄이거나 덮어쓰면 응답이 빈다.)
"""

import asyncio
import io
import os

import httpx
import soundfile as sf
from fastapi import FastAPI, Request, Response
from fastapi.responses import JSONResponse
from starlette.concurrency import run_in_threadpool

# 중계 대상 llama-server 주소 (하드코딩 금지 — 환경변수로 조정 가능)
LLAMA_UPSTREAM = os.environ.get("LLAMA_UPSTREAM", "http://127.0.0.1:8080")

# TTS 추론 디바이스: 'auto' | 'cpu' | 'cuda:0' (GPU가 llama와 공존 어려우면 'cpu')
TTS_DEVICE = os.environ.get("TTS_DEVICE", "auto")

app = FastAPI(title="ChattingNPC Proxy")

# 프록시가 UE보다 먼저 끊지 않도록 타임아웃을 두지 않는다(UE 타임아웃 120s가 상한).
_client = httpx.AsyncClient(timeout=None)

# --- TTS 상태 (앱 시작 시 1회 로드) -----------------------------------------
_tts_model = None          # MeloTTS 모델 (로드 실패 시 None → /tts 503)
_tts_speaker_id = None     # 한국어 화자 id
_tts_sample_rate = None    # 모델 출력 샘플레이트 (WAV 헤더에 그대로 담김)
_tts_lock = asyncio.Lock() # 모델 추론 직렬화(동시 요청 보호)


@app.on_event("startup")
async def _load_tts() -> None:
    """MeloTTS 한국어 모델을 1회 로드. 실패해도 서버는 뜨고 /tts만 503."""
    global _tts_model, _tts_speaker_id, _tts_sample_rate
    try:
        from melo_ko_patch import install_eunjeon_shim

        install_eunjeon_shim()  # Windows 한국어 G2P 우회 (import melo 이전)
        from melo.api import TTS

        model = TTS(language="KR", device=TTS_DEVICE)
        _tts_model = model
        _tts_speaker_id = model.hps.data.spk2id["KR"]
        _tts_sample_rate = model.hps.data.sampling_rate
        print(f"[tts] MeloTTS Korean loaded (sr={_tts_sample_rate}, device={TTS_DEVICE}).")
    except Exception as exc:  # noqa: BLE001 - 로드 실패를 격리(채팅 중계는 계속)
        _tts_model = None
        print(f"[tts] MeloTTS load failed; /tts will return 503. reason: {exc}")


@app.on_event("shutdown")
async def _close_client() -> None:
    await _client.aclose()


@app.get("/health")
async def health() -> dict:
    return {"status": "ok", "tts": _tts_model is not None}


@app.post("/v1/chat/completions")
async def chat_completions(request: Request) -> Response:
    """요청 body를 읽은 그대로 llama-server로 중계하고, 응답도 그대로 반환."""
    body = await request.body()
    url = f"{LLAMA_UPSTREAM}/v1/chat/completions"
    headers = {"Content-Type": request.headers.get("content-type", "application/json")}

    try:
        upstream = await _client.post(url, content=body, headers=headers)
    except httpx.RequestError as exc:
        # upstream 연결 실패 → 502 (UE의 "서버 연결 불가" 처리와 호환)
        return JSONResponse(
            status_code=502,
            content={"error": f"upstream 연결 실패: {exc}"},
        )

    # status_code / body / content-type를 변형 없이 그대로 통과
    return Response(
        content=upstream.content,
        status_code=upstream.status_code,
        media_type=upstream.headers.get("content-type"),
    )


def _shift_pitch(audio, sr: int, semitones: float):
    """포먼트를 보존하며 피치만 옮긴다(praat PSOLA "Change gender").

    naive 피치 시프트는 포먼트까지 밀려 '다람쥐' 소리가 나므로, 음색(포먼트)을
    대부분 유지한 채 피치 중앙값만 이동해 자연스럽게 만든다. 실패 시 librosa 폴백.
    """
    import numpy as np

    try:
        import parselmouth
        from parselmouth.praat import call

        snd = parselmouth.Sound(np.asarray(audio, dtype="float64"), sampling_frequency=sr)
        f0 = snd.to_pitch().selected_array["frequency"]
        voiced = f0[f0 > 0]
        if voiced.size == 0:
            return np.asarray(audio, dtype="float32")

        ratio = 2.0 ** (semitones / 12.0)
        new_median = min(max(float(np.median(voiced)) * ratio, 75.0), 500.0)
        # 포먼트는 조금만(부분 지수) 옮겨 음색 유지 → 자연스러움 우선
        formant_ratio = ratio ** 0.35
        out = call(snd, "Change gender", 75, 600, formant_ratio, new_median, 1.0, 1.0)
        return np.asarray(out.values, dtype="float32").flatten()
    except Exception:  # noqa: BLE001 - praat 실패 시 안전 폴백
        import librosa

        return librosa.effects.pitch_shift(
            np.asarray(audio, dtype="float32"), sr=sr, n_steps=float(semitones)
        )


def _synthesize_wav(text: str, speed: float, pitch: float) -> bytes:
    """블로킹 합성 → 16-bit PCM mono WAV 바이트. 스레드풀에서 호출한다.

    MeloTTS 한국어는 화자가 1종이라, NPC별 음색은 speed(속도)와
    pitch(반음 단위, 포먼트 보존 시프트)로 차등화한다. pitch≈0이면 시프트 생략.
    """
    import numpy as np

    audio = _tts_model.tts_to_file(text, _tts_speaker_id, output_path=None, speed=speed)
    audio = np.asarray(audio, dtype="float32")
    if abs(pitch) >= 0.1:
        audio = _shift_pitch(audio, _tts_sample_rate, pitch)
    buf = io.BytesIO()
    # 샘플레이트는 모델 출력값을 그대로 사용 → UE는 WAV 헤더에서 읽는다.
    sf.write(buf, audio, _tts_sample_rate, format="WAV", subtype="PCM_16")
    return buf.getvalue()


@app.post("/tts")
async def tts(request: Request) -> Response:
    """NPC 대사 텍스트를 한국어 음성(WAV)으로 합성해 반환."""
    if _tts_model is None:
        return JSONResponse(status_code=503, content={"error": "TTS 모델 미로드"})

    try:
        payload = await request.json()
    except Exception:  # noqa: BLE001
        return JSONResponse(status_code=400, content={"error": "잘못된 JSON"})

    text = (payload.get("text") or "").strip()
    if not text:
        return JSONResponse(status_code=400, content={"error": "빈 텍스트"})

    try:
        speed = float(payload.get("speed", 1.0))
    except (TypeError, ValueError):
        speed = 1.0
    try:
        pitch = float(payload.get("pitch", 0.0))
    except (TypeError, ValueError):
        pitch = 0.0
    # 극단값이 음질을 망치지 않도록 안전 범위로 제한
    speed = min(max(speed, 0.5), 2.0)
    pitch = min(max(pitch, -8.0), 8.0)

    try:
        async with _tts_lock:  # 추론 직렬화(모델 동시 접근 방지)
            wav_bytes = await run_in_threadpool(_synthesize_wav, text, speed, pitch)
    except Exception as exc:  # noqa: BLE001
        return JSONResponse(status_code=500, content={"error": f"합성 실패: {exc}"})

    return Response(content=wav_bytes, media_type="audio/wav")


@app.post("/stt")
async def stt() -> Response:
    """향후 STT(음성→텍스트) 연동용 자리. 현재는 미구현 스텁."""
    return JSONResponse(status_code=501, content={"error": "미구현"})
