// Fill out your copyright notice in the Description page of Project Settings.


#include "FurnitureStatComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CatchCharacter/Player/PlayerInteractComponent.h"

UFurnitureStatComponent::UFurnitureStatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // 캐릭터 이동 이후에 실행되도록 설정
	SetIsReplicatedByDefault(true);

	// Default values
	DefaultStats.MaxHealth = 100.f;
	DefaultStats.RequiredPlayer = 1;
	DefaultStats.BaseSpeed = 100.f;
	DefaultStats.CollisionDamageMultiplier = 10.f;

	CurrentHealth = 100.f;
	RequiredPlayer = 1;
	BaseSpeed = 100.f;
	CollisionDamageMultiplier = 10.f;
	
	CurrentGrabbedPlayer = 0;
	bIsGrabbed = false;

	CombinedVelocity = FVector::ZeroVector;
	CombinedForward = FVector::ZeroVector;
	ActivePullerCount = 0;
}

void UFurnitureStatComponent::BeginPlay()
// ... (BeginPlay unchanged)
{
	Super::BeginPlay();
	
	// 당연하지만 서버에서만 작동해야함
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 에디터에서 설정한 DefaultStats를 실제 런타임 변수에 적용
		CurrentHealth = DefaultStats.MaxHealth;
		RequiredPlayer = DefaultStats.RequiredPlayer;
		BaseSpeed = DefaultStats.BaseSpeed;
		CollisionDamageMultiplier = DefaultStats.CollisionDamageMultiplier;
	}
}

void UFurnitureStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
// ... (GetLifetimeReplicatedProps unchanged)
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFurnitureStatComponent, CurrentHealth);
	DOREPLIFETIME(UFurnitureStatComponent, RequiredPlayer);
	DOREPLIFETIME(UFurnitureStatComponent, BaseSpeed);
	DOREPLIFETIME(UFurnitureStatComponent, CollisionDamageMultiplier);
	DOREPLIFETIME(UFurnitureStatComponent, CurrentGrabbedPlayer);
	DOREPLIFETIME(UFurnitureStatComponent, bIsGrabbed);
}

void UFurnitureStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 서버에서 가구가 움직일 시 매틱마다 계산
	if (GetOwner() && GetOwner()->HasAuthority() && bIsGrabbed)
	{
		if (ActivePullerCount > 0 && !CombinedVelocity.IsNearlyZero())
		{
			float SpeedScale = GetMovementSpeedScale();
			
			// 1. 가구 이동 및 회전 계산
			FVector MoveDirection = CombinedVelocity.GetSafeNormal();
			FVector MoveDelta = MoveDirection * BaseSpeed * SpeedScale * DeltaTime;
			
			FVector OldFurnitureLoc = GetOwner()->GetActorLocation();
			FRotator OldFurnitureRot = GetOwner()->GetActorRotation();

			// 가구 회전 시도
			if (!CombinedForward.IsNearlyZero())
			{
				FVector TargetForward = CombinedForward.GetSafeNormal();
				TargetForward.Z = 0;
				if (!TargetForward.IsNearlyZero())
				{
					FRotator TargetRotation = TargetForward.Rotation();
					FRotator NewRotation = FMath::RInterpTo(OldFurnitureRot, TargetRotation, DeltaTime, 5.0f);
					GetOwner()->SetActorRotation(NewRotation);
				}
			}

			// 가구 이동 시도 (충돌 체크)
			FHitResult FurnitureHit;
			GetOwner()->AddActorWorldOffset(MoveDelta, true, &FurnitureHit);
			
			FVector ActualFurnitureDelta = GetOwner()->GetActorLocation() - OldFurnitureLoc;
			FVector FurnitureVelocity = ActualFurnitureDelta / DeltaTime;

			// 2. 플레이어들을 가구의 새로운 위치에 맞춰 강제 고정
			FTransform NewFurnitureTransform = GetOwner()->GetActorTransform();

			for (int i = 0; i < GrabbingPlayers.Num(); ++i)
			{
				UPlayerInteractComponent* PlayerComp = GrabbingPlayers[i];
				if (PlayerComp && PlayerComp->GetOwner())
				{
					AActor* PlayerActor = PlayerComp->GetOwner();
					FVector TargetWorldLoc = NewFurnitureTransform.TransformPosition(GrabbingPlayerRelativeLocations[i]);
					
					FVector OldPlayerLoc = PlayerActor->GetActorLocation();
					
					// 플레이어를 가구 위치에 맞춰 강제 이동 (충돌 체크 포함)
					FHitResult PlayerHit;
					PlayerActor->SetActorLocation(TargetWorldLoc, true, &PlayerHit);

					FVector ActualPlayerDelta = PlayerActor->GetActorLocation() - OldPlayerLoc;

					// 동기화 보정: 플레이어가 막혔다면 가구를 그만큼 반대 방향으로 밀어내어 동기화
					if (PlayerHit.bBlockingHit)
					{
						FVector Correction = ActualPlayerDelta - (TargetWorldLoc - OldPlayerLoc);
						GetOwner()->AddActorWorldOffset(Correction, true);
						// 가구가 밀려났으므로 트랜스폼/속도 갱신
						NewFurnitureTransform = GetOwner()->GetActorTransform();
						FurnitureVelocity = (GetOwner()->GetActorLocation() - OldFurnitureLoc) / DeltaTime;
					}

					// 애니메이션 출력을 위해 캐릭터의 속도와 입력 방향 설정
					ACharacter* Char = Cast<ACharacter>(PlayerActor);
					if (Char && Char->GetCharacterMovement())
					{
						// 속도 동기화
						Char->GetCharacterMovement()->Velocity = FurnitureVelocity;
						
						// 입력을 추가하여 AnimBP가 가속도를 계산하게 함
						Char->AddMovementInput(MoveDirection, 1.0f);
					}
				}
			}
		}
		else
		{
			// 이동 입력이 없을 때 속도 초기화
			for (UPlayerInteractComponent* PlayerComp : GrabbingPlayers)
			{
				if (PlayerComp && PlayerComp->GetOwner())
				{
					ACharacter* Char = Cast<ACharacter>(PlayerComp->GetOwner());
					if (Char && Char->GetCharacterMovement())
					{
						Char->GetCharacterMovement()->Velocity = FVector::ZeroVector;
					}
				}
			}
		}
		
		// 데이터 초기화
		CombinedVelocity = FVector::ZeroVector;
		CombinedForward = FVector::ZeroVector;
		ActivePullerCount = 0;
	}
}

void UFurnitureStatComponent::TakeCollisionDamage(float ImpactMagnitude)
// ... (TakeCollisionDamage unchanged)
{
	if (GetOwner() && !GetOwner()->HasAuthority()) return;

	float Damage = ImpactMagnitude * CollisionDamageMultiplier;
	CurrentHealth -= Damage;
	
	UE_LOG(LogTemp, Warning, TEXT("[Server] %s Take Damage: %f. Current Health: %f"), *GetOwner()->GetName(), Damage, CurrentHealth);

	if (CurrentHealth <= 0.f)
	{
		CurrentHealth = 0.f;
		OnFurnitureDestroyed.Broadcast();
		
		UE_LOG(LogTemp, Error, TEXT("[Server] %s Destroyed!"), *GetOwner()->GetName());
		GetOwner()->SetActorEnableCollision(false);
	}
}

void UFurnitureStatComponent::AddGrabber(UPlayerInteractComponent* PlayerComp)
{
	// 서버에서만 동작해야함
	if (GetOwner() && !GetOwner()->HasAuthority()) return;

	if (PlayerComp && !GrabbingPlayers.Contains(PlayerComp))
	{
		GrabbingPlayers.Add(PlayerComp);
		
		// 상대 좌표 계산 및 저장
		FVector RelativeLoc = GetOwner()->GetActorTransform().InverseTransformPosition(PlayerComp->GetOwner()->GetActorLocation());
		GrabbingPlayerRelativeLocations.Add(RelativeLoc);

		CurrentGrabbedPlayer = GrabbingPlayers.Num();
		bIsGrabbed = true;
	}
}

void UFurnitureStatComponent::RemoveGrabber(UPlayerInteractComponent* PlayerComp)
{
	if (GetOwner() && !GetOwner()->HasAuthority()) return;

	if (PlayerComp)
	{
		int32 Index = GrabbingPlayers.Find(PlayerComp);
		if (Index != INDEX_NONE)
		{
			GrabbingPlayers.RemoveAt(Index);
			GrabbingPlayerRelativeLocations.RemoveAt(Index);
			
			CurrentGrabbedPlayer = GrabbingPlayers.Num();
			if (CurrentGrabbedPlayer == 0)
			{
				bIsGrabbed = false;
			}
		}
	}
}

void UFurnitureStatComponent::AddMovementInput(FVector MovementInput, FVector PlayerForward)
{
	if (GetOwner() && !GetOwner()->HasAuthority()) return;
	
	// 입력이 유효한 경우에만(당기는 중일 때만) 카운트 및 방향 합산
	if (MovementInput.SizeSquared() > 0.0001f)
	{
		CombinedVelocity += MovementInput;
		CombinedForward += PlayerForward;
		ActivePullerCount++;
	}
}

float UFurnitureStatComponent::GetMovementSpeedScale() const
{
	if (ActivePullerCount == 0) return 0.f;
	
	if (ActivePullerCount >= RequiredPlayer)
	{
		return 1.0f;
	}
	else
	{
		// 한 명만 당기거나 요구 인원보다 적을 경우 매우 느리게 설정
		return 0.1f;
	}
}

