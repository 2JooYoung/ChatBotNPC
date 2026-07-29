# ChattingNPC Notion Portfolio Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ChattingNPC의 WHY, 설계 판단, HOW, 문제 해결과 결과를 짧게 전달하는 노션 복붙용 프로젝트 포트폴리오를 만든다.

**Architecture:** 원고는 단일 Markdown 문서로 작성하고 기존 게임·서버·음성 설정 스크린샷을 재사용한다. UE, FastAPI, llama-server, MeloTTS의 관계는 별도 구조도 PNG로 만들어 원고에 삽입한다.

**Tech Stack:** Markdown, HTML/CSS 렌더링, PNG, 기존 프로젝트 스크린샷

## Global Constraints

- ChattingNPC 단일 프로젝트만 다룬다.
- 독자는 신입 개발자 채용 담당자와 실무 개발자다.
- 코드 전문과 클래스별 상세 설명은 제외한다.
- WHY → 설계 판단 → HOW → 문제 해결 → 결과 순서를 유지한다.
- 문장은 짧고 단정하게 작성한다.
- 확인되지 않은 성능 수치나 과장된 표현은 사용하지 않는다.
- 노션에 복사할 본문과 별도로 업로드할 이미지 파일을 제공한다.

---

### Task 1: 구조도 이미지 제작

**Files:**
- Create: `docs/assets/chattingnpc-architecture.png`
- Reference: `docs/architecture.html`
- Reference: `PORTFOLIO.md`

**Interfaces:**
- Consumes: `PORTFOLIO.md`에 기록된 실제 서버 관계와 요청 흐름
- Produces: 원고의 `전체 구조` 섹션에서 사용하는 `docs/assets/chattingnpc-architecture.png`

- [ ] **Step 1: 구조도 내용 확인**

다음 관계가 기존 문서와 일치하는지 확인한다.

```text
Player
  → Unreal Engine 5.7
      → FastAPI Proxy :8000
          ├→ llama-server :8080 → NPC dialogue text
          └→ MeloTTS → 16-bit PCM WAV
      ← dialogue text / WAV
  ← chat UI / NPC-positioned 3D voice
```

- [ ] **Step 2: 구조도 렌더링**

기존 `docs/architecture.html`의 전체 구조 영역을 1600px 이상 가로 해상도로 렌더링하고, 여백을 잘라 `docs/assets/chattingnpc-architecture.png`로 저장한다. 라벨은 `Unreal Engine`, `FastAPI Proxy`, `llama-server`, `MeloTTS`, `/v1/chat/completions`, `/tts`가 읽혀야 한다.

- [ ] **Step 3: 이미지 검증**

Run:

```powershell
Get-Item 'docs\assets\chattingnpc-architecture.png' | Select-Object Name,Length
```

Expected: 파일이 존재하고 크기가 0보다 크다.

이미지를 직접 열어 글자 잘림, 빈 화면, 지나치게 작은 라벨이 없는지 확인한다.

- [ ] **Step 4: 변경 내용 확인**

Run:

```powershell
git status --short -- 'docs/assets/chattingnpc-architecture.png'
```

Expected: 새 PNG 한 개만 표시된다.

- [ ] **Step 5: 구조도 커밋**

Run:

```powershell
git add -- 'docs/assets/chattingnpc-architecture.png'
git commit -m "docs: add ChattingNPC architecture diagram"
```

---

### Task 2: 노션용 포트폴리오 원고 작성

**Files:**
- Create: `docs/ChattingNPC-Notion-Portfolio.md`
- Reference: `PORTFOLIO.md`
- Reference: `IMPLEMENTATION_PROGRESS.md`
- Reference: `docs/assets/game-chat.jpg`
- Reference: `docs/assets/server-logs.jpg`
- Reference: `docs/assets/npc-voices.jpg`
- Reference: `docs/assets/chattingnpc-architecture.png`

**Interfaces:**
- Consumes: Task 1의 구조도 PNG와 기존 스크린샷 3장
- Produces: 노션에 복사할 최종 원고 `docs/ChattingNPC-Notion-Portfolio.md`

- [ ] **Step 1: 문서 골격 작성**

다음 제목을 정확한 순서로 작성한다.

```markdown
# ChattingNPC
## 한 줄 소개
## WHY — 왜 이 프로젝트를 만들었나
## 설계 목표
## 전체 구조 — 왜 서버를 둘로 나눴나
## HOW 1 — 대화가 만들어지는 흐름
## HOW 2 — 대사가 목소리가 되는 흐름
## NPC마다 다른 성격과 목소리
## 문제 해결
## 결과와 배운 점
## 기술 스택
```

- [ ] **Step 2: 실제 이미지 삽입**

다음 상대 경로를 해당 섹션에 한 번씩만 넣는다.

```markdown
![게임 내 NPC 대화 화면](assets/game-chat.jpg)
![ChattingNPC 전체 구조](assets/chattingnpc-architecture.png)
![LLM과 TTS 서버 실행 로그](assets/server-logs.jpg)
![NPC별 음성 설정](assets/npc-voices.jpg)
```

- [ ] **Step 3: WHY와 설계 판단 작성**

WHY에는 정해진 대사의 한계, 외부 API의 비용·인터넷·데이터 전송 문제, 완전 로컬 방식을 선택한 이유를 쓴다. 전체 구조에는 게임이 프록시 주소 하나만 알도록 한 이유와 llama-server/TTS를 분리해 얻은 교체 가능성·장애 격리를 설명한다.

- [ ] **Step 4: HOW와 안정성 작성**

텍스트 흐름은 `플레이어 입력 → NPC 프로필·최근 기록 조립 → FastAPI → llama-server → UI 표시`로 쓴다. 음성 흐름은 `확정된 대사 → /tts → MeloTTS → WAV → NPC 위치의 3D 재생`으로 쓴다. 늦은 응답 폐기, 객체 유효성 확인, TTS 실패 시 텍스트 대화 유지도 포함한다.

- [ ] **Step 5: 결과와 검증 근거 작성**

다음 확인된 결과만 사용한다.

```text
NPC 3명
외부 API 호출 0
NPC별 독립 대화 기록
MVP 체크리스트 16개 항목 통과
한국어 16-bit PCM WAV 생성 및 UE 재생
서버 장애 시 크래시 없이 오류 처리
```

- [ ] **Step 6: 링크와 금칙어 검증**

Run:

```powershell
rg -n "TBD|TODO|미정|추후 결정|혁신적|압도적" 'docs\ChattingNPC-Notion-Portfolio.md'
```

Expected: 출력 없음.

Run:

```powershell
$paths = @(
  'docs\assets\game-chat.jpg',
  'docs\assets\chattingnpc-architecture.png',
  'docs\assets\server-logs.jpg',
  'docs\assets\npc-voices.jpg'
)
$paths | ForEach-Object { "$_ : $(Test-Path $_)" }
```

Expected: 네 경로가 모두 `True`.

- [ ] **Step 7: 문서 자체 검토**

각 섹션이 다음 질문에 한 번씩 답하는지 확인한다.

```text
WHY: 무엇이 문제였고 왜 로컬 방식을 택했는가?
DESIGN: 왜 게임과 AI 엔진 사이에 프록시를 두었는가?
HOW: 텍스트와 음성이 각각 어떤 순서로 흐르는가?
ROBUSTNESS: 실패와 늦은 응답을 어떻게 격리했는가?
RESULT: 실제로 무엇을 검증했는가?
```

- [ ] **Step 8: 최종 변경 내용 확인**

Run:

```powershell
git diff --check
git status --short -- 'docs/ChattingNPC-Notion-Portfolio.md' 'docs/assets/chattingnpc-architecture.png'
```

Expected: 공백 오류가 없고 최종 원고와 구조도 PNG만 새 결과물로 표시된다.

- [ ] **Step 9: 원고 커밋**

Run:

```powershell
git add -- 'docs/ChattingNPC-Notion-Portfolio.md'
git commit -m "docs: add ChattingNPC Notion portfolio"
```
