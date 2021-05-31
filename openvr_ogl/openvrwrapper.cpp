#include "openvrwrapper.h"
#include <thread>

//#pragma warning(disable:4312)

bool OpenVRRenderModel::init(const vr::RenderModel_t& vrRenderModel, const vr::RenderModel_TextureMap_t& vrDiffuseTexture)
{
	// create and bind a VAO to hold state for this model
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Populate a vertex buffer
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * vrRenderModel.unVertexCount, vrRenderModel.rVertexData, GL_STATIC_DRAW);

	// Identify the components in the vertex buffer
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

	// Create and populate the index buffer
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * vrRenderModel.unTriangleCount * 3, vrRenderModel.rIndexData, GL_STATIC_DRAW);

	glBindVertexArray(0);

	// create and populate the texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData);

	// If this renders black ask McJohn what's wrong.
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	vertexCount = vrRenderModel.unTriangleCount * 3;

	return true;
}

void OpenVRRenderModel::render()
{
	glBindVertexArray(vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
}

void OpenVRRenderModel::destroy()
{
	if (ibo != 0)
	{
		glDeleteBuffers(1, &ibo);
	}
	
	if (vao != 0)
	{
		glDeleteVertexArrays(1, &vao);
	}
	
	if (vbo != 0)
	{
		glDeleteBuffers(1, &vbo);
	}
	
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
	}
}

void OpenVRWrapper::init()
{
	memset(deviceClassChar, 0, sizeof(deviceClassChar));

	if (!vr::VR_IsHmdPresent())
	{
		throw std::runtime_error("Failed to detect HMD!");
	}

	if (!vr::VR_IsRuntimeInstalled())
	{
		throw std::runtime_error("Failed to detect OpenVR runtime!");
	}

	vr::EVRInitError err = vr::VRInitError_None;
	system = vr::VR_Init(&err, vr::VRApplication_Scene);

	if (err != vr::VRInitError_None)
	{
		throw std::runtime_error(vr::VR_GetVRInitErrorAsEnglishDescription(err));
	}

	driverName = getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	displayName = getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

	if (!vr::VRCompositor())
	{
		throw std::runtime_error("Failed to initialize compositor!");
	}

	system->GetRecommendedRenderTargetSize(&rtWidth, &rtHeight);
	printf("Initialized HMD with driver: %s, display: %s, suggested render target size: %d*%d\n", 
		driverName.c_str(), displayName.c_str(), rtWidth, rtHeight);

	for (int i = 0; i < 2; ++i)
	{
		FramebufferDesc& framebufferDesc = eyeFramebufferDesc[i];
		glGenFramebuffers(1, &framebufferDesc.renderFramebufferID);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.renderFramebufferID);

		glGenRenderbuffers(1, &framebufferDesc.depthBufferID);
		glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.depthBufferID);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, rtWidth, rtHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.depthBufferID);

		glGenTextures(1, &framebufferDesc.renderTextureID);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.renderTextureID);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, rtWidth, rtHeight, true);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.renderTextureID, 0);

		glGenFramebuffers(1, &framebufferDesc.resolveFramebufferID);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.resolveFramebufferID);

		glGenTextures(1, &framebufferDesc.resolveTextureID);
		glBindTexture(GL_TEXTURE_2D, framebufferDesc.resolveTextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rtWidth, rtHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.resolveTextureID, 0);

		// check FBO status
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("Failed to create eye frame buffer description!");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	eyeViewProjMat[0] = getEyeProjMat(vr::Eye_Left) * getEyeViewMat(vr::Eye_Left);
	eyeViewProjMat[1] = getEyeProjMat(vr::Eye_Right) * getEyeViewMat(vr::Eye_Right);

	vr::EVRInputError inputError = vr::VRInputError_None;
	inputError = vr::VRInput()->SetActionManifestPath("E:/VRViewer/openvr_ogl/openvr_ogl/asset/config/actions.json");
	if (inputError != vr::VRInputError_None)
	{
		printf("Failed to SetActionManifestPath, error: %d", inputError);
	}

	inputError = vr::VRInput()->GetActionSetHandle("/actions/main", &actionSet);
	inputError = vr::VRInput()->GetActionHandle("/actions/main/in/trigger", &triggerAction);
	inputError = vr::VRInput()->GetActionHandle("/actions/main/in/grip", &gripAction);
	inputError = vr::VRInput()->GetActionHandle("/actions/main/in/trackpad", &trackpadAction);
	inputError = vr::VRInput()->GetActionHandle("/actions/main/in/application_menu", &menuAction);

	inputError = vr::VRInput()->GetActionHandle("/actions/main/out/haptic_left", &controller[0].hapticAction);
	inputError = vr::VRInput()->GetInputSourceHandle("/user/hand/left", &controller[0].source);
	inputError = vr::VRInput()->GetActionHandle("/actions/main/in/hand_left", &controller[0].poseAction);

	inputError = vr::VRInput()->GetActionHandle("/actions/main/out/haptic_right", &controller[1].hapticAction);
	inputError = vr::VRInput()->GetInputSourceHandle("/user/hand/right", &controller[1].source);
	inputError = vr::VRInput()->GetActionHandle("/actions/main/in/hand_right", &controller[1].poseAction);
}

void OpenVRWrapper::update()
{
	updateInput();
	updateTrackedDevicePose();
}

void OpenVRWrapper::render()
{
	if (!renderSceneFunc)
	{
		return;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, eyeFramebufferDesc[0].renderFramebufferID);
	glViewport(0, 0, rtWidth, rtHeight);

	renderSceneFunc(getViewProjMat(0));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, eyeFramebufferDesc[0].renderFramebufferID);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, eyeFramebufferDesc[0].resolveFramebufferID);

	glBlitFramebuffer(0, 0, rtWidth, rtHeight, 0, 0, rtWidth, rtHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glEnable(GL_MULTISAMPLE);

	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, eyeFramebufferDesc[1].renderFramebufferID);
	glViewport(0, 0, rtWidth, rtHeight);

	renderSceneFunc(getViewProjMat(1));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, eyeFramebufferDesc[1].renderFramebufferID);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, eyeFramebufferDesc[1].resolveFramebufferID);

	glBlitFramebuffer(0, 0, rtWidth, rtHeight, 0, 0, rtWidth, rtHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	submit(eyeFramebufferDesc[0].resolveTextureID, eyeFramebufferDesc[1].resolveTextureID);
}

void OpenVRWrapper::destroy()
{
	for (int i = 0; i < 2; ++i)
	{
		if (controller[i].renderModel)
		{
			controller[i].renderModel->destroy();
		}
	}

	if (system)
	{
		vr::VR_Shutdown();
		system = nullptr;
	}
}

bool OpenVRWrapper::isControllerActive(uint32_t hand)
{
	return controller[hand].renderModel != nullptr;
}

const glm::mat4& OpenVRWrapper::getControllerModelMat(uint32_t hand)
{
	return controller[hand].modelMat;
}

void OpenVRWrapper::renderController(uint32_t hand)
{
	if (controller[hand].renderModel)
	{
		controller[hand].renderModel->render();
	}
}

glm::mat4 OpenVRWrapper::getViewProjMat(uint32_t hand)
{
	return eyeViewProjMat[hand] * hmdModelMat;
}

void OpenVRWrapper::setRenderSceneFunc(const std::function<void(const glm::mat4&)>& renderSceneFunc)
{
	this->renderSceneFunc = renderSceneFunc;
}

void OpenVRWrapper::submit(GLuint leftEyeTextureID, GLuint rightEyeTextureID)
{
	vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeTextureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeTextureID, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };

	vr::EVRCompositorError CompositorError;
	CompositorError = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	if (CompositorError != vr::VRCompositorError_None)
	{
		printf("Failed to submit left eye texture! Error: %d\n", CompositorError);
	}

	CompositorError = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	if (CompositorError != vr::VRCompositorError_None)
	{
		printf("Failed to submit right eye texture! Error: %d\n", CompositorError);
	}	
}

std::string OpenVRWrapper::getTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError /*= nullptr*/)
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
	{
		return "";
	}

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;

	return sResult;
}

glm::mat4 OpenVRWrapper::getEyeProjMat(vr::Hmd_Eye nEye, float fNear, float fFar)
{
	vr::HmdMatrix44_t mat = system->GetProjectionMatrix(nEye, fNear, fFar);

	return glm::mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

glm::mat4 OpenVRWrapper::getEyeViewMat(vr::Hmd_Eye nEye)
{
	vr::HmdMatrix34_t mat = system->GetEyeToHeadTransform(nEye);
	glm::mat4 invertedMat(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	);

	return glm::inverse(invertedMat);
}

glm::mat4 OpenVRWrapper::convertOpenVRMatrixToQMatrix(const vr::HmdMatrix34_t &mat)
{
	return glm::mat4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	);
}

void OpenVRWrapper::updateInput()
{
	vr::VREvent_t event;
	while (system->PollNextEvent(&event, sizeof(event)))
	{
		handleVREvent(event);
	}

	vr::VRActiveActionSet_t activeActionSet = { actionSet };
	vr::VRInput()->UpdateActionState(&activeActionSet, sizeof(activeActionSet), 1);

	bTrigger = getDigitalActionState(triggerAction, EDigitalActionStateType::All);

	vr::VRInputValueHandle_t ulHapticDevice;
	if (getDigitalActionState(gripAction, EDigitalActionStateType::Rising, &ulHapticDevice))
	{
		if (ulHapticDevice == controller[0].source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(controller[0].hapticAction, 0.0f, 1.0f, 4.0f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
		if (ulHapticDevice == controller[1].source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(controller[1].hapticAction, 0.0f, 1.0f, 4.0f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
	}

	vr::InputAnalogActionData_t analogData;
	if (vr::VRInput()->GetAnalogActionData(trackpadAction, &analogData, sizeof(analogData), 
		vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && analogData.bActive)
	{
		trackpad[0] = analogData.x;
		trackpad[1] = analogData.y;
	}

	for (int i = 0; i < 2; ++i)
	{
		Controller& hand = controller[i];
		vr::InputPoseActionData_t poseData;
		if (vr::VRInput()->GetPoseActionDataForNextFrame(hand.poseAction, vr::TrackingUniverseStanding, &poseData, 
			sizeof(poseData), vr::k_ulInvalidInputValueHandle) == vr::VRInputError_None && poseData.bActive && poseData.pose.bPoseIsValid)
		{
			hand.modelMat = convertOpenVRMatrixToQMatrix(poseData.pose.mDeviceToAbsoluteTracking);

			vr::InputOriginInfo_t originInfo;
			if (vr::VRInput()->GetOriginTrackedDeviceInfo(poseData.activeOrigin, &originInfo, sizeof(originInfo)) == vr::VRInputError_None
				&& originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid)
			{
				std::string renderModelName = getTrackedDeviceString(originInfo.trackedDeviceIndex, vr::Prop_RenderModelName_String);
				if (!hand.renderModel)
				{
					hand.renderModel = loadRenderModel(renderModelName);
				}
			}
		}
	}
}

void OpenVRWrapper::updateTrackedDevicePose()
{
	vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

	int validPoseCount = 0;
	std::string poseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (trackedDevicePose[nDevice].bPoseIsValid)
		{
			validPoseCount++;
			trackedDeviceModelMat[nDevice] = convertOpenVRMatrixToQMatrix(trackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (deviceClassChar[nDevice] == 0)
			{
				switch (system->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        deviceClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               deviceClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           deviceClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    deviceClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: deviceClassChar[nDevice] = 'T'; break;
				default:                                       deviceClassChar[nDevice] = '?'; break;
				}
			}
			poseClasses += deviceClassChar[nDevice];
		}
	}

	if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		hmdModelMat = glm::inverse(trackedDeviceModelMat[vr::k_unTrackedDeviceIndex_Hmd]);
	}
}

void OpenVRWrapper::handleVREvent(const vr::VREvent_t& event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		printf("Device %d detached\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		printf("Device %d updated\n", event.trackedDeviceIndex);
	}
	break;
	}
}

bool OpenVRWrapper::getDigitalActionState(vr::VRActionHandle_t action, EDigitalActionStateType digitalActionStateType, vr::VRInputValueHandle_t *pDevicePath /*= nullptr*/)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}

	switch (digitalActionStateType)
	{
	case EDigitalActionStateType::Rising:
		return actionData.bActive && actionData.bChanged && actionData.bState;
	case EDigitalActionStateType::Falling:
		return actionData.bActive && actionData.bChanged && !actionData.bState;
	case EDigitalActionStateType::All:
		return actionData.bActive && actionData.bState;
	default:
		return false;
	}
}

OpenVRRenderModel* OpenVRWrapper::loadRenderModel(const std::string& renderModelName)
{
	if (renderModels.find(renderModelName) != renderModels.end())
	{
		return renderModels[renderModelName];
	}

	vr::RenderModel_t *pModel;
	vr::EVRRenderModelError error;
	int aysncLoadSleepTime = 1;
	while (true)
	{
		error = vr::VRRenderModels()->LoadRenderModel_Async(renderModelName.c_str(), &pModel);
		if (error != vr::VRRenderModelError_Loading)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(aysncLoadSleepTime));
	}

	if (error != vr::VRRenderModelError_None)
	{
		printf("Failed to load render model %s, error: %s\n", renderModelName.c_str(), 
			vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));

		return nullptr;
	}

	vr::RenderModel_TextureMap_t *pTexture;
	while (1)
	{
		error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
		if (error != vr::VRRenderModelError_Loading)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::seconds(aysncLoadSleepTime));
	}

	if (error != vr::VRRenderModelError_None)
	{
		printf("Failed to load render model %s's texture id %d\n", renderModelName.c_str(), pModel->diffuseTextureId);
		vr::VRRenderModels()->FreeRenderModel(pModel);

		return nullptr;
	}

	OpenVRRenderModel* pRenderModel = new OpenVRRenderModel;
	pRenderModel->init(*pModel, *pTexture);

	vr::VRRenderModels()->FreeRenderModel(pModel);
	vr::VRRenderModels()->FreeTexture(pTexture);

	renderModels[renderModelName] = pRenderModel;
	return pRenderModel;
}
