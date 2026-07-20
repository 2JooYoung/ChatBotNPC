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
| **2** | Local LLM 통신 (C++ 코어) | ✅ 완료 (빌드 성공) |
| **3** | NPC (ANPCCharacter) | ✅ 완료 (빌드 성공) |
| **4** | 플레이어 상호작용 | ✅ 완료 (빌드 성공) |
| **5** | 채팅 UI | ✅ 완료 (빌드 성공) |
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

---

# Phase 2 — Local LLM 통신 (완료, 빌드 성공)

> NPC/UI 미연결. C++ 코어와 llama-server 왕복만 구현. **UE 5.7.4에서 컴파일·링크 성공 확인.**

## 1. 생성한 파일
| 파일 | 역할 |
|---|---|
| `Source/ChattingNPC.Target.cs` | Game 타겟 |
| `Source/ChattingNPCEditor.Target.cs` | Editor 타겟 |
| `Source/ChattingNPC/ChattingNPC.Build.cs` | 모듈 의존성 + 모듈 루트 인클루드 경로 |
| `Source/ChattingNPC/ChattingNPC.h/.cpp` | `IMPLEMENT_PRIMARY_GAME_MODULE` + `LogChattingNPC` 로그 카테고리 |
| `AIChat/NPCChatTypes.h` | `FNPCChatMessage`, `FNPCConversationSession`(추가/트림/클리어), `NPCChatRoles` |
| `AIChat/NPCDialogueContext.h` | `FNPCDialogueContext`(테스트 기본값) |
| `AIChat/LocalLLMSettings.h/.cpp` | `ULocalLLMSettings`(UDeveloperSettings): URL/모델/기본값 |
| `AIChat/NPCSystemPromptBuilder.h/.cpp` | 프로필+컨텍스트 → System Prompt(§6 규칙 전체) |
| `AIChat/LocalLLMSubsystem.h/.cpp` | `ULocalLLMSubsystem`: HTTP/JSON/세션/딜리게이트 |
| `NPC/NPCProfileDataAsset.h` | `UNPCProfileDataAsset`(전 필드) |

## 2. 수정한 파일
- `ChattingNPC.uproject` — `Modules` 배열 추가(Runtime/Default) → C++ 프로젝트로 전환.

## 3. 구현한 기능
- OpenAI 호환 요청 본문 생성(model/temperature/max_tokens/stream=false/messages).
- System(프롬프트) + NPC별 저장 히스토리 + 현재 user 메시지 순서로 messages 조립.
- `choices[0].message.content` 파싱(각 단계 null/빈 배열/빈 값 방어).
- NPCId별 독립 세션 저장, 성공 시에만 user+assistant 저장 후 `MaxHistoryMessages`로 트림.
- 중복 요청 방지(NPCId InFlight), 요청 타임아웃(`SetTimeout`), **RequestId 기반 늦은 응답 폐기**.
- 비동기 콜백에서 `TWeakObjectPtr<ULocalLLMSubsystem>`로 유효성 검사 → Widget/Subsystem 파괴 후 안전.
- 성공/실패 델리게이트(`OnResponseReceived`, `OnRequestFailed`) — Blueprint 바인딩 가능.
- URL/모델명 비하드코딩(DeveloperSettings), 전용 로그 카테고리, Shipping에서 응답 본문 미로그.
- 콘솔 명령 `llm.test` 등록(Phase 2 검증용 고정 메시지 전송).

## 4. 핵심 코드 흐름
`SendMessageToNPC(Profile, PlayerMsg)` → 입력검증(널/공백/InFlight) → Settings 로드 →
SystemPrompt 빌드 + 히스토리 + user 메시지로 JSON 직렬화 → RequestId 발급 + InFlight 등록 →
비동기 POST → `HandleResponse`(InFlight 해제 → RequestId 일치 검사(불일치 시 폐기) →
연결/코드/파싱/빈값 검사 → 세션 저장·트림 → `OnResponseReceived` 브로드캐스트).

## 5. 컴파일 결과
- **성공.** `Build.bat ChattingNPCEditor Win64 Development` → `Result: Succeeded`.
- UHT 처리 정상(한글 리터럴/USTRUCT/UCLASS OK), `UnrealEditor-ChattingNPC.dll` 링크 완료.
- 초기 실패 1건(인클루드 경로) → `Build.cs`에 `PublicIncludePaths.Add(ModuleDirectory)` 추가로 해결.

## 6. 컴파일 못한 경우 — 해당 없음.

## 7. Unreal Editor에서 직접 해야 할 작업
1. `.uproject` 우클릭 → **Generate Visual Studio project files** (선택: IDE 사용 시).
2. 에디터 열면 모듈 리빌드 안내 → 빌드(또는 이미 빌드된 DLL 로드).
3. (선택) Project Settings > Plugins > **Local LLM**에서 URL/모델/기본값 확인.

## 8. 테스트 방법
1. 로컬 `llama-server` 기동(`http://127.0.0.1:8080`).
2. 에디터 실행 → `~`로 콘솔 → **`llm.test`** 입력.
3. Output Log(`LogChattingNPC`)에 NPC 응답 출력 확인.
4. 서버 종료 후 `llm.test` → "대화 서버에 연결할 수 없습니다." 경고 로그, **무크래시** 확인.

## 9. 현재 남아 있는 문제
- 아직 NPC/플레이어/UI 미연결(설계상 Phase 3~5). 런타임 왕복은 에디터에서 `llm.test`로만 검증 가능.
- `llm.test`는 개발용 콘솔 명령 — 이후 Phase에서 제거하거나 유지 결정.

## 10. 다음 Phase 작업
- **Phase 3 — NPC 구현**: `ANPCCharacter`(Sphere Collision 접근/이탈 감지, 프로필 참조, 초기 인사), `BP_NPCCharacter` 생성 절차 문서화.

---

# Phase 3 — NPC (ANPCCharacter) (완료, 빌드 성공)

## 1. 생성한 파일
| 파일 | 역할 |
|---|---|
| `NPC/NPCInteractorInterface.h` | `INPCInteractorInterface`(NotifyNPCEnteredRange/ExitedRange) — NPC↔플레이어 느슨한 결합 |
| `NPC/NPCCharacter.h/.cpp` | `ANPCCharacter`(ACharacter) + `USphereComponent` 상호작용 범위 |

## 2. 수정한 파일 — 없음.

## 3. 구현한 기능
- `USphereComponent`(Trigger 프로필, 반경 `InteractionRadius` 기본 220) 상호작용 범위, 캡슐에 부착.
- 오버랩 begin/end에서 **player-controlled Pawn만** 필터 → 인터페이스로 등록/해제 호출 + BP 훅.
- `Profile`(UNPCProfileDataAsset) 참조, `GetNPCId/GetDisplayName/GetRole/GetInitialGreeting/HasValidProfile`.
- `StartConversation/EndConversation`(중복 방지) + `bIsConversing`.
- **Tick 미사용**(오버랩 이벤트 기반). `OnConstruction`에서 반경 편집 반영.
- BP 훅: `OnPlayerEnteredRange/OnPlayerExitedRange/OnConversationStarted/OnConversationEnded`
  (→ "E로 대화" 프롬프트 표시 등에 사용).
- 프로필 미지정 시 BeginPlay에서 경고 로그(크래시 없음).

## 4. 핵심 코드 흐름
플레이어가 Sphere 진입 → `OnInteractionSphereBeginOverlap` → APawn·IsPlayerControlled 확인 →
`INPCInteractorInterface::NotifyNPCEnteredRange(this)`(플레이어가 자신의 근접 NPC 목록에 등록, Phase 4) +
`OnPlayerEnteredRange`(BP). 이탈 시 대칭.

## 5. 컴파일 결과
- **성공.** `Result: Succeeded`, DLL 재링크 완료. UHT 5 파일 생성.

## 6. 컴파일 못한 경우 — 해당 없음.

## 7. Unreal Editor에서 직접 해야 할 작업 (BP_NPCCharacter 생성)
> ⚠️ 아래는 코드로 생성 불가 — 에디터에서 직접 수행. (아직 생성되지 않음)
1. 에디터 재시작/컴파일로 `ANPCCharacter` 로드 확인(Live Coding 또는 재빌드).
2. Content Browser `Content/Blueprints/NPC/` 폴더 생성 → 우클릭 → Blueprint Class →
   부모 클래스로 **NPCCharacter** 검색·선택 → 이름 `BP_NPCCharacter`.
3. `BP_NPCCharacter` 열기 → Mesh(CharacterMesh0)에 `SKM_Manny_Simple`(또는 `SKM_Quinn_Simple`) 지정,
   필요 시 위치/회전(Z -90, Yaw -90) 및 AnimClass 설정(선택; 기본 T포즈 허용).
4. Details의 **Profile** 슬롯은 인스턴스별로 지정(Phase 7에서 DataAsset 3종 생성 후) — 지금은 비워도 됨.
5. (선택) `OnPlayerEnteredRange`/`OnPlayerExitedRange` 이벤트에 화면 프롬프트 위젯 표시 로직 연결.

## 8. 테스트 방법
- `BP_NPCCharacter`를 맵에 배치, PIE 실행 → 플레이어가 접근/이탈 시 로그/프롬프트로 오버랩 감지 확인.
  (전체 대화는 Phase 4~5 연결 후 검증.)

## 9. 현재 남아 있는 문제
- 플레이어 클래스가 아직 인터페이스를 구현하지 않음(Phase 4에서 구현) → 현재는 BP 훅만 동작.
- NPC 외형(메시/애님)은 에디터 수작업 필요.

## 10. 다음 Phase 작업
- **Phase 4 — 플레이어 상호작용**: `AChattingNPCPlayerCharacter`(INPCInteractorInterface 구현, 근접 NPC 목록·최근접 선택), `IA_Interact`(E) 바인딩, 대화 시작/종료, 이동·입력모드 제한/복구. `IMC_Default` E 매핑 + `BP_ThirdPersonCharacter` Reparent 절차.

---

# Phase 4 — 플레이어 상호작용 (완료, 빌드 성공)

> 이번 Phase부터 **런타임 확인용 온스크린 로그** 추가(`ChattingNPCScreenLog`, Shipping no-op). PIE에서 좌상단에 상태가 표시됨.

## 1. 생성한 파일
| 파일 | 역할 |
|---|---|
| `Player/ChattingNPCPlayerCharacter.h/.cpp` | `AChattingNPCPlayerCharacter`(ACharacter + `INPCInteractorInterface`) |

## 2. 수정한 파일
- `ChattingNPC.h/.cpp` — 온스크린 로그 헬퍼 `ChattingNPCScreenLog()` 추가.
- `NPC/NPCCharacter.cpp` — 오버랩 진입/이탈, 대화 시작/종료, BeginPlay에 로그 추가.

## 3. 구현한 기능
- `INPCInteractorInterface` 구현: NPC가 범위 진입/이탈 시 `NPCsInRange`에 등록/해제(로그).
- `IA_Interact`(E) → `OnInteract`: 대화 중이면 종료, 아니면 **최근접 NPC** 선택 후 시작(토글).
- `FindNearestNPC()` 거리제곱 비교, 무효 WeakPtr 스킵.
- `StartConversationWith`: 프로필 검증(미지정 시 경고·중단) → NPC `StartConversation()` → 입력 잠금.
- 입력 제한/복구 `SetConversationInputMode`: `SetIgnoreMoveInput/LookInput` + 커서 표시 + InputMode 전환(GameAndUI↔GameOnly).
- 범위 이탈 시 현재 대화 상대면 자동 종료.
- BP 훅 `OnConversationStartedWithNPC/OnConversationEndedWithNPC`(Phase 5에서 위젯 열기/닫기 연결).
- 모든 주요 지점에 UE_LOG + 온스크린 로그.

## 4. 핵심 코드 흐름
NPC 범위 진입 → NPC가 `NotifyNPCEnteredRange` 호출 → 목록 등록 + "E 키로 대화" 표시.
E 입력 → `OnInteract` → `FindNearestNPC` → `StartConversationWith`(프로필 검증 → 입력 잠금 → BP 훅).
E 재입력/범위 이탈/버튼(Phase 5) → `EndConversation`(입력 복구 → BP 훅).

## 5. 컴파일 결과
- **성공.** `Result: Succeeded`, DLL 재링크. UHT 정상.

## 6. 컴파일 못한 경우 — 해당 없음.

## 7. Unreal Editor에서 직접 해야 할 작업 (⚠️ 직접 수행 필요)
> **선행 조건**: 에디터에서 코드 리컴파일(또는 재시작)로 `AChattingNPCPlayerCharacter` 로드.

**A. IA_Interact 생성**
1. `Content/Input/Actions/`에서 우클릭 → Input → **Input Action** → 이름 `IA_Interact`.
2. `IA_Interact` 열기 → Value Type = **Digital (bool)** → 저장.

**B. IMC_Default에 E 매핑 추가**
1. `Content/Input/IMC_Default` 열기 → Mappings에 `+` → **IA_Interact** 선택.
2. 키를 **E** (Keyboard)로 지정 → 저장.

**C. BP_ThirdPersonCharacter Reparent**
1. `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` 열기.
2. 상단 메뉴 **File → Reparent Blueprint** → **ChattingNPCPlayerCharacter** 선택.
3. 컴파일 → 기존 Move/Look/Jump BP 이벤트가 그대로 유지되는지 확인(에러 시 재컴파일).
4. Class Defaults의 **Input > InteractAction** 슬롯에 `IA_Interact` 지정 → 저장.

> 참고: 기존 BP의 이동/점프 입력(BP EnhancedInputAction 이벤트)은 그대로 동작하며, C++는 IA_Interact만 바인딩한다.

## 8. 테스트 방법
1. `BP_NPCCharacter`를 맵에 배치(프로필은 Phase 7에서 지정 — 미지정 시 시작 시 빨간 로그로 안내).
2. PIE 실행 → NPC에 접근 시 좌상단 "[Player] E 키로 대화: …" 표시.
3. **E** → "[Player] 대화 시작: …" + 커서 표시 + 이동 잠금 확인.
4. **E** 재입력 → "[Player] 대화 종료 (이동/입력 복구)" + 이동 복구 확인.
5. 대화 중 NPC 범위 밖으로 걸어나가면 자동 종료(단, 이동이 잠겨 있으므로 이 경로는 Phase 5 UI 이후 실질 테스트).

## 9. 현재 남아 있는 문제
- 실제 채팅 UI 없음(Phase 5) — 현재 E는 대화 시작/종료 토글까지만. LLM 응답 표시는 Phase 5 연결 후.
- Reparent/IA_Interact 지정은 에디터 수작업 필수(미완료 시 E 입력 무반응, 경고 로그로 안내됨).

## 10. 다음 Phase 작업
- **Phase 5 — 채팅 UI**: `UNPCChatWidget`(BlueprintCallable 함수 + BlueprintImplementableEvent), Subsystem 델리게이트 구독, 플레이어 `OnConversationStartedWithNPC`에서 위젯 생성/표시, 메시지 송수신·로딩·오류 표시. `WBP_NPCChat` 등 제작 절차.

---

# Phase 5 — 채팅 UI (완료, 빌드 성공)

## 1. 생성한 파일
| 파일 | 역할 |
|---|---|
| `UI/NPCChatWidget.h/.cpp` | `UNPCChatWidget`(UUserWidget, Abstract) — 대화 UI 로직 + LLM 구독 |

## 2. 수정한 파일
- `Player/ChattingNPCPlayerCharacter.h/.cpp` — `ChatWidgetClass`/`ChatWidget` 추가, 대화 시작 시 위젯 생성·표시, 종료 시 해제, 닫기 요청 핸들러.

## 3. 구현한 기능
- **BlueprintCallable 함수**: `StartConversation/SetCurrentNPC/SendPlayerMessage/EndConversation/SetWaitingForResponse/AddPlayerMessage/AddNPCMessage/ShowErrorMessage` + `IsWaitingForResponse/GetCurrentNPC`.
- **BlueprintImplementableEvent**(WBP에서 구현): `OnConversationStarted(name,role)/OnPlayerMessageAdded/OnNPCMessageAdded/OnRequestStarted/OnRequestCompleted/OnRequestFailed/OnConversationEnded/OnWaitingForResponseChanged/OnErrorMessage`.
- Subsystem 델리게이트(`OnResponseReceived/OnRequestFailed`) 구독 → **현재 NPCId 일치할 때만** 처리(다른 NPC 응답 무시).
- 전송 검증: 빈 문자열/응답 대기 중 재전송 차단(오류 메시지).
- 성공 시 NPC 메시지 추가 + 대기 해제, 실패 시 오류 표시 + 대기 해제.
- 위젯 파괴/종료 시 구독 해제(`NativeDestruct`, `HandleConversationShutdown`) + `CancelRequest`로 늦은 응답 폐기.
- 닫기 버튼(`EndConversation`) → `OnCloseRequested` 브로드캐스트 → 플레이어가 UI 제거·입력 복구·NPC 종료 오케스트레이션(단일 경로, 루프 없음).

## 4. 핵심 코드 흐름
플레이어 `StartConversationWith` → 위젯 생성·`AddToViewport`·`OnCloseRequested` 바인딩 → `Widget.StartConversation(NPC)`
(SetCurrentNPC → LLM 구독 → `OnConversationStarted` → 초기 인사 `AddNPCMessage`).
WBP 전송버튼 → `SendPlayerMessage`(검증 → `AddPlayerMessage` → 대기 ON → `SendMessageToNPC`).
Subsystem 응답 → `HandleLLMResponse`(NPCId 일치 → `AddNPCMessage` + 대기 OFF + `OnRequestCompleted`).
닫기버튼/E/범위이탈 → 플레이어 `EndConversation` → `Widget.HandleConversationShutdown`(구독해제·CancelRequest·`OnConversationEnded`) → RemoveFromParent → 입력 복구.

## 5. 컴파일 결과
- **성공.** `Result: Succeeded`, DLL 재링크. UHT 정상.

## 6. 컴파일 못한 경우 — 해당 없음.

## 7. Unreal Editor에서 직접 해야 할 작업 (⚠️ 위젯 제작)
> 선행: 에디터 리컴파일로 `NPCChatWidget` 로드.

**A. WBP_NPCChat 생성 (Content/UI/)**
1. `Content/UI/` 폴더 생성 → 우클릭 → User Interface → **Widget Blueprint** → 부모 클래스 선택 창에서 **NPCChatWidget** 선택 → 이름 `WBP_NPCChat`.
2. Designer에서 다음 위젯 배치(이름 자유, 아래는 예시 역할):
   - NPC 이름 `TextBlock`, NPC 직업 `TextBlock`
   - 대화 내용 `ScrollBox`(안에 VerticalBox), 플레이어 입력 `EditableTextBox`(또는 MultiLine)
   - 전송 `Button`, 종료 `Button`, 로딩 표시(예: Throbber/Text), 오류 `TextBlock`
3. **Graph에서 이벤트 구현**(부모 함수/이벤트 오버라이드):
   - `OnConversationStarted(NPCName, NPCRole)` → 이름/직업 TextBlock에 Set Text.
   - `OnPlayerMessageAdded(Message)` / `OnNPCMessageAdded(Message)` → ScrollBox에 한 줄 위젯 추가(또는 텍스트 append) 후 `ScrollToEnd`.
   - `OnWaitingForResponseChanged(bWaiting)` → 로딩 표시 Visibility + 전송 Button `SetIsEnabled(!bWaiting)`.
   - `OnErrorMessage(ErrorMessage)` / `OnRequestFailed` → 오류 TextBlock에 Set Text + 표시.
   - `OnConversationEnded` → (선택) 정리/애니메이션.
4. **버튼 바인딩**:
   - 전송 Button `OnClicked` → `SendPlayerMessage`(EditableText의 GetText) → 이후 EditableText `SetText(빈값)`.
   - (선택) EditableText `OnTextCommitted`(Enter) → 동일 처리.
   - 종료 Button `OnClicked` → `EndConversation`.

**B. (선택) 메시지 줄 위젯** `WBP_PlayerChatMessage`, `WBP_NPCChatMessage` — 한 줄 표현 분리 시 제작.

**C. 플레이어에 위젯 클래스 지정**
1. `BP_ThirdPersonCharacter`(Phase 4에서 reparent 완료) 열기 → Class Defaults.
2. **NPC Interaction > Chat Widget Class**에 `WBP_NPCChat` 지정 → 저장.

## 8. 테스트 방법
1. llama-server 기동 + Phase 4 에디터 작업(IA_Interact/IMC/reparent/InteractAction) 완료 + WBP_NPCChat 지정.
2. `BP_NPCCharacter`에 프로필 지정(Phase 7 DataAsset) 후 맵 배치 → PIE.
3. NPC 접근 → E → 채팅창 열림 + NPC 이름/직업/초기 인사 표시 + 커서·이동잠금.
4. 메시지 입력 → 전송 → 대기 표시(전송 비활성) → NPC 응답 표시.
5. 서버 종료 후 전송 → "대화 서버에 연결할 수 없습니다." 오류 표시 + 무크래시.
6. 종료 버튼/E → 창 닫힘 + 이동/입력 복구.

## 9. 현재 남아 있는 문제
- WBP_NPCChat 레이아웃/이벤트 바인딩은 에디터 수작업 필수(미완료 시 대화 시작해도 화면에 아무것도 안 뜸 — 빨간 로그로 안내).
- NPC 프로필 3종(DataAsset) 및 맵 배치는 Phase 7.

## 10. 다음 Phase 작업
- **Phase 6 — 다중 NPC & 대화 기록**: NPCId별 독립 세션은 Subsystem에 이미 구현됨. Phase 6에서 다중 NPC 전환/기록 독립성/최근 10개 제한/프로필별 Temperature·MaxTokens 동작을 실사용 관점에서 점검·문서화하고, 필요 시 특정 NPC 기록 초기화 UI/명령 노출.

---

# 트러블슈팅 기록 (Phase 5 실사용 테스트 중)

## A. 플레이어 이동 불가 → 자립형 C++ 캐릭터로 해결
- `BP_ThirdPersonCharacter` 리페어런트가 저장 오류 → 대신 **C++ `AChattingNPCPlayerCharacter`를 자립형으로 확장**
  (SpringArm+Camera, `bOrientRotationToMovement`, Move/Look/Jump Enhanced Input 바인딩, BeginPlay에서 IMC 등록).
- 새 `BP_Player`(이 클래스 상속)를 GameMode Default Pawn으로 사용. BP 기본값에
  `IMC_Default / IA_Move / IA_Look / IA_Jump / IA_Interact / WBP_NPCChat` 지정 필요.

## B. 채팅창이 E 누르는 동안만 표시 → 트리거 이벤트 수정
- 원인: `ETriggerEvent::Triggered`는 디지털 키를 누르는 **매 프레임** 발화 → 토글이 프레임마다 반복.
- 수정: `ETriggerEvent::Started`(누르는 순간 1회)로 변경. → E 한 번에 열리고 유지됨. 닫기는 종료 버튼.

## C. LLM 응답 오류("서버 연결 실패"/"답변 생성 실패") → 원인 3중
1. **모델이 항상-생각(thinking) 모델** (`gemma-4-E2B-it`): 생각에 250~300토큰을 소모한 뒤 답변.
   `max_tokens=200`이면 생각만 하다 잘려 **`content`가 빈 문자열**(생각은 `reasoning_content`로 분리됨).
   → **MaxTokens 기본 512로 상향** (Settings/Profile 기본값 코드 수정, 빌드 완료).
2. **CPU 추론 35초+** → 기본 타임아웃 30초 초과. → **타임아웃 기본 120초로 상향**(코드), Project Settings에서 조정 가능.
3. **CUDA 초기화 실패**: RTX 3060 존재하나 드라이버 560.94 < CUDA 13.3 요구(580+). → CPU로 동작 중.
   **해결(2026-07-20)**: CUDA 12.4 빌드(`llama-b10038-bin-win-cuda-12.4-x64`)로 교체 → GPU 정상 동작 확인.
   - nvidia-smi에서 llama-server가 CUDA 앱으로 등록, VRAM 약 6.1GB 사용(모델 GPU 탑재 확인).
   - 동일 512토큰 테스트: **8.9초 / 37.5 tok/s** (CPU 35.7초 / 10.4 tok/s 대비 약 4배).
   - CUDA 13.3 폴더(`llama-b10034-bin-win-cuda-13.3-x64`)는 삭제해도 됨.
- **검증**: curl로 `max_tokens=512` 요청 → `finish_reason:"stop"` + 정상 한국어 답변 확인(35.7s CPU / 8.9s GPU).
- llama-server는 **기본 옵션으로 실행**(추가 플래그 불필요): `llama-b10038-bin-win-cuda-12.4-x64\llama-server.exe -m ..\model\gemma-4-E2B-it-Q8_0.gguf`
- 주의: 서버가 소리 없이 종료되는 경우 있음 → 게임 테스트 전 `curl http://127.0.0.1:8080/health` 확인.
