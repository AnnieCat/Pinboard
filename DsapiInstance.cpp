#include "DsapiPluginPrivatePCH.h"
#include "DsapiInstance.h"

#include "DSAPI.h"
#include "DSAPIUtil.h"
#include <string>

#include "motion.h"

#define DS_CHECK_ERRORS(s) if (!s)                                                                                                                                                                            \
{                                                                                                                                                                                                             \
    if(DebugPrintErrors)                                                                                                                                                                                      \
    {                                                                                                                                                                                                         \
        GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Red, FString::Printf(_T("Details: %s"), ANSI_TO_TCHAR(ds->getLastErrorDescription())));                                                              \
        GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Red, FString::Printf(_T("DSAPI call failed at DsapiInstance.cpp:%d with %s."), __LINE__, ANSI_TO_TCHAR(DSStatusString(ds->getLastErrorStatus()))));  \
    }                                                                                                                                                                                                         \
    goto error;                                                                                                                                                                                               \
}                          


ADsapiInstance::ADsapiInstance(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP), ds()
{
	PrimaryActorTick.bCanEverTick = true;

    ColorMode = EColorMode::VGA;
    DepthMode = EDepthMode::BIG;

	MyDepthValues = TArray<int32>();
	MyDepthValues.AddUninitialized(60 * 46);
}

void ADsapiInstance::BeginPlay() 
{
    ds = DSCreate(DS_DS4_PLATFORM);
    DS_CHECK_ERRORS(ds->probeConfiguration());

    auto third = ds->accessThird();
    if(!third && ColorMode != EColorMode::OFF)
    {
        GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Red, _T("DSThird interface not available, Color Mode must be set to \"Off\"."));
        goto error;
    }

    if(DebugPrintInfo)
    {
        uint32_t serialNo;
        DS_CHECK_ERRORS(ds->getCameraSerialNumber(serialNo));
        GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Blue, FString::Printf(_T("Camera serial no: %d"), serialNo));
        GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Blue, FString::Printf(_T("Software version: %s"), ANSI_TO_TCHAR(ds->getSoftwareVersionString())));
        GEngine->AddOnScreenDebugMessage(-1, 20, FColor::Blue, FString::Printf(_T("Firmware version: %s"), ANSI_TO_TCHAR(ds->getFirmwareVersionString())));
    }
   
    // Configure core Z-from-stereo capabilities
    DS_CHECK_ERRORS(ds->enableZ(false));
    DS_CHECK_ERRORS(ds->enableLeft(false));
    DS_CHECK_ERRORS(ds->enableRight(false));
    if(DepthMode != EDepthMode::OFF)
    {
        const int depthRes[][2] = {{0,0},{320,240},{480,360},{628,468}};
        DS_CHECK_ERRORS(ds->setLRZResolutionMode(true, depthRes[DepthMode][0], depthRes[DepthMode][1], 60, DS_LUMINANCE8));
        DS_CHECK_ERRORS(ds->enableZ(true));
    }

    // Configure third camera
    if(ColorMode != EColorMode::OFF)
    {       
        const int colorRes[][2] = {{0,0},{640,480},{1920,1080}};
        DS_CHECK_ERRORS(third->setThirdResolutionMode(true, colorRes[ColorMode][0], colorRes[ColorMode][1], 30, DS_BGRA8));
        DS_CHECK_ERRORS(third->enableThird(true));
    }
    else AlignDepthToColor = false;

    // Change exposure and gain values
    if(auto hardware = ds->accessHardware())
    {
        DS_CHECK_ERRORS(hardware->setImagerExposure(16.3f, DS_BOTH_IMAGERS));
        DS_CHECK_ERRORS(hardware->setImagerGain(2.0f, DS_BOTH_IMAGERS));
    }

    // Begin capturing images
    DS_CHECK_ERRORS(ds->startCapture());   

    // Allocate textures
    if(ds->isZEnabled())
    {
        if(AlignDepthToColor) DepthVideoStream = UTexture2D::CreateTransient(third->thirdWidth(), third->thirdHeight(), PF_G16);
        else DepthVideoStream = UTexture2D::CreateTransient(ds->zWidth(), ds->zHeight(), PF_G16);
    }
    if(third->isThirdEnabled())
    {
        ColorVideoStream = UTexture2D::CreateTransient(third->thirdWidth(), third->thirdHeight(), PF_B8G8R8A8);
    }

	sensor = CreateSensor();

    return;

error:
    CloseDS();
}

void ADsapiInstance::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
    CloseDS();
}

void AlignZToThird(DSAPI & dsapi, DSThird & third, uint16_t *inDepth, bool fillHoles, uint16_t *alignedZ);

class Image
{
private:
	uint16_t * pixels;
	int width, height;
public:
	void ReferenceDepthMap(DSAPI * ds)
	{
		pixels = ds->getZImage();
		width = ds->zWidth();
		height = ds->zHeight();
	}

	void Allocate(int w, int h)
	{
		pixels = new uint16_t[w*h];
		width = w;
		height = h;
	}

	void SetPixel(int x, int y, uint16_t value)
	{
		pixels[y * width + x] = value;
	}

	uint16_t GetPixel(int x, int y)
	{
		return pixels[y * width + x];
	}

	int GetWidth() { return width; }
	int GetHeight() { return height; }
};

int MapLinearRange(int value, int fromLow, int fromHigh, int toLow, int toHigh)
{
	return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

void ADsapiInstance::ReceiveTick(float DeltaSeconds)
{
    if(ds)
    {
        DS_CHECK_ERRORS(ds->grab());
  
        auto third = ds->accessThird();

		// BEGIN ANNIE PLAYGROUND

		Image depthBuffer;
		depthBuffer.ReferenceDepthMap(ds); // Point to DSAPI's depth image, and make sure we have the right size

		Image secondaryImage;
		secondaryImage.Allocate(60, 46); // Allocate space for a 30 wide, 23 tall image

		// This implements a poor man's scale down
		for (int y = 0; y < secondaryImage.GetHeight(); ++y)
		{
			for (int x = 0; x < secondaryImage.GetWidth(); ++x)
			{
				int otherY = MapLinearRange(y,   0, secondaryImage.GetHeight() - 1,   0, depthBuffer.GetHeight() - 1);
				int otherX = MapLinearRange(x,   0, secondaryImage.GetWidth() - 1,    58, depthBuffer.GetWidth() - 1);
				
				uint16_t sample1 = depthBuffer.GetPixel(otherX, otherY);
				uint16_t sample2 = depthBuffer.GetPixel(otherX + 1, otherY);
				uint16_t sample3 = depthBuffer.GetPixel(otherX, otherY + 1);
				uint16_t sample4 = depthBuffer.GetPixel(otherX + 1, otherY + 1);
				uint16_t sampleAverage = (sample1 + sample2 + sample3 + sample4) / 4;
				
				//if (sampleAverage<1500)
				//	secondaryImage.SetPixel(x, y, sampleAverage);
				secondaryImage.SetPixel(x, y, sample1);
				//MyDepthValues[y * secondaryImage.GetWidth() + x] = (sample1 + sample2 + sample3 + sample4) / 4;
				MyDepthValues[y * secondaryImage.GetWidth() + x] = sample1;
			}
		}

		// END ANNIE PLAYGROUND

        if(DepthVideoStream)
        {
            auto & mip = DepthVideoStream->PlatformData->Mips[0];
            auto depthTex = reinterpret_cast<uint16_t *>(mip.BulkData.Lock(LOCK_READ_WRITE));
            if(AlignDepthToColor)
            {
                AlignZToThird(*ds, *third, ds->getZImage(), true, depthTex);
            }
            else
            {
	            FMemory::Memcpy(depthTex, ds->getZImage(), mip.SizeX*mip.SizeY*sizeof(uint16_t));	
            }
	        mip.BulkData.Unlock();
	        DepthVideoStream->UpdateResource();
        }

        if(ColorVideoStream)
        {
            auto & mip = ColorVideoStream->PlatformData->Mips[0];
	        FMemory::Memcpy(mip.BulkData.Lock(LOCK_READ_WRITE), third->getThirdImage(), mip.SizeX*mip.SizeY*sizeof(uint32_t));	
	        mip.BulkData.Unlock();
	        ColorVideoStream->UpdateResource();
        }
    }

	if (sensor && IsStarted(sensor))
	{
		GetCurrentFrame(sensor);
		IMUYaw = GetYaw(sensor);
		IMUPitch = GetPitch(sensor);
		IMURoll = GetRoll(sensor);

		float quat[4];
		GetQuat(sensor, quat);
		GetCameraRotation = FRotator(FQuat(quat[0], quat[1], quat[2], quat[3]));
	}

    goto finally;

error:
    CloseDS();

finally:
    AActor::ReceiveTick(DeltaSeconds);
}

FRotator ADsapiInstance::FixRotatorBases(FRotator r)
{
	FQuat q1 = r.Quaternion();
	FQuat q2 = FQuat(-q1.Z, q1.X, -q1.Y, q1.W);
	return FRotator(q2);
}

void ADsapiInstance::CloseDS()
{
	if (sensor)
	{
		DeleteSensor(sensor);
	}
	sensor = nullptr;

    if(ds)
    {
        DSDestroy(ds);
    }

    ds = nullptr;
    ColorVideoStream = nullptr;
    DepthVideoStream = nullptr;
}

void AlignZToThird(DSAPI & dsapi, DSThird & third, uint16_t *inDepth, bool fillHoles, uint16_t *alignedZ)
{
    double translation[3];
    third.getCalibExtrinsicsZToRectThird(translation);
    float offsetX = static_cast<float>(translation[0]), offsetY = static_cast<float>(translation[1]), offsetZ = static_cast<float>(translation[2]);

    DSCalibIntrinsicsRectified zIntrinsics, thirdIntrinsics;
    dsapi.getCalibIntrinsicsZ(zIntrinsics);
    third.getCalibIntrinsicsRectThird(thirdIntrinsics);
    float invZFocalX = 1.0f / zIntrinsics.rfx, invZFocalY = 1.0f / zIntrinsics.rfy;

	memset(alignedZ, 0, thirdIntrinsics.rw * thirdIntrinsics.rh * sizeof(uint16_t));

    for (unsigned int y = 0; y < zIntrinsics.rh; ++y)
    {
		const float tempy = (y - zIntrinsics.rpy) * invZFocalY;
        for (unsigned int x = 0; x < zIntrinsics.rw; ++x)
        {
            auto depth = *inDepth++;

            // DSTransformFromZImageToZCamera(zIntrinsics, zImage, zCamera); // Move from image coordinates to 3D coordinates
			float zCamZ = static_cast<float>(depth); 
			float zCamX = zCamZ * (x - zIntrinsics.rpx) * invZFocalX;
			float zCamY = zCamZ * tempy;

            // DSTransformFromZCameraToRectThirdCamera(translation, zCamera, thirdCamera); // Move from Z to Third
            float thirdCamX = zCamX + offsetX;
            float thirdCamY = zCamY + offsetY;
            float thirdCamZ = zCamZ + offsetZ;

            // DSTransformFromThirdCameraToRectThirdImage(thirdIntrinsics, thirdCamera, thirdImage); // Move from 3D coordinates back to image coordinates
            int thirdImageX = static_cast<int>(thirdIntrinsics.rfx * (thirdCamX / thirdCamZ) + thirdIntrinsics.rpx);
            int thirdImageY = static_cast<int>(thirdIntrinsics.rfy * (thirdCamY / thirdCamZ) + thirdIntrinsics.rpy);

            // Clip anything that falls outside the boundaries of the third image
            if (thirdImageX < 0 || thirdImageY < 0 || thirdImageX >= static_cast<int>(thirdIntrinsics.rw) || thirdImageY >= static_cast<int>(thirdIntrinsics.rh))
            {
                continue;
            }

            // Write the current pixel to the aligned image
            auto & outDepth = alignedZ[thirdImageY * thirdIntrinsics.rw + thirdImageX];
            if(!outDepth || depth < outDepth) outDepth = depth;
        }
    }

    // OPTIONAL: This does a very simple hole-filling by propagating each pixel into holes to its immediate left and above
    if (fillHoles)
    {
        auto out = alignedZ;
        for (unsigned int y = 0; y < thirdIntrinsics.rh; ++y)
        {
            for (unsigned int x = 0; x < thirdIntrinsics.rw; ++x)
            {
                if (x + 1 < thirdIntrinsics.rw)
                {
                    if (!out[0] && out[1])
                    {
                        out[0] = out[1];
                    }
                }
                if (y + 1 < thirdIntrinsics.rh)
                {
                    if (!out[0] && out[thirdIntrinsics.rw])
                    {
                        out[0] = out[thirdIntrinsics.rw];
                    }
                }
                if (x + 1 < thirdIntrinsics.rw && y + 1 < thirdIntrinsics.rh)
                {
                    if (!out[0] && out[thirdIntrinsics.rw + 1])
                    {
                        out[0] = out[thirdIntrinsics.rw + 1];
                    }
                }
                ++out;
            }
        }
    }
}