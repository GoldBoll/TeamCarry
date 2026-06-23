// Fill out your copyright notice in the Description page of Project Settings.


#include "Level/Struct/InteractableDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AInteractableDoor::AInteractableDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
	SetRootComponent(DoorRoot);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorRoot);
}

void AInteractableDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AInteractableDoor, bIsOpen);
	DOREPLIFETIME(AInteractableDoor, bIsInteracting);
}

void AInteractableDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DoorMesh->SetRelativeRotation(ClosedRotation); // 에디터 미리보기
}

void AInteractableDoor::BeginPlay()
{
	Super::BeginPlay();
	DoorMesh->SetRelativeRotation(bIsOpen ? OpenRotation : ClosedRotation);
	bAnimating = false;
}

bool AInteractableDoor::CanInteract_Implementation(AActor*) const
{
	return !bIsInteracting; // 애니메이션 중이면 전 클라 잠금
}

void AInteractableDoor::Interact_Implementation(AActor* Interactor)
{
	if (HasAuthority()) { HandleInteract(Interactor); }
}

FText AInteractableDoor::GetInteractText_Implementation() const
{
	return bIsOpen ? NSLOCTEXT("Door", "Close", "닫기")
		: NSLOCTEXT("Door", "Open", "열기");
}

void AInteractableDoor::HandleInteract(AActor*)
{
	if (!HasAuthority() || bIsInteracting) return;

	bIsInteracting = true; 
	bIsOpen = !bIsOpen;    
	StartAnimation();
	ForceNetUpdate();
}

void AInteractableDoor::StartAnimation() { bAnimating = true; }

void AInteractableDoor::OnRep_IsOpen() { StartAnimation(); }
void AInteractableDoor::OnRep_IsInteracting() { /* UI 프롬프트 갱신 훅 */ }

void AInteractableDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bAnimating) return;

	const FRotator Target = bIsOpen ? OpenRotation : ClosedRotation;
	const FRotator Current = DoorMesh->GetRelativeRotation();
	const FRotator NewRot = FMath::RInterpConstantTo(Current, Target, DeltaSeconds, OpenSpeed);
	DoorMesh->SetRelativeRotation(NewRot);

	if (NewRot.Equals(Target, 0.5f))
	{
		DoorMesh->SetRelativeRotation(Target);
		bAnimating = false;

		if (HasAuthority()) 
		{
			bIsInteracting = false;
			ForceNetUpdate();
		}
	}
}
