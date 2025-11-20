#include "RenderThread.hpp"

#include <filesystem>
#include <time.h>
#include <fstream>

using namespace Maths;

const u8 MOVEMENT_KEYS[6] =
{
	'A', 'E', 'W', 'D', 'Q', 'S'
};

std::string GetFormattedTime()
{
	time_t timeObj;
	time(&timeObj);
	tm pTime = {};
	gmtime_s(&pTime, &timeObj);
	char buffer[256];
	sprintf_s(buffer, 255, "%d-%d-%d_%d-%d-%d", pTime.tm_year+1900, pTime.tm_mon+1, pTime.tm_mday, pTime.tm_hour, pTime.tm_min, pTime.tm_sec);
	return std::string(buffer);
}

std::string LoadFile(const std::string &path)
{
	std::ifstream file = std::ifstream(path, std::ios_base::binary | std::ios_base::ate);
	if (!file.is_open())
		return "";

	u64 size = file.tellg();
	file.seekg(std::ios_base::beg);

	std::string result(size, '0');
	file.read(result.data(), size);
	file.close();

	return result;
}

void RenderThread::Init(HWND hwnd, HINSTANCE hinstance, Maths::IVec2 resIn)
{
	appData.hWnd = hwnd;
	appData.hInstance = hinstance;
	res = resIn;
	thread = std::thread(&RenderThread::ThreadFunc, this);
}

void RenderThread::Resize(s32 x, s32 y)
{
	u64 packed = (u32)(x) | ((u64)(y) << 32);
	if (packed != lastRes)
		resized = true;
	lastRes = packed;
	storedRes = packed;
}

bool RenderThread::HasFinished() const
{
	return exit;
}

bool RenderThread::HasCrashed() const
{
	return crashed;
}

void RenderThread::Quit()
{
	exit = true;
	if (thread.joinable())
		thread.join();
}

void RenderThread::MoveMouse(Vec2 delta)
{
	mouseLock.lock();
	storedDelta -= delta;
	mouseLock.unlock();
}

void RenderThread::SetKeyState(u8 key, bool state)
{
	keyLock.lock();
	keyDown.set(key, state);
	if (state)
		keyToggle.flip(key);
	keyLock.unlock();
}

void RenderThread::HandleResize()
{
	res.x = (s32)(storedRes & 0xffffffff);
	res.y = (s32)(storedRes >> 32);
}

void RenderThread::InitThread()
{
	SetThreadDescription(GetCurrentThread(), L"Render Thread");
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	start = now.time_since_epoch();
}

void RenderThread::ThreadFunc()
{
	InitThread();
	LoadAssets();

	if (!InitVulkan())
	{
		crashed = true;
		return;
	}

	f64 last = 0;
	while (!exit)
	{
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch() - start;
		auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
		f64 iTime = micros / 1000000.0;
		f32 deltaTime = static_cast<f32>(iTime - last);
		last = iTime;

		mouseLock.lock();
		Vec2 delta = storedDelta;
		storedDelta = Vec2();
		mouseLock.unlock();
		delta *= 0.005f;
		rotation.x = Util::Clamp(rotation.x - delta.y, static_cast<f32>(-M_PI_2), static_cast<f32>(M_PI_2));
		rotation.y = Util::Mod(rotation.y + delta.x, static_cast<f32>(2 * M_PI));
		Maths::Vec3 dir;
		keyLock.lock();
		for (u8 i = 0; i < 6; ++i)
		{
			f32 key = keyDown.test(MOVEMENT_KEYS[i]);
			dir[i % 3] += (i > 2) ? -key : key;
		}
		f32 fovDir = static_cast<f32>(keyDown.test(VK_UP)) - static_cast<f32>(keyDown.test(VK_DOWN));
		LaunchParams params = keyPress.test(VK_F3) ? BOXDEBUG : (keyPress.test(VK_F4) ? ADVANCED : NONE);
		bool screenshot = keyPress.test(VK_F2);
		keyPress.reset();
		keyLock.unlock();
		fov = Util::Clamp(fov + fovDir * deltaTime * fov, 0.5f, 100.0f);
		Quat q = Quat::FromEuler(Vec3(rotation.x, rotation.y, 0.0f));
		if (dir.Dot())
		{
			dir = dir.Normalize() * deltaTime * 10;
			position += q * dir;
		}
		if ((params & ADVANCED) && (delta.Dot() || dir.Dot() || fovDir))
			params = (LaunchParams)(params | CLEAR);
		
		HandleResize();

		if (!DrawFrame())
			break;
		
		if (screenshot)
		{
			if (!std::filesystem::exists("Screenshots"))
			{
				std::filesystem::create_directory("Screenshots");
			}
			std::string name = "Screenshots/";
			name += GetFormattedTime();
			//CudaUtil::SaveFrameBuffer(kernels.GetMainFrameBuffer(), name);
		}
	}

	appData.disp.deviceWaitIdle();
	UnloadAssets();
	Cleanup();

	if (!exit)
		crashed = true;
}

bool RenderThread::InitVulkan()
{
	return	InitDevice() &&
			CreateSwapchain() &&
			GetQueues() &&
			CreateRenderPass() &&
			CreateGraphicsPipeline() &&
			CreateFramebuffers() &&
			CreateCommandPool() &&
			CreateVertexBuffer(sceneData.mesh) &&
			CreateObjectBuffer(OBJECT_COUNT) &&
			CreateCommandBuffers() &&
			CreateSyncObjects();
}

void RenderThread::LoadAssets()
{
	sceneData.mesh.CreateDefaultCube();
	/*
	ModelLoader::LoadModel(meshes, materials, textures, "Assets/Scenes/monkey.obj");
	ModelLoader::LoadCubemap(cubemaps, "Assets/Cubemaps/hall.cbm");
	
	//materials[5].diffuseColor = Vec3(1,1,0);
	materials[5].shouldTeleport = 1;
	materials[5].posDisplacement = Vec3(0, 0, -15);
	materials[5].rotDisplacement = Quat();

	//materials[6].diffuseColor = Vec3(1, 1, 0);
	materials[6].shouldTeleport = 1;
	materials[6].posDisplacement = Vec3(0, 0, 15);
	materials[6].rotDisplacement = Quat();
	kernels.LoadTextures(textures);
	kernels.LoadCubemaps(cubemaps);
	kernels.LoadMaterials(materials);
	kernels.LoadMeshes(meshes);

	for (u32 i = 0; i < meshes.size(); ++i)
	{
		kernels.UpdateMeshVertices(&meshes[i], i, Vec3(0, 0, 0), Quat(), Vec3(1));
	}
	kernels.Synchronize();
	*/
}

void RenderThread::UnloadAssets()
{
	/*
	kernels.UnloadTextures(textures);
	kernels.UnloadCubemaps(cubemaps);
	kernels.UnloadMaterials();
	kernels.UnloadMeshes(meshes);
	kernels.ClearKernels();
	*/
}

void RenderThread::SendErrorPopup(const std::string &err)
{
	LogMessage(err);
#ifdef NDEBUG
	MessageBoxA(appData.hWnd, err.c_str(), "Error!", MB_YESNO);
#else
	if (MessageBoxA(appData.hWnd, (err + "\nBreak?").c_str(), "Error!", MB_YESNO) == IDYES)
		DebugBreak();
#endif
}

void RenderThread::SendErrorPopup(const std::wstring &err)
{
	LogMessage(err);
#ifdef NDEBUG
	MessageBoxW(appData.hWnd, err.c_str(), L"Error!", MB_YESNO);
#else
	if (MessageBoxW(appData.hWnd, (err + L"\nBreak?").c_str(), L"Error!", MB_YESNO) == IDYES)
		DebugBreak();
#endif
}

void RenderThread::LogMessage(const std::string &msg)
{
	OutputDebugStringA(msg.c_str());
}

void RenderThread::LogMessage(const std::wstring &msg)
{
	OutputDebugStringW(msg.c_str());
}

VkSurfaceKHR RenderThread::CreateSurfaceWin32(VkInstance instance, HINSTANCE hInstance, HWND window, VkAllocationCallbacks* allocator)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkWin32SurfaceCreateInfoKHR winSurfInfo = {};
	winSurfInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	winSurfInfo.hwnd = window;
	winSurfInfo.hinstance = hInstance;

	VkResult err = vkCreateWin32SurfaceKHR(instance, &winSurfInfo, allocator, &surface);
	if (err)
	{
		IErrorInfo* e;
		s32 ret = GetErrorInfo(0, &e);
		if (ret != 0)
		{
			std::wstring text = L"Could not create surface!\n";
			BSTR s = NULL;

			if (e && SUCCEEDED(e->GetDescription(&s)))
				text += L"Unknown error";
			else
				text += s;
			SendErrorPopup(text);
		}
		surface = VK_NULL_HANDLE;
	}
	return surface;
}

bool RenderThread::InitDevice()
{
	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.set_app_name("Vulkan Demo").set_app_version(VK_MAKE_VERSION(1, 0, 0));
	instanceBuilder.set_engine_name("Ligma Engine").request_validation_layers();

	instanceBuilder.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void*) -> VkBool32 {
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				const char* severity = vkb::to_string_message_severity(messageSeverity);
				const char* type = vkb::to_string_message_type(messageType);
				std::string res = std::string("[") + severity + ": " + type + "] " + pCallbackData->pMessage + "\n";
				OutputDebugStringA(res.c_str());
			}
			// Return false to move on, but return true for validation to skip passing down the call to the driver
			return VK_TRUE;
		});

	auto instanceRet = instanceBuilder.build();

	if (!instanceRet)
	{
		SendErrorPopup(instanceRet.error().message());
		return false;
	}
	appData.instance = instanceRet.value();
	appData.instDisp = appData.instance.make_table();
	appData.surface = CreateSurfaceWin32(appData.instance, appData.hInstance, appData.hWnd);

	vkb::PhysicalDeviceSelector physDeviceSelector(appData.instance);

	auto physDeviceRet = physDeviceSelector.set_surface(appData.surface).select();
	if (!physDeviceRet)
	{
		std::string err = physDeviceRet.error().message() + '\n';
		if (physDeviceRet.error() == vkb::PhysicalDeviceError::no_suitable_device)
		{
			const auto &detailedReasons = physDeviceRet.detailed_failure_reasons();
			if (!detailedReasons.empty())
			{
				err += "GPU Selection failure reasons:\n";
				for (u32 i = 0; i < detailedReasons.size(); i++)
					err += detailedReasons[i] + '\n';
			}
		}
		SendErrorPopup(err);
		return false;
	}
	vkb::PhysicalDevice physicalDevice = physDeviceRet.value();
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	auto deviceRet = deviceBuilder.build();
	if (!deviceRet)
	{
		SendErrorPopup(deviceRet.error().message());
		return false;
	}
	appData.device = deviceRet.value();
	appData.disp = appData.device.make_table();

	return true;
}

bool RenderThread::CreateSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ appData.device };
	u32 x, y;
	auto swapRet = swapchainBuilder.set_old_swapchain(appData.swapchain).build(x, y);
	if (x == 0 || y == 0)
	{
		res.x = x;
		res.y = y;
		return false;
	}
	if (!swapRet)
	{
		SendErrorPopup(swapRet.error().message() + ' ' + std::to_string(swapRet.vk_result()));
		return false;
	}
	vkb::destroy_swapchain(appData.swapchain);
	appData.swapchain = swapRet.value();
	return true;
}

bool RenderThread::GetQueues()
{
	auto gq = appData.device.get_queue(vkb::QueueType::graphics);
	if (!gq.has_value())
	{
		SendErrorPopup("failed to get graphics queue: " + gq.error().message());
		return false;
	}
	renderData.graphicsQueue = gq.value();

	auto pq = appData.device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		SendErrorPopup("failed to get present queue: " + pq.error().message());
		return false;
	}
	renderData.presentQueue = pq.value();

	auto tq = appData.device.get_queue(vkb::QueueType::transfer);
	if (!tq.has_value())
		renderData.transferQueue = renderData.graphicsQueue;
	else
		renderData.transferQueue = tq.value();
	return true;
}

bool RenderThread::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = appData.swapchain.image_format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (appData.disp.createRenderPass(&renderPassInfo, nullptr, &renderData.renderPass) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create render pass");
		return false;
	}
	return true;
}

VkShaderModule RenderThread::CreateShaderModule(const std::string &code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const u32*>(code.data());

	VkShaderModule shaderModule;
	if (appData.disp.createShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		return VK_NULL_HANDLE;

	return shaderModule;
}

VkVertexInputBindingDescription RenderThread::GetBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Resource::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> RenderThread::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Resource::Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Resource::Vertex, uv);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Resource::Vertex, col);

	return attributeDescriptions;
}

bool RenderThread::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding objectLayoutBinding = {};
    uboLayoutBinding.binding = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	VkDescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, objectLayoutBinding };
	layoutInfo.pBindings = bindings;
	
	if ( vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create descriptor set layout!");
	}

	return false;
}

bool RenderThread::CreateGraphicsPipeline()
{
	const std::filesystem::path defaultPath = std::filesystem::current_path();
	std::string vertCode = LoadFile(std::filesystem::path(defaultPath).append("Assets/Shaders/triangle.vert.spv").string());
	std::string fragCode = LoadFile(std::filesystem::path(defaultPath).append("Assets/Shaders/triangle.frag.spv").string());

	VkShaderModule vertModule = CreateShaderModule(vertCode);
	VkShaderModule fragModule = CreateShaderModule(fragCode);
	if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE)
	{
		SendErrorPopup("failed to create shader module");
		return false;
	}

	VkPipelineShaderStageCreateInfo vertStageInfo = {};
	vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStageInfo = {};
	fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragModule;
	fragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

	auto bindingDescription = GetBindingDescription();
	auto attributeDescriptions = GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)appData.swapchain.extent.width;
	viewport.height = (float)appData.swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = appData.swapchain.extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (appData.disp.createPipelineLayout(&pipelineLayoutInfo, nullptr, &renderData.pipelineLayout) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create pipeline layout");
		return false;
	}

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.dynamicStateCount = static_cast<u32>(dynamicStates.size());
	dynamicInfo.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicInfo;
	pipelineInfo.layout = renderData.pipelineLayout;
	pipelineInfo.renderPass = renderData.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (appData.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &renderData.graphicsPipeline) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create pipline");
		return false;
	}

	appData.disp.destroyShaderModule(fragModule, nullptr);
	appData.disp.destroyShaderModule(vertModule, nullptr);
	return true;
}

bool RenderThread::CreateObjectBuffer(const u32 objectCount)
{
	VkDeviceSize bufferSize = sizeof(Vec4) * objectCount;
	renderData.objectBuffers.resize(renderData.swapchainImageViews.size());
	renderData.objectBuffersMemory.resize(renderData.swapchainImageViews.size());
	for (u32 i = 0; i < renderData.swapchainImageViews.size(); i++)
	{
		if (!CreateBuffer(	bufferSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							renderData.objectBuffers[i],
							renderData.objectBuffersMemory[i]))
			return false;
	}
	return true;
}

bool RenderThread::CreateFramebuffers()
{
	renderData.swapchainImages = appData.swapchain.get_images().value();
	renderData.swapchainImageViews = appData.swapchain.get_image_views().value();

	renderData.framebuffers.resize(renderData.swapchainImageViews.size());

	for (u32 i = 0; i < renderData.swapchainImageViews.size(); i++)
	{
		VkImageView attachments[] = { renderData.swapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderData.renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = appData.swapchain.extent.width;
		framebufferInfo.height = appData.swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (appData.disp.createFramebuffer(&framebufferInfo, nullptr, &renderData.framebuffers[i]) != VK_SUCCESS)
			return false;
	}
	return true;
}

bool RenderThread::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo0 = {};
	poolInfo0.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo0.queueFamilyIndex = appData.device.get_queue_index(vkb::QueueType::graphics).value();

	if (appData.disp.createCommandPool(&poolInfo0, nullptr, &renderData.commandPool) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create command pool");
		return false;
	}

	VkCommandPoolCreateInfo poolInfo1 = {};
	poolInfo1.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo1.queueFamilyIndex = appData.device.get_queue_index(vkb::QueueType::transfer).value();

	if (appData.disp.createCommandPool(&poolInfo1, nullptr, &renderData.transfertCommandPool) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create command pool");
		return false;
	}
	return true;
}

bool RenderThread::CreateVertexBuffer(const Resource::Mesh &m)
{
	const auto &vertices = m.GetVertices();

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(	bufferSize,
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					stagingBuffer,
					stagingBufferMemory);

	void *data;
	appData.disp.mapMemory(stagingBufferMemory, 0, bufferSize, 0, &data);
	std::memcpy(data, vertices.data(), bufferSize);
	appData.disp.unmapMemory(stagingBufferMemory);

    CreateBuffer(	bufferSize,
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					renderData.vertexBuffer,
					renderData.vertexBufferMemory);
	
	CopyBuffer(stagingBuffer, renderData.vertexBuffer, bufferSize);

	appData.disp.destroyBuffer(stagingBuffer, nullptr);
    appData.disp.freeMemory(stagingBufferMemory, nullptr);
	
	return true;
}

bool RenderThread::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = renderData.transfertCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
	appData.disp.allocateCommandBuffers(&allocInfo, &commandBuffer);
    
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	appData.disp.beginCommandBuffer(commandBuffer, &beginInfo);
	
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	appData.disp.cmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	appData.disp.endCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	appData.disp.queueSubmit(renderData.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	appData.disp.queueWaitIdle(renderData.transferQueue);
	appData.disp.freeCommandBuffers(renderData.transfertCommandPool, 1, &commandBuffer);

	return true;
}

bool RenderThread::CreateCommandBuffers()
{
	renderData.commandBuffers.resize(renderData.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = renderData.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (u32)renderData.commandBuffers.size();

	if (appData.disp.allocateCommandBuffers(&allocInfo, renderData.commandBuffers.data()) != VK_SUCCESS)
	{
		SendErrorPopup("failed to allocate command buffers");
		return false;
	}

	VkCommandBufferAllocateInfo allocInfoTr = {};
	allocInfoTr.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfoTr.commandPool = renderData.commandPool;
	allocInfoTr.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfoTr.commandBufferCount = 1;

	if (appData.disp.allocateCommandBuffers(&allocInfoTr, &renderData.transferCommandBuffer) != VK_SUCCESS)
	{
		SendErrorPopup("failed to allocate transfer command buffer");
		return false;
	}

	for (u32 i = 0; i < renderData.commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (appData.disp.beginCommandBuffer(renderData.commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			SendErrorPopup("failed to begin recording command buffer");
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderData.renderPass;
		renderPassInfo.framebuffer = renderData.framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = appData.swapchain.extent;
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)appData.swapchain.extent.width;
		viewport.height = (float)appData.swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = appData.swapchain.extent;

		appData.disp.cmdSetViewport(renderData.commandBuffers[i], 0, 1, &viewport);
		appData.disp.cmdSetScissor(renderData.commandBuffers[i], 0, 1, &scissor);

		appData.disp.cmdBeginRenderPass(renderData.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		appData.disp.cmdBindPipeline(renderData.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.graphicsPipeline);

		VkBuffer vertexBuffers[] = { renderData.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		appData.disp.cmdBindVertexBuffers(renderData.commandBuffers[i], 0, 1, vertexBuffers, offsets);

		appData.disp.cmdDraw(renderData.commandBuffers[i], (u32)(sceneData.mesh.GetVertices().size()), 1, 0, 0);

		appData.disp.cmdEndRenderPass(renderData.commandBuffers[i]);

		if (appData.disp.endCommandBuffer(renderData.commandBuffers[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to record command buffer");
			return false;
		}
	}
	return true;
}

bool RenderThread::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	bufferInfo.queueFamilyIndexCount = 2;
	u32 queueFamilies[2] = {appData.device.get_queue_index(vkb::QueueType::graphics).value(), appData.device.get_queue_index(vkb::QueueType::transfer).value()};
	bufferInfo.pQueueFamilyIndices = queueFamilies;

    if (appData.disp.createBuffer(&bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
        SendErrorPopup("failed to create buffer!");
		return false;
    }

    VkMemoryRequirements memRequirements;
	appData.disp.getBufferMemoryRequirements(buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (appData.disp.allocateMemory(&allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		SendErrorPopup("failed to allocate buffer memory!");
		return false;
	}

	appData.disp.bindBufferMemory(buffer, bufferMemory, 0);
	return true;
}

bool RenderThread::CreateSyncObjects()
{
	renderData.availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderData.finishedSemaphore.resize(appData.swapchain.image_count);
	renderData.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	renderData.imageInFlight.resize(appData.swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (u32 i = 0; i < appData.swapchain.image_count; i++)
	{
		if (appData.disp.createSemaphore(&semaphoreInfo, nullptr, &renderData.finishedSemaphore[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to create sync objects");
			return false;
		}
	}

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (appData.disp.createSemaphore(&semaphoreInfo, nullptr, &renderData.availableSemaphores[i]) != VK_SUCCESS ||
			appData.disp.createFence(&fenceInfo, nullptr, &renderData.inFlightFences[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to create sync objects");
			return false;
		}
	}
	return true;
}

bool RenderThread::RecreateSwapchain()
{
	HandleResize();
	if (res.x <= 0 || res.y <= 0)
	{
		return true;
	}
	appData.disp.deviceWaitIdle();
	if (renderData.commandPool != VK_NULL_HANDLE)
	{
		appData.disp.destroyCommandPool(renderData.commandPool, nullptr);
		appData.disp.destroyCommandPool(renderData.transfertCommandPool, nullptr);
		renderData.commandPool = VK_NULL_HANDLE;

		for (u32 i = 0; i < renderData.framebuffers.size(); i++)
			appData.disp.destroyFramebuffer(renderData.framebuffers[i], nullptr);

		appData.swapchain.destroy_image_views(renderData.swapchainImageViews);
	}

	if (!CreateSwapchain())
	{
		if (res.x <= 0 || res.y <= 0)
			return true;
		return false;
	}
	if (!CreateFramebuffers()) return false;
	if (!CreateCommandPool()) return false;
	if (!CreateCommandBuffers()) return false;
	resized = false;
	return true;
}

u32 RenderThread::FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	appData.instDisp.getPhysicalDeviceMemoryProperties(appData.device.physical_device, &memProperties);
	
	for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	SendErrorPopup("failed to find suitable memory type!");
	return -1;
}

bool RenderThread::DrawFrame()
{
	if (resized)
	{
		HandleResize();
		if (res.x <= 0 || res.y <= 0)
			return true;
		else
			return RecreateSwapchain();
	}

	appData.disp.waitForFences(1, &renderData.inFlightFences[renderData.currentFrame], VK_TRUE, UINT64_MAX);

	u32 imgIndex = 0;
	VkResult result = appData.disp.acquireNextImageKHR(
		appData.swapchain, UINT64_MAX, renderData.availableSemaphores[renderData.currentFrame], VK_NULL_HANDLE, &imgIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return RecreateSwapchain();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		SendErrorPopup("failed to acquire swapchain image. Error " + result);
		return false;
	}

	if (renderData.imageInFlight[imgIndex] != VK_NULL_HANDLE)
	{
		appData.disp.waitForFences(1, &renderData.imageInFlight[imgIndex], VK_TRUE, UINT64_MAX);
	}
	renderData.imageInFlight[imgIndex] = renderData.inFlightFences[renderData.currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { renderData.availableSemaphores[renderData.currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &renderData.commandBuffers[imgIndex];

	VkSemaphore signalSemaphores[] = { renderData.finishedSemaphore[imgIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	appData.disp.resetFences(1, &renderData.inFlightFences[renderData.currentFrame]);

	if (appData.disp.queueSubmit(renderData.graphicsQueue, 1, &submitInfo, renderData.inFlightFences[renderData.currentFrame]) != VK_SUCCESS)
	{
		SendErrorPopup("failed to submit draw command buffer");
		return false;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { appData.swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imgIndex;

	result = appData.disp.queuePresentKHR(renderData.presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized)
	{
		return RecreateSwapchain();
	}
	else if (result != VK_SUCCESS)
	{
		SendErrorPopup("failed to present swapchain image");
		return false;
	}

	renderData.currentFrame = (renderData.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}

void RenderThread::Cleanup()
{
	for (u32 i = 0; i < appData.swapchain.image_count; i++)
	{
		appData.disp.destroySemaphore(renderData.finishedSemaphore[i], nullptr);
	}
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		appData.disp.destroySemaphore(renderData.availableSemaphores[i], nullptr);
		appData.disp.destroyFence(renderData.inFlightFences[i], nullptr);
	}

	appData.disp.destroyCommandPool(renderData.commandPool, nullptr);
	appData.disp.destroyCommandPool(renderData.transfertCommandPool, nullptr);

	for (u32 i = 0; i < renderData.framebuffers.size(); i++)
	{
		appData.disp.destroyFramebuffer(renderData.framebuffers[i], nullptr);
	}

	for (u32 i = 0; i < renderData.objectBuffers.size(); i++)
	{
		appData.disp.destroyBuffer(renderData.objectBuffers[i], nullptr);
		appData.disp.freeMemory(renderData.objectBuffersMemory[i], nullptr);
	}

	appData.disp.destroyPipeline(renderData.graphicsPipeline, nullptr);
	appData.disp.destroyPipelineLayout(renderData.pipelineLayout, nullptr);
	appData.disp.destroyRenderPass(renderData.renderPass, nullptr);
	appData.disp.destroyBuffer(renderData.vertexBuffer, nullptr);
	appData.disp.freeMemory(renderData.vertexBufferMemory, nullptr);

	appData.swapchain.destroy_image_views(renderData.swapchainImageViews);

	vkb::destroy_swapchain(appData.swapchain);
	vkb::destroy_device(appData.device);
	vkb::destroy_surface(appData.instance, appData.surface);
	vkb::destroy_instance(appData.instance);
}
