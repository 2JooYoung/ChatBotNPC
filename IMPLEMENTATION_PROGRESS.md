# ChattingNPC — 로컬 LLM 다중 NPC 채팅 시스템 구현 진행 문서

로컬 llama-server(`http://127.0.0.1:8080/v1/chat/completions`, OpenAI 호환) 기반 다중 NPC
채팅 시스템 구현 진행 기록. 각 Phase 완료 시 이 문서에 보고서를 누적한다.

- 엔진: Unreal Engine 5.7
- 프로젝트: `ChattingNPC` (Third Person 템플릿)
- 상세 설계/로드맵: `C:\Users\user\.claude\plans\unreal-engine-5-inherited-sky.md`

## 진행 상황 요약

| Phase | 내용 | 상태 |
|---|---|---|
| **1** | 프로젝트 확인 및 기반 설계 | ✅ 완료 |
| 2 | Local LLM 통신 (C++ 코어) | ⬜ 대기 |
| 3 | NPC (ANPCCharacter) | ⬜ 대기 |
| 4 | 플레이어 상호작용 | ⬜ 대기 |
| 5 | 채팅 UI | ⬜ 대기 |
| 6 | 다중 NPC & 대화 기록 | ⬜ 대기 |
| 7 | 에디터 설정 & 테스트 | ⬜ 대기 |

---

# Phase 1 — 프로젝트 확인 및 기반 설계 (완료)

> 이 단계에서는 **코드를 생성/수정하지 않았다.** 프로젝트 구조 확인과 전체 구현 계획 수립만 수행.

## 1. 현재 프로젝트 구조
- Third Person 템플릿. `ChattingNPC/` 아래 `Content/`, `Config/`, `Saved/`, `Intermediate/` 등 존재.
- **`Source/` 폴더 없음** → 순수 Blueprint 프로젝트.
- 기본 맵: `Content/ThirdPerson/Lvl_ThirdPerson` (World Partition + `__ExternalActors__`/`__ExternalObjects__` 사용).
- Input 에셋: `Content/Input/IMC_Default`, `IMC_MouseLook`, `Actions/IA_Move`, `IA_Look`, `IA_MouseLook`, `IA_Jump`.
- 캐릭터 메시: `Content/Characters/Mannequins/Meshes/SKM_Manny_Simple` 등.

## 2. C++ 프로젝트 여부
- **아니오 — Blueprint 전용.** `.uproject`에 `Modules` 항목 없음, `Source/` 폴더 없음.
- → **C++ 프로젝트로 변환 필요** (승인된 방식: Source 스캐폴딩을 텍스트로 직접 생성).

## 3. 프로젝트 모듈 이름
- **`ChattingNPC`**.
- 근거: `Config/DefaultEngine.ini`
  `+ActiveGameNameRedirects=(OldGameName="/Script/TP_ThirdPersonBP",NewGameName="/Script/ChattingNPC")`.

## 4. 기존 Third Person Character 클래스
- `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` — **순수 Blueprint**.
- 엔진 `ACharacter`를 직접 상속(프로젝트 C++ 부모 없음).
- GameMode: `Content/ThirdPerson/Blueprints/BP_ThirdPersonGameMode` (기본 GameMode로 지정됨).

## 5. Enhanced Input 사용 여부
- **사용 중.** `Config/DefaultInput.ini`:
  - `DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput`
  - `DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent`
- 기존 IMC: `IMC_Default`(이동/점프/룩), `IMC_MouseLook`.
- → `IA_Interact`(E) 신규 생성 후 `IMC_Default`에 매핑 예정.

## 6. 새로 생성할 파일 (C++ — 코드로 직접 작성)
```
ChattingNPC.uproject                        (수정: Modules 항목 추가)
Source/
├─ ChattingNPC.Target.cs
├─ ChattingNPCEditor.Target.cs
└─ ChattingNPC/
   ├─ ChattingNPC.Build.cs
   ├─ ChattingNPC.h / .cpp                  (IMPLEMENT_PRIMARY_GAME_MODULE + 로그 카테고리)
   ├─ AIChat/
   │  ├─ NPCChatTypes.h                     (FNPCChatMessage, FNPCConversationSession)
   │  ├─ NPCDialogueContext.h               (FNPCDialogueContext)
   │  ├─ LocalLLMSettings.h / .cpp          (UDeveloperSettings)
   │  ├─ LocalLLMSubsystem.h / .cpp         (UGameInstanceSubsystem)
   │  └─ NPCSystemPromptBuilder.h / .cpp
   ├─ NPC/
   │  ├─ NPCProfileDataAsset.h / .cpp
   │  └─ NPCCharacter.h / .cpp
   ├─ Player/
   │  └─ ChattingNPCPlayerCharacter.h / .cpp
   └─ UI/
      └─ NPCChatWidget.h / .cpp
```
Build.cs 의존성: `Core, CoreUObject, Engine, InputCore, EnhancedInput, HTTP, Json, JsonUtilities, UMG, Slate, SlateCore, DeveloperSettings`.

## 7. 수정할 기존 파일
- `ChattingNPC.uproject` — `Modules` 배열 추가(Runtime/Default).
- (에디터 작업) `IMC_Default` — E 키 매핑 추가.
- (에디터 작업) `BP_ThirdPersonCharacter` — `AChattingNPCPlayerCharacter`로 Reparent.

## 8. Unreal Editor에서 직접 해야 할 작업 (.uasset — 코드로 생성 불가)
1. Source 생성 후: `.uproject` 우클릭 → **Generate Visual Studio project files** → 빌드.
2. `IA_Interact`(Input Action, Digital) 생성 → `IMC_Default`에 E 매핑.
3. `BP_ThirdPersonCharacter`를 `AChattingNPCPlayerCharacter`로 **Reparent**.
4. `BP_NPCCharacter`(ANPCCharacter 기반) 생성 + 메시 지정.
5. `DA_NPC_Blacksmith`, `DA_NPC_Merchant`, `DA_NPC_Guard`(UNPCProfileDataAsset) 생성.
6. `WBP_NPCChat`(UNPCChatWidget 기반), `WBP_PlayerChatMessage`, `WBP_NPCChatMessage` 제작.
7. 맵에 NPC 3명 배치 + 각기 다른 프로필 지정.
8. (권장) Project Settings에서 LocalLLM 설정값 확인.

## 9. 전체 구현 계획 (Phase 2~7 요약)
- **Phase 2**: Source 스캐폴딩 + LLM 통신 코어(타입/프로필/컨텍스트/프롬프트빌더/Subsystem). NPC·UI 미연결, 로그로 왕복 검증.
- **Phase 3**: `ANPCCharacter` + Sphere Collision 접근/이탈 감지, 프로필 참조, 초기 인사.
- **Phase 4**: `AChattingNPCPlayerCharacter` + IA_Interact(E) → 최근접 NPC 대화 시작, 이동/입력모드 제한·복구.
- **Phase 5**: `UNPCChatWidget` + BlueprintCallable 함수/BlueprintImplementableEvent, Subsystem 델리게이트 구독.
- **Phase 6**: NPC별 독립 세션, 최근 10개 제한, 프로필별 Temperature/MaxTokens, 세션 스위칭.
- **Phase 7**: DataAsset 3종 + 맵 배치 + 비교/장애/독립성/입력복구 테스트, 사용법 문서화, MVP 16항목 체크.

## 핵심 설계 원칙
- LLM은 대사 생성만 담당. 게임 상태 변경(아이템/퀘스트/이동 등)은 절대 LLM 응답으로 실행하지 않음.
- URL/모델명 하드코딩 금지(`ULocalLLMSettings`), 전용 로그 카테고리, Shipping에서 대화 로그 미출력.
- 모든 HTTP 비동기 콜백에서 UObject 유효성 검사, 늦은 응답은 RequestId로 폐기.

## 남아 있는 문제 / 주의
- Reparent 후 기존 BP의 Enhanced Input 바인딩과 C++ `SetupPlayerInputComponent` 공존 → Phase 4에서 조율 필요.
- Input Action/DataAsset/Blueprint/Widget은 반드시 에디터에서 수작업 생성.
- 한글(System Prompt/프로필)은 UTF-8 소스로 처리.

## 다음 Phase 작업
- **Phase 2 — Local LLM 통신 구현**: Source 스캐폴딩 생성 → 빌드 → 타입/프로필/컨텍스트/프롬프트빌더/Subsystem 구현 → 고정 메시지로 왕복 및 서버 장애 처리 검증.
