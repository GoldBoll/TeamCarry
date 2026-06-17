# TeamCarry (Project A)

## 문서

- **[기여 가이드 (브랜치·커밋·PR·Revision Control 규칙)](./CONTRIBUTING.md)** — 팀원 필독
- [역할 분담](Docs/roles.md)

---

> **Carry The Glass(운반 시스템) × Moving Out(코어 루프) — 이삿짐 센터 협동 시뮬레이션**

2~4명이 호흡을 맞춰 무거운 가구를 **부수지 않고** 이삿짐 트럭까지 함께 옮기는 캐주얼 협동 게임. 한 명의 실수로 가구가 부딪혀 파손될 때 나오는 (역설적) 우정 파괴의 재미가 핵심.

---

## 게임 컨셉

| 항목 | 내용 |
|---|---|
| **게임명** | Project A / TeamCarry (2~4인 협동) |
| **장르** | 캐주얼, 협동(co-op) |
| **엔진** | Unreal Engine 5.8 |
| **언어** | C++ / Blueprint |
| **플랫폼** | PC |
| **레퍼런스** | Carry The Glass(운반), Moving Out(코어 루프) |

### 코어 판타지
- 다같이 호흡을 맞춰 큰 가구를 아슬아슬하게 운반하는 극도의 긴장감
- 한 명의 실수로 가구가 파손될 때 나오는 우정의 유대감(파괴)

### 핵심 메커니즘
- **협력 물리** — 소파·TV 같은 무거운 가구를 여러 명이 동시에 들어 트럭까지 운반 (필요 인원 충족 시 안정)
- **환경 퍼즐 & 파괴** — 운반 중 벽·다른 가구에 부딪히면 내구도 손상 → 점수 하락
- **스테이지 선택** — 구조가 다른 여러 집에서 체험
- **최종 결과** — 가구를 모두 옮기면 자동 결과, 점수 미달 시 패배

---

## 코어 루프

```
[메인 메뉴] → [캐릭터 선택] → [튜토리얼] → [스테이지 선택] → [인게임] → [최종 결과]
                                                                    │
                                                          점수 만족 → [계속]
                                                          점수 미달 → [패배]
```

---

## 가구 옮기기 시스템 (게임 정체성)

| 변수 | 타입 | 의미 |
|---|---|---|
| `Furniture_MaxHealth` / `CurrentHealth` | Float | 내구도 (충돌 시 차감) |
| `Furniture_RequiredPlayer` | Int | 안정적으로 들 최소 인원 |
| `Furniture_CurrentGrabbedPlayer` | Int | 현재 잡은 인원 |
| `Furniture_CombinedVelocity` | Vector | 플레이어 입력 합산 이동 벡터 |
| `Furniture_bIsGrabbed` | Bool | 잡힘 여부 |

> 멀티플레이 게임이므로 위 상태는 **서버 권위로 판정 후 복제**한다 (`Source/TeamCarry/Network/`).

---

## 소스 구조 (도메인 모듈)

```
Source/TeamCarry/
├── Core/        게임모드·게임스테이트, 공용 인터페이스/데이터, 결과·점수 판정
├── Player/      캐릭터, 이동, 상호작용(잡기) 입력
├── Furniture/   가구 본체, 다인 잡기, 내구도·파손  ← 게임 정체성
├── Network/     2~4인 복제·RPC, 서버 권위 동기화
├── Level/       스테이지(여러 집), 스테이지 선택, 트럭 도착 판정
└── UI/          메인메뉴·캐릭터선택·HUD·결과 화면
```

각 폴더의 `OWNER.txt`에 담당·용도·의존 방향 명시.

---

## 기술 스택

| 항목 | 내용 |
|---|---|
| Engine | Unreal Engine 5.8 |
| Language | C++ / Blueprint |
| Networking | Replication + RPC (서버 권위) |
| UI | UMG (Unreal Motion Graphics) |
| VCS | Git (GitHub) + 언리얼 에디터 Revision Control |
