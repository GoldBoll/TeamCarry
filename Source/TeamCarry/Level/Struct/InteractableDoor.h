// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/InteractableInterface.h"
#include "InteractableDoor.generated.h"

class UStaticMeshComponent;

UCLASS()
class TEAMCARRY_API AInteractableDoor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
	
public:
	AInteractableDoor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool  CanInteract_Implementation(AActor* Interactor) const override;
	virtual void  Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractText_Implementation() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<USceneComponent> DoorRoot;

	// 경첩을 모서리에 맞춘 메시 — 직접 회전
	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, Category = "Door")
	FRotator ClosedRotation = FRotator(0.f, 0.f, 0.f);

	// 미는 문 = -90 (당기는 문이면 90)
	UPROPERTY(EditAnywhere, Category = "Door")
	FRotator OpenRotation = FRotator(0.f, -90.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenSpeed = 120.f;

	UPROPERTY(ReplicatedUsing = OnRep_IsOpen)
	bool bIsOpen = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsInteracting)
	bool bIsInteracting = false;

private:
	bool bAnimating = false;

	UFUNCTION() void OnRep_IsOpen();
	UFUNCTION() void OnRep_IsInteracting();

	void HandleInteract(AActor* Interactor);
	void StartAnimation();
};