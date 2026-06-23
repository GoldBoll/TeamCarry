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

	// 보간을 위한 작업
	//const static float ActorNetUpdateFrequency = 100.f;
	//SetNetUpdateFrequency(ActorNetUpdateFrequency);
	//// 1초에 100번씩 액터 레플리케이션 시도
	//NetUpdatePeriod = 1 / GetNetUpdateFrequency();
	//// 주기 = 1 / 주파수
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

	// 서버에서 매틱마다 연산
	if (GetOwner() && GetOwner()->HasAuthority() && GrabbedPlayers.Num() > 0)
	{
		// 가구가 이동을 하며 캐릭터에게 이동, 회전해야할 값을 보냄
		HandleMovement(DeltaTime);
	}
	// 클라에서 보간하여 부드럽게 움직이도록 함
	else if (GetOwner() && !GetOwner()->HasAuthority() && GrabbedPlayers.Num() > 0)
	{
		UpdateClientInterpolation(DeltaTime);
	}
	// 아무도 가구를 잡지 않고 방치된 상태일 때 (물리 낙하 등)
	else if (GetOwner() && !GetOwner()->HasAuthority() && GrabbedPlayers.Num() == 0)
	{
		// 출발점 갱신
		PreviousClientLoc = GetOwner()->GetActorLocation();
		PreviousClientRot = GetOwner()->GetActorRotation();
	}

	//for (ACharacter* Player : GrabbedPlayers)
	//{
	//	Player->AddActorWorldRotation(FRotator(0.0f, 1.f, 0.0f));
	//}
}

void UFurnitureGrabSystem::Grab(ACharacter* Grabber, FVector height, UPrimitiveComponent* GrabberComponent)
{
	// Grab은 서버에서만 작업해야한다.
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Grabber || GrabbedPlayers.Contains(Grabber)) 
		return;

	// 필요 인원까지만 잡을 수 있도록 제한
	if (FurnitureStat && GrabbedPlayers.Num() >= FurnitureStat->GetRequiredPlayer())
	{
		return;
	}

	// 들어올려지는 가구는 물리를 끄고 일정높이 들어올려준다.
	if (GrabbedPlayers.Num() == 0 && FurnitureMesh)
	{
		FurnitureMesh->SetSimulatePhysics(false);
		FurnitureMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		GetOwner()->AddActorWorldOffset(height);
	}

	// 잡은 사람과 가구끼리 충돌을 제거시킨다.
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
	PreviousControlYaws.Add(Grabber, Grabber->GetControlRotation().Yaw);

	// 플레이어와 가구사이의 거리와 각도 저장
	InitialVectors.Add(Grabber, GetOwner()->GetActorLocation() - Grabber->GetActorLocation());
	InitialYaws.Add(Grabber, GetOwner()->GetActorRotation().Yaw);

	// 가구 스텟에 현재 잡고있는 플레이어 반영
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
	PreviousControlYaws.Remove(Grabber);
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

	// 이동제어할 총합
	FVector CombinedDeltaLoc = FVector::ZeroVector;
	float CombinedDeltaYaw = 0.0f;
	
	// 각 플레이어별 회전값 변화를 저장할 맵
	TMap<ACharacter*, float> PlayerDeltaYaws;

	FVector OldFurnitureLoc = GetOwner()->GetActorLocation();
	FRotator OldFurnitureRot = GetOwner()->GetActorRotation();

	for (ACharacter* Player : GrabbedPlayers)
	{
		if (Player && InitialVectors.Contains(Player))
		{
			UCharacterMovementComponent* CMC = Player->GetCharacterMovement();
			if (CMC)
			{
				// 이동 방향 (가속도 벡터의 정규화 사용)
				FVector InputDir = CMC->GetCurrentAcceleration().GetSafeNormal();

				// 캐릭터의 원래 최대 속도를 기준으로 미는 힘 결정
				float PlayerPushPower = CMC->MaxWalkSpeed;

				// 가구중심에서 플레이어까지의 방향 벡터
				FVector VectorFromCenter = Player->GetActorLocation() - OldFurnitureLoc;

				// 직선 이동량 누적
				CombinedDeltaLoc += InputDir * PlayerPushPower;

				// 위치 벡터와 미는 힘 벡터의 외적의 Z성분 = 회전력
				FVector Torque = FVector::CrossProduct(VectorFromCenter, InputDir * PlayerPushPower);
				CombinedDeltaYaw += Torque.Z * 0.05f;

				// 마우스 회전 입력 감지
				float PlayerDeltaYaw = 0.0f;
				if (PreviousControlYaws.Contains(Player))
				{
					float CurrentControlYaw = Player->GetControlRotation().Yaw;
					float PreviousControlYaw = PreviousControlYaws[Player];
					PlayerDeltaYaw = FMath::FindDeltaAngleDegrees(PreviousControlYaw, CurrentControlYaw);
				}
				PlayerDeltaYaws.Add(Player, PlayerDeltaYaw);

				// 마우스 회전에 비례하는 회전 토크 계산 누적
				CombinedDeltaYaw += PlayerDeltaYaw * PlayerPushPower * 10.0f;
			}
		}
	}

	// 가구의 무게와 마찰력세팅 (0 이하 예외 처리)
	float FurnitureMass = FurnitureStat->GetMass();
	if (FurnitureMass <= 0.0f) 
		FurnitureMass = 200.0f;
	float Friction = FurnitureStat->GetFriction();

	// F = ma -> a = F / m
	FVector Acceleration = CombinedDeltaLoc / FurnitureMass;
	float YawAcceleration = CombinedDeltaYaw / FurnitureMass;

	// 관성 및 속도 증가
	CurrentVelocity += Acceleration * DeltaTime;
	CurrentYawVelocity += YawAcceleration * DeltaTime;

	// 마찰력 감속
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, FVector::ZeroVector, DeltaTime, Friction);
	CurrentYawVelocity = FMath::FInterpTo(CurrentYawVelocity, 0.0f, DeltaTime, Friction);

	// 가구 실제 이동
	FVector ActualDeltaLoc = CurrentVelocity * DeltaTime;
	float ActualDeltaYaw = CurrentYawVelocity * DeltaTime;
	FRotator TargetRotation = OldFurnitureRot;
	TargetRotation.Yaw += ActualDeltaYaw;

	// 가구를 새 위치와 각도로 이동
	GetOwner()->SetActorLocationAndRotation(OldFurnitureLoc + ActualDeltaLoc, TargetRotation, true);

	// 가구의 실제 회전량 (장애물 충돌 감안)
	float RealDeltaYaw = FMath::FindDeltaAngleDegrees(OldFurnitureRot.Yaw, GetOwner()->GetActorRotation().Yaw);

	// 플레이어 위치 및 회전 동기화
	for (ACharacter* Player : GrabbedPlayers)
	{
		UCharacterMovementComponent* CMC = Player->GetCharacterMovement();
		if (CMC && InitialVectors.Contains(Player) && InitialYaws.Contains(Player))
		{
			// 위치 보정
			float TotalYawChange = FMath::FindDeltaAngleDegrees(InitialYaws[Player], TargetRotation.Yaw);
			FVector CurrentVectorToFurniture = InitialVectors[Player].RotateAngleAxis(TotalYawChange, FVector::UpVector);

			// 이상적인 위치 (가구와 처음 거리 유지)
			FVector IdealLoc = GetOwner()->GetActorLocation() - CurrentVectorToFurniture;

			// 위치가 미세하게 어긋났을 때 당겨주는 고무줄 스프링 효과
			FVector CorrectionVelocity = (IdealLoc - Player->GetActorLocation()) * 10.0f;

			CMC->Velocity = CurrentVelocity + CorrectionVelocity;

			// 플레이어 몸체 및 마우스 카메라 컨트롤러 회전각도 동기화 보정
			if (PlayerDeltaYaws.Contains(Player))
			{
				float YawCorrection = RealDeltaYaw - PlayerDeltaYaws[Player];
				if (FMath::Abs(YawCorrection) > 0.01f)
				{
					// 캐릭터 메시 회전 보정
					Player->AddActorWorldRotation(FRotator(0.0f, YawCorrection, 0.0f));

					// 카메라 마우스 컨트롤러 회전 보정 (화면 동기화)
					if (AController* PC = Player->GetController())
					{
						FRotator ControlRot = PC->GetControlRotation();
						ControlRot.Yaw += YawCorrection;
						PC->SetControlRotation(ControlRot);
					}
				}
			}
		}
	}

	// 위치 및 컨트롤러 회전 갱신 기록
	for (ACharacter* Player : GrabbedPlayers)
	{
		if (Player)
		{
			PreviousPlayerLocations.Add(Player, Player->GetActorLocation());
			PreviousControlYaws.Add(Player, Player->GetControlRotation().Yaw);
		}
	}

	// 서버에서 가구 최종 위치를 클라이언트에 전송할 변수에 복사
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
	// 아무도 안잡게 되었을 경우
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
	// 무브먼트 컴포넌트가 서버에서 적용한 회전을 개무시해서 수동으로 설정하기위함.
	// 해당 캐릭터의 주인에게만 적용됨.
	if (PlayerToTarget && PlayerToTarget->IsLocallyControlled() && !GetOwner()->HasAuthority())
	{
		FRotator TargetRot = PlayerToTarget->GetActorRotation();
		TargetRot.Yaw = YawToSet;
		// 언리얼 무브먼트 컴포넌트의 고집을 뚫고 억지로 위치와 회전을 세팅
		PlayerToTarget->SetActorLocationAndRotation(LocToSet, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

