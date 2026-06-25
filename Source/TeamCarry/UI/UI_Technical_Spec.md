# 📦 이사 협동 게임 UI 최종 기획 및 기술 명세서

## 1. UI 전체 구조도 (화면 흐름도)
[S_Boot] 로고/인트로
   │
   ▼
[S_MainMenu] 타이틀 ──게임 종료(버튼/ESC)──▶ (O_Confirm) ──예──▶ 앱 종료
   ├─ 게임 시작 ───────────────▶ [S_SlotSelect] (세이브 슬롯 관리)
   ├─ 옵션 ────────────────────▶ (O_Settings)
   └─ 크레딧 ──────────────────▶ [크레딧 스크롤] ──Esc──▶ 복귀
        │
[S_SlotSelect] 게임 선택 슬롯 ──뒤로(ESC)──▶ [S_MainMenu]
   ├─ 데이터 슬롯 선택(이어하기) ─▶ (맵·진행도 로드) ─────┐
   └─ 빈 슬롯 선택(새 게임) ─▶ (새 세이브 할당) ────────┤
        │                                            │ (두 경우 모두 로비로 집결)
        ▼                                            ▼
[S_CharacterSelect] 캐릭터 선택/로비 ──뒤로──▶ (O_Confirm: 방 종료) ──▶ [S_MainMenu]
   └─ 전원 준비완료 → 호스트 '시작' 버튼 클릭 → 즉시 전환 (카운트다운 없음)
        │
        ├─ [새 게임 방] ──▶ [S_Tutorial] ──마지막 Step 완료/건너뛰기──┐
        │                                                          │
        └─ [이어하기 방] ──────────────────────────────────────────┤
                                                                   ▼
[S_StageSelect] 스테이지 선택 ──뒤로──▶ (O_Confirm) ──▶ [S_MainMenu]
   └─ 스테이지 선택 및 진입
        │
        ▼
[S_InGame] 인게임 HUD ──ESC──▶ (O_PauseMenu) ──[수동 저장]──▶ (O_SaveLoad)
   └─ 가구 전량 운반 완료
        │
        ▼
[S_Result] 최종 결과 ──확인──▶ [S_StageSelect] 또는 [S_MainMenu]

---

## 2. 화면(State) 목록 및 공통 오버레이 명세

### 화면(State) 목록
| State 명칭 | 종류 | 진입 경로 | 일시정지 | 비고 |
| :--- | :--- | :--- | :--- | :--- |
| **S_Boot** | 풀스크린 | 앱 실행 | X | 로고/인트로 연출 |
| **S_MainMenu** | 풀스크린 | Boot 완료 | X | 메인 타이틀 화면 |
| **S_SlotSelect** | 풀스크린 | 메인 메뉴 '게임 시작' | X | 가로형 카드 배치 (이어/새 게임 통합) |
| **S_CharacterSelect**| 풀스크린 | 슬롯 선택 직후 | X | 멀티 로비 겸 캐릭터 선택 |
| **S_Tutorial** | 풀스크린 | 새 게임 로비 시작 직후 | O | 수동 저장 비활성 |
| **S_StageSelect** | 풀스크린 | 튜토리얼 종료 / 이어하기 | X | 스테이지 목록 및 퀘스트 수주 |
| **S_InGame** | 풀스크린 | 스테이지 선택 진입 | O | 코어 루프 HUD |
| **S_Result** | 풀스크린 | 가구 전량 운반 완료 | X | 점수 정산 및 통계 |

### 공통 컴포넌트 명세 (Overlay)
* **통합 라우팅 규칙:** 풀스크린은 교체(Replace), 오버레이는 누적(Push/Pop) 방식. ESC는 '한 단계 뒤로/닫기' 통일.
* **O_Confirm:** 강제 모달 팝업. 기본 포커스는 '아니오'에 위치하여 오조작 방지.
* **O_Settings:** 오디오, 비디오, 키보드/패드 설정. 비디오 변경 시 15초 카운트다운 복구 로직.
* **O_PauseMenu:** `S_InGame`, `S_Tutorial`에서 ESC로 호출.
  * 항목: [계속하기], [설정], [수동 저장], [타이틀로 돌아가기]
  * 권한: 멀티플레이 동기화를 위해 [수동 저장]은 호스트(방장) 전용. 튜토리얼 맵에서는 강제 비활성화.
* **O_SaveLoad:** `O_PauseMenu`에서 [수동 저장] 선택 시 호출. 현재 상태를 슬롯에 덮어쓰거나 빈 슬롯에 기록.

---

## 3. 화면별 상세 기술 명세

### ① S_MainMenu (타이틀 화면)
* **역할:** 게임의 시작점.
* **구성:** 배경 루프, 로고, 리스트 버튼(`Btn_Start`, `Btn_Options`, `Btn_Credits`, `Btn_Quit`).
* **입력 라우팅:** `Btn_Start` 클릭 시 `S_SlotSelect`로 이동 (기존 이어하기/새 게임 분기 제거).

### ② S_SlotSelect (게임 선택 슬롯)
* **역할:** 세이브 데이터 진입 및 관리.
* **구성:** 맵 썸네일과 진행도가 포함된 큰 가로형 카드 슬롯. 내부에 명시적 삭제 버튼 `[ X ]` 존재.
* **선택 로직:**
  * **데이터 슬롯:** 이어하기. 데이터 로드 후 `Continue` 모드 방 생성.
  * **빈 슬롯:** `O_Confirm` 확인 후 새 게임 할당, `NewGame` 모드 방 생성.
* **삭제 로직:** `[ X ]` 클릭 시 `O_Confirm` 호출 후 데이터 삭제.

### ③ S_CharacterSelect (캐릭터 선택 및 멀티 로비)
* **역할:** 모든 플레이어 집결지.
* **구성:** 1P~4P 가로 슬롯, 방 코드, 호스트 전용 `Btn_Start`, 참가자 전용 `Btn_Ready`.
* **규칙:** 1P 호스트 고정. 중도 이탈 시 슬롯 번호 유지. 방 코드로 친구 초대.
* **시작 로직:** 모든 점유 슬롯 `bIsReady = true` 시 호스트 시작 버튼 활성화. 클릭 시 카운트다운 없이 즉시 전환.

### ④ S_Tutorial (튜토리얼)
* **역할:** `NewGame` 방 최초 1회 학습 맵.
* **동작:** 잡기/이동/놓기/적재. 1명만 성공해도 다음 단계 진행. 달성 시 `S_StageSelect` 직행.

### ⑤ S_StageSelect (스테이지 선택)
* **역할:** 플레이할 맵 선택 화면.
* **연동:** 맵 선택 후 진입 시 메모리에 로드된 세이브 데이터 갱신(저장) 후 `S_InGame` 씬 전환.

### ⑥ S_InGame (인게임 HUD)
* **역할:** 가구 운반 코어 루프.
* **연동:** 가구 액터 상태 변화를 델리게이트로 수신해 점수판(`HUD_Score`), 프롬프트, 내구도 게이지 갱신.
* **저장:** 게임 중 ESC를 눌러 호스트 권한으로 수동 저장.

### ⑦ S_Result (최종 결과)
* **정산:** 남은 내구도에 따라 0~5 등급. 점수 산정 후 `Team_Money` 표기. 확인 누르면 화면 이탈.

### ⑧ O_Confirm (확인 팝업)
* **구성:** 제목, 설명 텍스트, `Btn_Yes`, `Btn_No`.
* **동작 로직:** 생성 시 강제 모달(Modal). `Btn_No`에 기본 포커스. `Btn_Yes` 클릭 시 전달받은 콜백(Delegate) 실행 후 스택에서 Pop.

### ⑨ O_PauseMenu (인게임 메뉴)
* **구성:** `Btn_Resume`, `Btn_Settings`, `Btn_Save`, `Btn_ToTitle`.
* **입력 라우팅:** `Btn_Settings`는 `O_Settings`를 Push. `Btn_ToTitle`은 `O_Confirm` 호출.

### ⑩ O_Settings (설정 창)
* **동작 로직:** 값 변경 후 적용 클릭 시 `UGameUserSettings` 호출. 비디오 설정 시 15초 미확인 시 이전 상태로 원복하는 안전 로직 구현.

### ⑪ O_SaveLoad (저장/불러오기 창)
* **동작 로직:** 인게임 메뉴에서 호출되며, 현재 진행도의 덮어쓰기 및 빈 슬롯 기록 역할만 수행.

---

## 4. 통합 데이터 모델 및 저장 스키마

**저장 규칙 확정:** USaveGame 객체에는 플레이어 인원, 접속자 정보 등 유동 데이터 일절 미저장. 세계 상태(맵/진행도)만 기록.

| 데이터 구조 (도메인) | 유지 방식 | 포함되는 핵심 필드 (예시) |
| :--- | :--- | :--- |
| **세이브 슬롯 (디스크)** | 영구 보존 | `UnlockedStages`, `ClearedStages`, `CurrentTeamMoney`, `LastPlayedDate`, `SaveSlotIndex` |
| **멀티 로비 (메모리)** | 세션 내 유지 | `SlotIndex`, `PlayerName`, `SelectedCharacterID`, `bIsReady`, `RoomCode` |
| **가구/운반 (액터)** | 스테이지 내 유지 | `MaxHealth`, `CurrentHealth`, `RequiredPlayer`, `CurrentGrabbedPlayer`, `BaseScore` |
| **전역 설정 (로컬)** | 클라이언트별 | `MasterVolume`, `GraphicsQuality`, `InputBindings` |

---

## 5. 프로토타입 구현 아키텍처 규칙 (AI 코드 생성 가이드)

1. **UMockUIController (중앙 라우터) 필수 구현:**
   * 개별 UI 위젯 내부에서 `CreateWidget`이나 `AddToViewport`를 직접 호출하는 것을 절대 금지합니다.
   * 모든 화면의 전환은 전역 시스템인 `UMockUIController`의 `ReplaceState()`, `PushOverlay()`, `PopCurrentOverlay()` 함수를 통해서만 수행되어야 합니다.
2. **이벤트 주도 데이터 바인딩 (Delegate Mapping):**
   * UI 위젯은 백엔드 액터나 데이터를 매 프레임(Tick) 직접 참조하지 않습니다.
   * `S_InGame` 등의 위젯은 `UMockUIController` 내부의 아래 델리게이트들을 구독(Bind)하여 화면을 갱신해야 합니다.
     * `OnTeamMoneyUpdated(int32 NewTotalMoney)`
     * `OnDurabilityChanged(float Current, float Max)`
     * `OnInteractTargetChanged(AActor* Target, FString Key)`
     * `OnLobbySlotUpdated(int32 SlotIndex, FPlayerInfo Data)`