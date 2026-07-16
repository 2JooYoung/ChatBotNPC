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
