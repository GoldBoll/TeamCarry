// Fill out your copyright notice in the Description page of Project Settings.

#include "FurnitureGrabSystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "CatchCharacter/Furniture/FurnitureStat.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"

UFurnitureGrabSystem::UFurnitureGrabSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFurnitureGrabSystem::BeginPlay()
{
	Super::BeginPlay();
}

void UFurnitureGrabSystem::Setup(UStaticMeshComponent* InMesh, UFurnitureStat* InStat)
{
	FurnitureMesh = InMesh;
	FurnitureStat = InStat;
}

void UFurnitureGrabSystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFurnitureGrabSystem, GrabbedPlayers);
	DOREPLIFETIME(UFurnitureGrabSystem, ServerLocation);
	DOREPLIFETIME(UFurnitureGrabSystem, ServerRotation);
}

void UFurnitureGrabSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 서버에서 매틱마다 계산
	if (GetOwner() && GetOwner()->HasAuthority() && GrabbedPlayers.Num() > 0)
	{
		// 가구를 이동시키며 캐릭터에게 이동, 회전해야할 값을 보냄
		HandleMovement(DeltaTime);
	}
	// 클라이언트에서 보간하여 부드럽게 움직이도록 함
	else if (GetOwner() && !GetOwner()->HasAuthority() && GrabbedPlayers.Num() > 0)
	{
		UpdateClientInterpolation(DeltaTime);
	}
	// 아무도 가구를 잡고 있지 않을 때 방치된 상태 (물리 동기화용)
	else if (GetOwner() && !GetOwner()->HasAuthority() && GrabbedPlayers.Num() == 0)
	{
		// 출발점 갱신
		PreviousClientLoc = GetOwner()->GetActorLocation();
		PreviousClientRot = GetOwner()->GetActorRotation();
	}
}

void UFurnitureGrabSystem::Grab(ACharacter* Grabber, FVector height, UPrimitiveComponent* GrabberComponent)
{
	// Grab은 서버에서만 작업해야한다.
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Grabber || GrabbedPlayers.Contains(Grabber)) 
		return;

	// 필요 인원까지는 다 잡을 수있도록 제한
	if (FurnitureStat && GrabbedPlayers.Num() >= FurnitureStat->GetRequiredPlayer())
	{
		return;
	}

	// 처음 잡는 사람이면 가구는 물리끄고 특정위치로 이동시켜준다.
	if (GrabbedPlayers.Num() == 0 && FurnitureMesh)
	{
		FurnitureMesh->SetSimulatePhysics(false);
		FurnitureMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		GetOwner()->AddActorWorldOffset(height);
	}

	// 서로 잡아끄는 가구끼리 충돌을 제거시킨다
	Grabber->MoveIgnoreActorAdd(GetOwner());
	if (FurnitureMesh && Grabber->GetCapsuleComponent())
	{
		FurnitureMesh->IgnoreComponentWhenMoving(Grabber->GetCapsuleComponent(), true);
		Grabber->GetCapsuleComponent()->IgnoreComponentWhenMoving(FurnitureMesh, true);
	}

	// 잡고있는 플레이어 등록
	GrabbedPlayers.Add(Grabber);

	// 현재 잡은 플레이어의 위치를 기록
	PreviousPlayerLocations.Add(Grabber, Grabber->GetActorLocation());
	PreviousPlayerYaws.Add(Grabber, Grabber->GetActorRotation().Yaw);

	// 플레이어와 가구사이의 거리와 각도 저장
	InitialVectors.Add(Grabber, GetOwner()->GetActorLocation() - Grabber->GetActorLocation());
	InitialYaws.Add(Grabber, Grabber->GetActorRotation().Yaw);

	// 가구스텟에 현재 잡고있는 플레이어 반영
	if (FurnitureStat)
	{
		FurnitureStat->UpdateGrabbedPlayers(GrabbedPlayers.Num());
	}
}

void UFurnitureGrabSystem::Release(ACharacter* Grabber)
{
	// 서버에서만 작동함
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Grabber || !GrabbedPlayers.Contains(Grabber)) 
		return;

	Grabber->MoveIgnoreActorRemove(GetOwner());
	
	if (FurnitureMesh && Grabber->GetCapsuleComponent())
	{
		FurnitureMesh->IgnoreComponentWhenMoving(Grabber->GetCapsuleComponent(), false);
		Grabber->GetCapsuleComponent()->IgnoreComponentWhenMoving(FurnitureMesh, false);
	}

	GrabbedPlayers.Remove(Grabber);
	PreviousPlayerLocations.Remove(Grabber);
	PreviousPlayerYaws.Remove(Grabber);
	InitialVectors.Remove(Grabber);
	InitialYaws.Remove(Grabber);

	if (GrabbedPlayers.Num() == 0 && FurnitureMesh)
	{
		FurnitureMesh->SetSimulatePhysics(true);
		FurnitureMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	}

	if (FurnitureStat)
	{
		FurnitureStat->UpdateGrabbedPlayers(GrabbedPlayers.Num());
	}
}

void UFurnitureGrabSystem::HandleMovement(float DeltaTime)
{
	// 해당 연산은 서버에서 이루어져야함
	if (!GetOwner() || !GetOwner()->HasAuthority() || !FurnitureStat)
		return;

	// 이동되어야할 총합
	FVector CombinedDeltaLoc = FVector::ZeroVector;
	float CombinedDeltaYaw = 0.0f;
	
	// 각 플레이어가 가구에 요청한 개별 이동량을 저장할 맵
	TMap<ACharacter*, FVector> PlayerDeltaLocs;
	TMap<ACharacter*, float> PlayerDeltaYaws;

	FVector OldFurnitureLoc = GetOwner()->GetActorLocation();

	for (ACharacter* Player : GrabbedPlayers)
	{
		if (Player && PreviousPlayerLocations.Contains(Player) && PreviousPlayerYaws.Contains(Player) && InitialVectors.Contains(Player))
		{
			// 현재 가구가 이동하여야할 양 계산
			float CurrentYaw = Player->GetActorRotation().Yaw;
			float PreviousYaw = PreviousPlayerYaws[Player];
			float PlayerDeltaYaw = FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw);

			// 플레이어가 처음 잡았을 때와 지금까지 회전한 각도
			float TotalYawChange = FMath::FindDeltaAngleDegrees(InitialYaws[Player], CurrentYaw);

			// 플레이어 회전에 영향받아 추가이동량
			FVector CurrentVectorToFurniture = InitialVectors[Player].RotateAngleAxis(TotalYawChange, FVector::UpVector);
			FVector DesiredFurnitureLoc = Player->GetActorLocation() + CurrentVectorToFurniture;

			FVector PlayerDeltaLoc = DesiredFurnitureLoc - OldFurnitureLoc;
			
			// 계산된 플레이어별 요청량을 맵에 저장
			PlayerDeltaLocs.Add(Player, PlayerDeltaLoc);
			PlayerDeltaYaws.Add(Player, PlayerDeltaYaw);

			CombinedDeltaLoc += PlayerDeltaLoc;
			CombinedDeltaYaw += PlayerDeltaYaw;
		}
	}

	int32 NumPlayers = GrabbedPlayers.Num();
	if (NumPlayers > 0)
	{
		// 끌어당기는 인원수에 따른 이동속도 제어...일단은 이렇게하고 혹시모름
		int32 Divider = NumPlayers;

		FVector AverageDeltaLoc = CombinedDeltaLoc / Divider;
		float AverageDeltaYaw = CombinedDeltaYaw / Divider;

		FRotator OldFurnitureRot = GetOwner()->GetActorRotation();

		// 이동해야할 위치
		// 회전은 Yaw만 할거임
		FVector TargetLocation = OldFurnitureLoc + AverageDeltaLoc;
		FRotator TargetRotation = OldFurnitureRot;
		TargetRotation.Yaw += AverageDeltaYaw;
		
		// 가구는 플레이어들의 평균 이동량만큼 이동
		GetOwner()->SetActorLocationAndRotation(TargetLocation, TargetRotation, true);

		// 가구가 벽에 부딪쳐서 이동하지 못한 경우의 실제 이동량 체크해보기
		FVector ActualDeltaLoc = GetOwner()->GetActorLocation() - OldFurnitureLoc;
		float ActualDeltaYaw = FMath::FindDeltaAngleDegrees(OldFurnitureRot.Yaw, GetOwner()->GetActorRotation().Yaw);

		// 플레이어 이동시킴(실제 이동량으로 움직이게)
		for (ACharacter* Player : GrabbedPlayers)
		{
			if (PlayerDeltaLocs.Contains(Player))
			{
				// 내가 밀려고했던 만큼 가구가 못갔다면 그 차이만큼 강제로 이동시킴
				FVector LocCorrection = ActualDeltaLoc - PlayerDeltaLocs[Player];
				float YawCorrection = ActualDeltaYaw - PlayerDeltaYaws[Player];
				if (!LocCorrection.IsNearlyZero(0.1f))
				{
					Player->AddActorWorldOffset(LocCorrection, true);
				}
				if (FMath::Abs(YawCorrection) > 0.1f)
				{
					Player->AddActorWorldRotation(FRotator(0.0f, YawCorrection, 0.0f));
				}

				// 로컬 클라이언트는 무브먼트의 고집이 꺾이게 하기위해 강제 위치/회전 브로드캐스트
				//if (!LocCorrection.IsNearlyZero(0.1f) || FMath::Abs(YawCorrection) > 0.1f)
				//{
				//	Multicast_ForcePlayerPositionAndRotation(Player, Player->GetActorLocation(), Player->GetActorRotation().Yaw);
				//}
			}
		}
	}

	// 모든 잡고 있는 플레이어의 위치기록
	for (ACharacter* Player : GrabbedPlayers)
	{
		if (Player)
		{
			// 다음 연산을 위해 위치설정
			PreviousPlayerLocations.Add(Player, Player->GetActorLocation());
			PreviousPlayerYaws.Add(Player, Player->GetActorRotation().Yaw);
		}
	}

	// 서버에서의 가구 최종 위치를 클라이언트 보간용 변수에 복사
	ServerLocation = GetOwner()->GetActorLocation();
	ServerRotation = GetOwner()->GetActorRotation();
}

void UFurnitureGrabSystem::OnRep_GrabbedPlayers() 
{
	// 누군가 잡은 상태가 되었을 경우
	if (GrabbedPlayers.Num() > 0)
	{
		if (FurnitureMesh)
		{
			FurnitureMesh->SetSimulatePhysics(false);
			FurnitureMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}

		for (ACharacter* Player : GrabbedPlayers)
		{
			if (Player && FurnitureMesh && Player->GetCapsuleComponent())
			{
				Player->MoveIgnoreActorAdd(GetOwner());
				FurnitureMesh->IgnoreComponentWhenMoving(Player->GetCapsuleComponent(), true);
				Player->GetCapsuleComponent()->IgnoreComponentWhenMoving(FurnitureMesh, true);
			}
		}
	}
	// 아무도 가구를 잡지 않았을 경우
	else
	{
		if (FurnitureMesh)
		{
			FurnitureMesh->SetSimulatePhysics(true);
			FurnitureMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		}

		if (GetWorld())
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				if (ACharacter* LocalPlayer = Cast<ACharacter>(PC->GetPawn()))
				{
					LocalPlayer->MoveIgnoreActorRemove(GetOwner());
					if (FurnitureMesh && LocalPlayer->GetCapsuleComponent())
					{
						FurnitureMesh->IgnoreComponentWhenMoving(LocalPlayer->GetCapsuleComponent(), false);
						LocalPlayer->GetCapsuleComponent()->IgnoreComponentWhenMoving(FurnitureMesh, false);
					}
				}
			}
		}
	}
}

void UFurnitureGrabSystem::UpdateClientInterpolation(float DeltaTime)
{
	float InterpSpeed = 30.0f; 

	// 서버에서 보내준 목적지까지 보간
	FVector EstimatedLoc = FMath::VInterpTo(PreviousClientLoc, ServerLocation, DeltaTime, InterpSpeed);
	FRotator EstimatedRot = FMath::RInterpTo(PreviousClientRot, ServerRotation, DeltaTime, InterpSpeed);

	if (GetOwner())
	{
		GetOwner()->SetActorLocationAndRotation(EstimatedLoc, EstimatedRot, false);
	}

	// 다음 프레임을 위해 현재 위치 백업
	PreviousClientLoc = EstimatedLoc;
	PreviousClientRot = EstimatedRot;
}

void UFurnitureGrabSystem::Multicast_ForcePlayerPositionAndRotation_Implementation(ACharacter* PlayerToTarget, FVector LocToSet, float YawToSet)
{
	// 무브먼트 컴포넌트가 서버에서 전송한 회전을 무시하고 이동하므로 강제로 셋팅
	if (PlayerToTarget && PlayerToTarget->IsLocallyControlled() && !GetOwner()->HasAuthority())
	{
		FRotator TargetRot = PlayerToTarget->GetActorRotation();
		TargetRot.Yaw = YawToSet;
		PlayerToTarget->SetActorLocationAndRotation(LocToSet, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
	}
}
