#pragma once

#include <openvr.h>
#include <glm/glm.hpp>

enum class EDigitalActionStateType
{
	Rising, Falling, All
};

struct Controller
{
	vr::VRInputValueHandle_t source = vr::k_ulInvalidInputValueHandle;
	vr::VRActionHandle_t poseAction = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t hapticAction = vr::k_ulInvalidActionHandle;

	glm::mat4 modelMat;
	vr::RenderModel_t* model = nullptr;
	vr::RenderModel_TextureMap_t* texture = nullptr;
};

class OpenVRWrapper
{
public:
	void init();
	void update();
	void destroy();

	glm::mat4 getViewProjMat(uint32_t hand);
	void submit(uint32_t leftEyeTexture, uint32_t rightEyeTexture);

private:
	std::string getTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr);
	glm::mat4 getEyeProjMat(vr::Hmd_Eye nEye, float fNear = 0.1f, float fFar = 100.0f);
	glm::mat4 getEyeViewMat(vr::Hmd_Eye nEye);
	glm::mat4 convertOpenVRMatrixToQMatrix(const vr::HmdMatrix34_t &mat);

	void updateInput();
	void updateTrackedDevicePose();
	void handleVREvent(const vr::VREvent_t& event);
	bool getDigitalActionState(vr::VRActionHandle_t action, EDigitalActionStateType digitalActionStateType, vr::VRInputValueHandle_t *pDevicePath = nullptr);

	void loadRenderModel(const char* name, vr::RenderModel_t*& model, vr::RenderModel_TextureMap_t*& texture);

	vr::IVRSystem* system = nullptr;
	std::string driverName;
	std::string displayName;

	vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	glm::mat4 trackedDeviceModelMat[vr::k_unMaxTrackedDeviceCount];
	char deviceClassChar[vr::k_unMaxTrackedDeviceCount];

	vr::VRActionSetHandle_t actionSet = vr::k_ulInvalidActionSetHandle;
	vr::VRActionHandle_t triggerAction = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t gripAction = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t trackpadAction = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t menuAction = vr::k_ulInvalidActionHandle;

	bool bTrigger;
	glm::vec2 trackpad;
	Controller controller[2];

	uint32_t rtWidth;
	uint32_t rtHeight;

	glm::mat4 eyeViewProjMat[2];
	glm::mat4 hmdModelMat;
};