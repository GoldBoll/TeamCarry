// TCAnimInstanceBase.cpp


#include "Player/Animation/TCAnimInstanceBase.h"

void UTCAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Pawn 가져오기
	APawn* OwnerPawn = TryGetPawnOwner();

	// Pawn 유효성 확인
	if (IsValid(OwnerPawn))
	{
		// Z축을 제외한 평면(X, Y) 이동 속도만 계산해서 Speed 변수에 저장
		Speed = OwnerPawn->GetVelocity().Size2D();
	}
}
