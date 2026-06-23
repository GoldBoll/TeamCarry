// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "FurnitureStatComponent.generated.h"

USTRUCT(BlueprintType)
struct FFurnitureStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredPlayer = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CollisionDamageMultiplier = 10.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFurnitureDestroyed);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CATCHCHARACTER_API UFurnitureStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFurnitureStatComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Furniture")
	void TakeCollisionDamage(float ImpactMagnitude);

	UFUNCTION(BlueprintCallable, Category = "Furniture")
	void AddGrabber(class UPlayerInteractComponent* PlayerComp);

	UFUNCTION(BlueprintCallable, Category = "Furniture")
	void RemoveGrabber(class UPlayerInteractComponent* PlayerComp);

	UFUNCTION(BlueprintCallable, Category = "Furniture")
	void AddMovementInput(FVector MovementInput, FVector PlayerForward);

	UFUNCTION(BlueprintPure, Category = "Furniture")
	bool IsCorrectGrabberCount() const { return CurrentGrabbedPlayer >= RequiredPlayer; }

	UFUNCTION(BlueprintPure, Category = "Furniture")
	float GetMovementSpeedScale() const;

	UPROPERTY(BlueprintAssignable, Category = "Furniture")
	FOnFurnitureDestroyed OnFurnitureDestroyed;

protected:
	// --- 설정값---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture|State")
	FFurnitureStats DefaultStats;

	// --- 런타임 후 스텟 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Furniture|State")
	float CurrentHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Furniture|State")
	int32 RequiredPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Furniture|State")
	float BaseSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Furniture|State")
	float CollisionDamageMultiplier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Furniture|State")
	int32 CurrentGrabbedPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Furniture|State")
	bool bIsGrabbed;

	UPROPERTY()
	TArray<class UPlayerInteractComponent*> GrabbingPlayers;

	UPROPERTY()
	TArray<FVector> GrabbingPlayerRelativeLocations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Furniture|State")
	FVector CombinedVelocity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Furniture|State")
	FVector CombinedForward;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Furniture|State")
	int32 ActivePullerCount;
};
