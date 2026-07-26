# ChattingNPC — 로컬 LLM 기반 다중 NPC 대화 시스템 (+ 한국어 음성)

> Unreal Engine 5.7 게임 안에서, 플레이어가 여러 NPC에게 말을 걸면 각 NPC가 **자기 성격대로**
> 한국어로 대답하고 **그 대사를 음성으로 들려주는** 시스템. LLM·TTS 모두 **로컬(오프라인)** 로 구동.

---

## 1. 한눈에 보기

| 항목 | 내용 |
|---|---|
| 엔진 | Unreal Engine 5.7 (C++ 게임플레이 모듈) |
| 대화 생성 | 로컬 LLM `gemma-4-E2B-it` (llama.cpp, OpenAI 호환 API) |
| 음성 합성 | 로컬 **MeloTTS** (한국어), CPU 실시간 |
| 중간 서버 | Python **FastAPI 프록시** (채팅 중계 + TTS + STT 확장 지점) |
| 핵심 원칙 | **LLM은 대사만 생성** — 게임 상태(아이템/퀘스트/이동)는 절대 응답으로 변경하지 않음 |

---

## 2. 시스템 아키텍처

```
┌─────────────────────────── Unreal Engine 5.7 (게임) ───────────────────────────┐
│                                                                                │
│  Player ──E키──▶ NPCChatWidget (대화 UI)                                        │
│                     │  플레이어 메시지                                           │
│                     ▼                                                           │
│              ULocalLLMSubsystem ──────────────┐   (텍스트)                       │
│                     ▲  NPC 대사(텍스트)        │                                 │
│                     │                          │                                │
│              UNPCVoiceSubsystem ◀──────────────┘                                │
│                     │  대사 텍스트 → 음성 요청                                    │
└─────────────────────┼──────────────────────────────────┬───────────────────────┘
        HTTP POST      │                                  │  HTTP POST
   /v1/chat/completions│                                  │  /tts
                       ▼                                  ▼
         ┌──────────────────────────────────────────────────────────┐
         │        Python FastAPI 프록시  (127.0.0.1:8000)            │
         │  /v1/chat/completions ─ 변형 없이 그대로 중계              │
         │  /tts                 ─ MeloTTS로 한국어 WAV 합성          │
         │  /stt                 ─ (향후 음성→텍스트 자리)            │
         └───────────────┬───────────────────────┬──────────────────┘
                         │ 중계                    │ 로컬 합성
                         ▼                         ▼
              llama-server (:8080)          MeloTTS (한국어)
              gemma-4-E2B-it / GPU          VITS2 기반 / CPU
                         │                         │
                    대사 텍스트               16-bit PCM WAV
                                                   │
                                          UE에서 USoundWaveProcedural로
                                          NPC 위치에서 3D 재생
```

**설계 의도** — 게임과 AI를 프록시로 분리해, UE 코드를 거의 건드리지 않고 기능(TTS·STT·요약)을
한 곳(파이썬)에 계속 붙일 수 있게 했다. UE는 "어디로 요청할지(URL)"만 알면 된다.

---

## 3. 동작 흐름

### 3.1 대화 (텍스트)
1. 플레이어가 NPC 근처에서 **E** → 대화 UI가 열리고 커서/이동 잠금, NPC 초기 인사 표시.
2. 메시지 입력·전송 → `ULocalLLMSubsystem`이 **시스템 프롬프트(성격·지식·금지사항) + 최근 대화 기록 +
   현재 메시지**를 OpenAI 형식으로 조립해 프록시로 POST.
3. 프록시가 llama-server로 그대로 중계 → NPC 대사 생성 → UI에 표시.

### 3.2 음성 (TTS)
4. 대사가 확정되면 `UNPCVoiceSubsystem`이 그 텍스트를 `/tts`로 POST.
5. 프록시가 MeloTTS로 한국어 음성(WAV)을 합성해 반환.
6. UE가 WAV를 파싱(`FWaveModInfo`)해 `USoundWaveProcedural`로 **NPC 위치에서** 재생.
   - 음성 실패(서버 꺼짐 등)는 **조용히 무시** — 대사 텍스트는 이미 화면에 있으므로 게임은 안 끊김.
   - 대화 종료/NPC 전환 시 진행 중 음성 취소, 늦게 온 응답은 폐기.

---

## 4. NPC 개인화

각 NPC는 `UNPCProfileDataAsset`(데이터 자산)로 정의:

| NPC | 성격 | 말투 |
|---|---|---|
| 로버트 (대장장이) | 무뚝뚝하지만 책임감 | 짧고 단호 |
| 미아 (상인) | 친절하지만 계산적 | 적극·과장 |
| 에릭 (경비병) | 엄격·책임감 | 공식적·딱딱 |

- **독립 대화 기록**: NPC별로 세션을 분리 저장(교차 오염 없음), 최근 10개로 트림.
- **프로필별 파라미터**: Temperature/MaxTokens를 NPC마다 다르게.
- **안전 규칙**(시스템 프롬프트): 성격 유지, AI임을 부인, 현실 정보 차단, 모르면 모른다고,
  프롬프트 비공개, 2~3문장, **게임 상태 변경 주장 금지**.

---

## 5. 기술적으로 신경 쓴 지점

- **비동기 안전성**: 모든 HTTP 콜백에서 `TWeakObjectPtr`로 객체 유효성 검사 → 위젯/서브시스템이
  파괴된 뒤 늦게 도착한 응답이 크래시를 내지 않음. `RequestId`로 오래된 응답 폐기.
- **thinking 모델 대응**: `gemma-4-E2B-it`는 답 전에 200~300토큰을 "생각"에 씀 → `max_tokens≥512`
  필수. 프록시는 요청 본문을 **변형 없이 통과**시켜 이 값을 보존.
- **관심사 분리**: TTS 로드 실패가 채팅 중계를 막지 않도록 격리(`/tts`만 503, 대화는 정상).
- **하드코딩 배제**: 서버 URL·모델명·타임아웃을 `UDeveloperSettings`(에디터 설정)로 노출.

---

## 6. 엔지니어링 도전기 — 한국어 TTS를 Windows에 올리기

"한국어 되는 로컬 TTS"를 붙이는 과정에서 실제로 부딪힌 문제들:

1. **엔진 오선택 정정** — 처음 고른 Kokoro-82M이 공식적으로 **한국어를 지원하지 않음**을 발견하고
   **MeloTTS**(한국어 정식 지원)로 교체.
2. **Python 3.12 실패** — MeloTTS의 구버전 의존성(tokenizers/fugashi)이 3.12용 wheel이 없어 설치 불가
   → **Python 3.11**로 전환하여 해결.
3. **의존성 충돌 3종 해결**:
   - `librosa`의 `pkg_resources` 누락 → `setuptools<80`로 복원.
   - 한국어 G2P가 요구하는 `eunjeon`(MeCab)이 Windows 빌드 실패, 대체재 `python-mecab-ko`는
     `mecab-python3`와 **대소문자 폴더 충돌** → **kiwipiepy를 `eunjeon` 인터페이스로 감싸는
     얇은 어댑터**(`melo_ko_patch.py`)로 우회.
4. **계약 기반 통합** — `/tts`의 입출력 규격(텍스트 → 16-bit PCM WAV)을 고정해, 엔진을 나중에
   XTTS 등으로 바꿔도 **UE 코드는 그대로** 쓰도록 설계.

> 결과: `POST /tts` → HTTP 200 / `audio/wav`(mono·16bit·44100Hz) / **CPU 실시간(~1x) 한국어 합성** 검증 완료.

---

## 7. 코드 맵

```
python_server/
  main.py            # FastAPI: /health, /v1/chat/completions(중계), /tts(합성), /stt(스텁)
  melo_ko_patch.py   # 한국어 G2P Windows 우회 (kiwipiepy→eunjeon 어댑터)
  tts_smoke.py       # 서버 없이 합성 검증 (out_kr.wav 생성)

ChattingNPC/Source/ChattingNPC/
  AIChat/
    LocalLLMSubsystem.*     # LLM 통신·세션·프롬프트 (대화)
    NPCVoiceSubsystem.*     # TTS 호출·WAV 재생 (음성)
    LocalLLMSettings.*      # 서버 URL/모델/TTS 설정 (에디터 노출)
    NPCSystemPromptBuilder.*# 프로필→시스템 프롬프트(안전 규칙)
  NPC/NPCCharacter.*        # 접근 감지·프로필 참조
  Player/ChattingNPCPlayerCharacter.*  # E키 상호작용·입력 모드
  UI/NPCChatWidget.*        # 대화 UI·응답/음성 트리거
```

---

## 8. 확장 로드맵
- **STT**: `faster-whisper`를 `/stt`에 연동 + UE 마이크 캡처 → 음성으로 말 걸기.
- **NPC별 목소리**: XTTS v2 음성 클로닝으로 캐릭터마다 다른 톤.
- **스트리밍 재생**: 문장 단위 합성으로 체감 지연 축소, 자막 타이핑과 음성 동기화.
- **장기 기억**: 대화 요약 엔드포인트로 오래된 맥락 압축.
