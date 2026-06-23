// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FurnitureGrabSystem.generated.h"


class UStaticMeshComponent;
class UFurnitureStat;
class ACharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CATCHCHARACTER_API UFurnitureGrabSystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFurnitureGrabSystem();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 실행될 상호작용 함수
	// GrabberComponent는 현재 더미데이터
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Grab(ACharacter* Grabber, FVector height, UPrimitiveComponent* GrabberComponent = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Release(ACharacter* Grabber);


	// 초기화 시 호출 (가구 본체에서 넘겨줌)
	void Setup(UStaticMeshComponent* InMesh, UFurnitureStat* InStat);

protected:
	virtual void BeginPlay() override;

	// --- 참조 캐싱 ---
	UPROPERTY()
	UStaticMeshComponent* FurnitureMesh;

	UPROPERTY()
	UFurnitureStat* FurnitureStat;

	// --- 상태 변수 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_GrabbedPlayers, Category = "Furniture|State")
	TArray<ACharacter*> GrabbedPlayers;

	// 클라에도 반영되도록 함수 호출됨
	// 안그러면 서버에서는 충돌되엇다고 말하는데 클라에서는 안그렇다고 생각함
	UFUNCTION()
	void OnRep_GrabbedPlayers();

	// ----보간----
	// 이동 보간작업을 위한 주기
	//float NetUpdatePeriod;

	UPROPERTY(Replicated)
	FVector ServerLocation;

	UPROPERTY(Replicated)
	FRotator ServerRotation;

private:
	void UpdateClientInterpolation(float DeltaTime);

	// 보간작업에서 출발점이 될 가장 최근 위치
	FVector PreviousClientLoc;
	FRotator PreviousClientRot;

private:

	// 플레이어의 이전 프레임 위치를 저장하여 이동량을 계산
	// 어차피 서버에서 연산될거라 클라에서 확인할필요없어보여서 리플렉션x
	TMap<ACharacter*, FVector> PreviousPlayerLocations;
	TMap<ACharacter*, float> PreviousPlayerYaws;

	// 가구를 최초로 잡은 순간의 절대적인 거리와 잣대 (파고듦 버그 방지)
	TMap<ACharacter*, FVector> InitialVectors;
	TMap<ACharacter*, float> InitialYaws;

	// 이동 로직 처리 (서버 전용)
	void HandleMovement(float DeltaTime);

	// 서버에서 계산된 위치/회전 보정값을 클라이언트에게 강제로 세팅
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ForcePlayerPositionAndRotation(ACharacter* PlayerToTarget, FVector LocToSet, float YawToSet);
};
