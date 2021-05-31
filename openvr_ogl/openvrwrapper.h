#pragma once

#include <openvr.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <functional>
#include <map>

enum class EDigitalActionStateType
{
	Rising, Falling, All
};

struct FramebufferDesc
{
	GLuint depthBufferID;

	GLuint renderTextureID;
	GLuint renderFramebufferID;

	GLuint resolveTextureID;
	GLuint resolveFramebufferID;
};

class OpenVRRenderModel
{
public:
	bool init(const vr::RenderModel_t& vrRenderModel, const vr::RenderModel_TextureMap_t& vrDiffuseTexture);
	void render();
	void destroy();
	
private:
	GLuint vbo = 0;
	GLuint ibo = 0;
	GLuint vao = 0;
	GLuint texture = 0;
	uint32_t vertexCount;
};

struct Controller
{
	vr::VRInputValueHandle_t source = vr::k_ulInvalidInputValueHandle;
	vr::VRActionHandle_t poseAction = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t hapticAction = vr::k_ulInvalidActionHandle;

	glm::mat4 modelMat;
	OpenVRRenderModel* renderModel = nullptr;
};

class OpenVRWrapper
{
public:
	void init();
	void update();
	void render();
	void destroy();

	bool isControllerActive(uint32_t hand);
	const glm::mat4& getControllerModelMat(uint32_t hand);
	void renderController(uint32_t hand);

	glm::mat4 getViewProjMat(uint32_t hand);
	void setRenderSceneFunc(const std::function<void(const glm::mat4&)>& renderSceneFunc);
	void submit(GLuint leftEyeTexture, GLuint rightEyeTexture);

private:
	std::string getTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr);
	glm::mat4 getEyeProjMat(vr::Hmd_Eye nEye, float fNear = 0.1f, float fFar = 100.0f);
	glm::mat4 getEyeViewMat(vr::Hmd_Eye nEye);
	glm::mat4 convertOpenVRMatrixToQMatrix(const vr::HmdMatrix34_t &mat);

	void updateInput();
	void updateTrackedDevicePose();
	void handleVREvent(const vr::VREvent_t& event);

	bool getDigitalActionState(vr::VRActionHandle_t action, EDigitalActionStateType digitalActionStateType, vr::VRInputValueHandle_t *pDevicePath = nullptr);
	OpenVRRenderModel* loadRenderModel(const std::string& renderModelName);

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
	std::map<std::string, OpenVRRenderModel*> renderModels;

	uint32_t rtWidth;
	uint32_t rtHeight;

	FramebufferDesc eyeFramebufferDesc[2];
	glm::mat4 eyeViewProjMat[2];
	glm::mat4 hmdModelMat;

	std::function<void(const glm::mat4&)> renderSceneFunc;
};