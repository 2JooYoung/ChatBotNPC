"""ChattingNPC 파이썬 프록시 서버 (골격).

UE와 llama-server 사이에 두는 얇은 프록시.
- GET  /health              : 상태 확인
- POST /v1/chat/completions : llama-server로 요청/응답을 변형 없이 그대로 중계
- POST /stt                 : 향후 음성→텍스트용 자리(현재 미구현 스텁)

주의: 채팅 요청/응답 body는 절대 파싱·변형하지 않는다.
(gemma-4-E2B-it는 thinking 모델 → max_tokens 등을 줄이거나 덮어쓰면 응답이 빈다.)
"""

import os

import httpx
from fastapi import FastAPI, Request, Response
from fastapi.responses import JSONResponse

# 중계 대상 llama-server 주소 (하드코딩 금지 — 환경변수로 조정 가능)
LLAMA_UPSTREAM = os.environ.get("LLAMA_UPSTREAM", "http://127.0.0.1:8080")

app = FastAPI(title="ChattingNPC Proxy")

# 프록시가 UE보다 먼저 끊지 않도록 타임아웃을 두지 않는다(UE 타임아웃 120s가 상한).
_client = httpx.AsyncClient(timeout=None)


@app.on_event("shutdown")
async def _close_client() -> None:
    await _client.aclose()


@app.get("/health")
async def health() -> dict:
    return {"status": "ok"}


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


@app.post("/stt")
async def stt() -> Response:
    """향후 STT(음성→텍스트) 연동용 자리. 현재는 미구현 스텁."""
    return JSONResponse(status_code=501, content={"error": "미구현"})
