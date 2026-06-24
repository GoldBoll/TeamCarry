// TCAnimInstanceBase.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TCAnimInstanceBase.generated.h"


UCLASS()
class TEAMCARRY_API UTCAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 매 프레임 애니메이션 상태 갱신
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 블루프린트에 캐릭터 이동 속도 등록
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float Speed;
};
