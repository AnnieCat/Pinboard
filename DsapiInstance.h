#pragma once

#include "DsapiInstance.generated.h"

class DSAPI;
struct Sensor;

UENUM(BlueprintType) namespace EColorMode
{
	enum Type
	{ 
        OFF UMETA(DisplayName="Off"),
        VGA UMETA(DisplayName="VGA (640 x 480)"),
		HD UMETA(DisplayName="HD (1920 x 1080)"),
	};
}

UENUM(BlueprintType) namespace EDepthMode
{
	enum Type
	{ 
        OFF UMETA(DisplayName="Off"),
        QRES UMETA(DisplayName="QRes (320 x 240)"),
		BIG UMETA(DisplayName="Big (480 x 360)"),
        FULL UMETA(DisplayName="Full (628 x 468)"),
	};
}

/**
 * An object which controls an Intel DS* device via DSAPI 1.6
 */
UCLASS() class ADsapiInstance : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = "DSAPI Configuration", EditAnywhere, BlueprintReadOnly) bool DebugPrintErrors;
    UPROPERTY(Category = "DSAPI Configuration", EditAnywhere, BlueprintReadOnly) bool DebugPrintInfo;
	UPROPERTY(Category = "DSAPI Configuration", EditAnywhere, BlueprintReadOnly) TEnumAsByte<EColorMode::Type> ColorMode;
    UPROPERTY(Category = "DSAPI Configuration", EditAnywhere, BlueprintReadOnly) TEnumAsByte<EDepthMode::Type> DepthMode;
    UPROPERTY(Category = "DSAPI Configuration", EditAnywhere, BlueprintReadOnly) bool AlignDepthToColor;

	UPROPERTY(Category = "DSAPI Streams", VisibleAnywhere, BlueprintReadOnly, transient) UTexture2D* ColorVideoStream;
	UPROPERTY(Category = "DSAPI Streams", VisibleAnywhere, BlueprintReadOnly, transient) UTexture2D* DepthVideoStream;

	UPROPERTY(Category = "Pixel Values", BlueprintReadOnly, transient, VisibleAnywhere) TArray<int32> MyDepthValues;

	UPROPERTY(Category = "IMU Readings", BlueprintReadOnly, transient, VisibleAnywhere) float IMUYaw;
	UPROPERTY(Category = "IMU Readings", BlueprintReadOnly, transient, VisibleAnywhere) float IMUPitch;
	UPROPERTY(Category = "IMU Readings", BlueprintReadOnly, transient, VisibleAnywhere) float IMURoll;

	UPROPERTY(Category = "IMU Readings", BlueprintReadOnly, transient, VisibleAnywhere) FRotator GetCameraRotation;

	UFUNCTION(Category = "IMU Utilities", BlueprintCallable, BlueprintPure) FRotator FixRotatorBases(FRotator r);

	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ReceiveTick(float DeltaSeconds) override;
private:
    void CloseDS();
    DSAPI * ds;
	Sensor * sensor;
};
