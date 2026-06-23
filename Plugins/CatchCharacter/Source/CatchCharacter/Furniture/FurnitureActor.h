// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CatchCharacter/Public/FurnitureDataTable.h"
#include "FurnitureActor.generated.h"

class UStaticMeshComponent;
class UFurnitureStat;
class UFurnitureGrabSystem;
class UFurnitureDamage;

UCLASS()
class CATCHCHARACTER_API AFurnitureActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AFurnitureActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FurnitureMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFurnitureStat* FurnitureStat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFurnitureGrabSystem* GrabSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFurnitureDamage* DamageSystem;

	// --- 설정 데이터 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furniture|Setup")
	FDataTableRowHandle FurnitureDataRow;

public:	
	virtual void Tick(float DeltaTime) override;

	UFurnitureGrabSystem* GetGrabSystem() const { return GrabSystem; }
	UFurnitureStat* GetFurnitureStat() const { return FurnitureStat; }
};
