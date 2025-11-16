#include "RenderThread.hpp"

#include <filesystem>
#include <time.h>
#include <fstream>

using namespace Maths;

const u32 MAX_FRAMES_IN_FLIGHT = 2;

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

std::string LoadFile(const std::string& path)
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

	if (!InitVulkan())
	{
		crashed = true;
		return;
	}

	LoadAssets();

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

		if (!DrawFrame(appData, renderData))
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
	Cleanup(appData, renderData);

	if (!exit)
		crashed = true;
}

bool RenderThread::InitVulkan()
{
	return	InitDevice(appData) &&
			CreateSwapchain(appData) &&
			GetQueues(appData, renderData) &&
			CreateRenderPass(appData, renderData) &&
			CreateGraphicsPipeline(appData, renderData) &&
			CreateFramebuffers(appData, renderData) &&
			CreateCommandPool(appData, renderData) &&
			CreateCommandBuffers(appData, renderData) &&
			CreateSyncObjects(appData, renderData);
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

void RenderThread::SendErrorPopup(const std::string& err)
{
	MessageBoxA(appData.hWnd, err.c_str(), "Error!", NULL);
}

void RenderThread::SendErrorPopup(const std::wstring& err)
{
	MessageBoxW(appData.hWnd, err.c_str(), L"Error!", NULL);
}

void RenderThread::LogMessage(const std::string& msg)
{
	OutputDebugStringA(msg.c_str());
}

void RenderThread::LogMessage(const std::wstring& msg)
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

bool RenderThread::InitDevice(AppData &init)
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
	init.instance = instanceRet.value();
	init.instDisp = init.instance.make_table();
	init.surface = CreateSurfaceWin32(init.instance, init.hInstance, init.hWnd);

	vkb::PhysicalDeviceSelector physDeviceSelector(init.instance);

	auto physDeviceRet = physDeviceSelector.set_surface(init.surface).select();
	if (!physDeviceRet)
	{
		std::string err = physDeviceRet.error().message() + '\n';
		if (physDeviceRet.error() == vkb::PhysicalDeviceError::no_suitable_device)
		{
			const auto& detailedReasons = physDeviceRet.detailed_failure_reasons();
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
	init.device = deviceRet.value();
	init.disp = init.device.make_table();

	return true;
}

bool RenderThread::CreateSwapchain(AppData &init)
{
	vkb::SwapchainBuilder swapchainBuilder{ init.device };
	auto swapRet = swapchainBuilder.set_old_swapchain(init.swapchain).build();
	if (!swapRet)
	{
		SendErrorPopup(swapRet.error().message() + ' ' + std::to_string(swapRet.vk_result()));
		return false;
	}
	vkb::destroy_swapchain(init.swapchain);
	init.swapchain = swapRet.value();
	return true;
}

bool RenderThread::GetQueues(AppData &init, RenderData &data)
{
	auto gq = init.device.get_queue(vkb::QueueType::graphics);
	if (!gq.has_value())
	{
		SendErrorPopup("failed to get graphics queue: " + gq.error().message());
		return false;
	}
	data.graphicsQueue = gq.value();

	auto pq = init.device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		SendErrorPopup("failed to get present queue: " + pq.error().message());
		return false;
	}
	data.presentQueue = pq.value();
	return true;
}

bool RenderThread::CreateRenderPass(AppData &init, RenderData &data)
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = init.swapchain.image_format;
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

	if (init.disp.createRenderPass(&renderPassInfo, nullptr, &data.renderPass) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create render pass");
		return false;
	}
	return true;
}

VkShaderModule RenderThread::CreateShaderModule(AppData &init, const std::string &code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const u32*>(code.data());

	VkShaderModule shaderModule;
	if (init.disp.createShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS)
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

std::array<VkVertexInputAttributeDescription, 2> RenderThread::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Resource::Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Resource::Vertex, col);

	return attributeDescriptions;
}

bool RenderThread::CreateGraphicsPipeline(AppData &init, RenderData &data)
{
	const std::filesystem::path defaultPath = std::filesystem::current_path();
	auto vertCode = LoadFile(std::filesystem::path(defaultPath).append("Assets/Shaders/triangle.vert.spv").string());
	auto fragCode = LoadFile(std::filesystem::path(defaultPath).append("Assets/Shaders/triangle.frag.spv").string());

	VkShaderModule vertModule = CreateShaderModule(init, vertCode);
	VkShaderModule fragModule = CreateShaderModule(init, fragCode);
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

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)init.swapchain.extent.width;
	viewport.height = (float)init.swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = init.swapchain.extent;

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

	if (init.disp.createPipelineLayout(&pipelineLayoutInfo, nullptr, &data.pipelineLayout) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create pipeline layout");
		return false;
	}

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
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
	pipelineInfo.layout = data.pipelineLayout;
	pipelineInfo.renderPass = data.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (init.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &data.graphicsPipeline) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create pipline");
		return false;
	}

	init.disp.destroyShaderModule(fragModule, nullptr);
	init.disp.destroyShaderModule(vertModule, nullptr);
	return true;
}

bool RenderThread::CreateFramebuffers(AppData &init, RenderData &data)
{
	data.swapchainImages = init.swapchain.get_images().value();
	data.swapchainImageViews = init.swapchain.get_image_views().value();

	data.framebuffers.resize(data.swapchainImageViews.size());

	for (u32 i = 0; i < data.swapchainImageViews.size(); i++)
	{
		VkImageView attachments[] = { data.swapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = data.renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = init.swapchain.extent.width;
		framebufferInfo.height = init.swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (init.disp.createFramebuffer(&framebufferInfo, nullptr, &data.framebuffers[i]) != VK_SUCCESS)
			return false;
	}
	return true;
}

bool RenderThread::CreateCommandPool(AppData &init, RenderData &data)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = init.device.get_queue_index(vkb::QueueType::graphics).value();

	if (init.disp.createCommandPool(&poolInfo, nullptr, &data.commandPool) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create command pool");
		return false;
	}
	return true;
}

bool RenderThread::CreateCommandBuffers(AppData &init, RenderData &data)
{
	data.commandBuffers.resize(data.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = data.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)data.commandBuffers.size();

	if (init.disp.allocateCommandBuffers(&allocInfo, data.commandBuffers.data()) != VK_SUCCESS)
	{
		SendErrorPopup("failed to allocate command buffers");
		return false;
	}

	for (u32 i = 0; i < data.commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (init.disp.beginCommandBuffer(data.commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			SendErrorPopup("failed to begin recording command buffer");
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = data.renderPass;
		renderPassInfo.framebuffer = data.framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = init.swapchain.extent;
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)init.swapchain.extent.width;
		viewport.height = (float)init.swapchain.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = init.swapchain.extent;

		init.disp.cmdSetViewport(data.commandBuffers[i], 0, 1, &viewport);
		init.disp.cmdSetScissor(data.commandBuffers[i], 0, 1, &scissor);

		init.disp.cmdBeginRenderPass(data.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		init.disp.cmdBindPipeline(data.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphicsPipeline);

		init.disp.cmdDraw(data.commandBuffers[i], 3, 1, 0, 0);

		init.disp.cmdEndRenderPass(data.commandBuffers[i]);

		if (init.disp.endCommandBuffer(data.commandBuffers[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to record command buffer");
			return false;
		}
	}
	return true;
}

bool RenderThread::CreateSyncObjects(AppData &init, RenderData &data)
{
	data.availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	data.finishedSemaphore.resize(init.swapchain.image_count);
	data.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	data.imageInFlight.resize(init.swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (u32 i = 0; i < init.swapchain.image_count; i++)
	{
		if (init.disp.createSemaphore(&semaphoreInfo, nullptr, &data.finishedSemaphore[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to create sync objects");
			return false;
		}
	}

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (init.disp.createSemaphore(&semaphoreInfo, nullptr, &data.availableSemaphores[i]) != VK_SUCCESS ||
			init.disp.createFence(&fenceInfo, nullptr, &data.inFlightFences[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to create sync objects");
			return false;
		}
	}
	return true;
}

bool RenderThread::RecreateSwapchain(AppData &init, RenderData &data)
{
	HandleResize();
	if (res.x <= 0 || res.y <= 0)
	{
		return true;
	}
	init.disp.deviceWaitIdle();
	init.disp.destroyCommandPool(data.commandPool, nullptr);

	for (auto framebuffer : data.framebuffers)
	{
		init.disp.destroyFramebuffer(framebuffer, nullptr);
	}

	init.swapchain.destroy_image_views(data.swapchainImageViews);

	if (!CreateSwapchain(init)) return false;
	if (!CreateFramebuffers(init, data)) return false;
	if (!CreateCommandPool(init, data)) return false;
	if (!CreateCommandBuffers(init, data)) return false;
	resized = false;
	return true;
}

bool RenderThread::DrawFrame(AppData &init, RenderData &data)
{
	if (resized)
	{
		HandleResize();
		if (res.x <= 0 || res.y <= 0)
			return true;
		else
			return RecreateSwapchain(init, data);
	}

	init.disp.waitForFences(1, &data.inFlightFences[data.currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imgIndex = 0;
	VkResult result = init.disp.acquireNextImageKHR(
		init.swapchain, UINT64_MAX, data.availableSemaphores[data.currentFrame], VK_NULL_HANDLE, &imgIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return RecreateSwapchain(init, data);
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		SendErrorPopup("failed to acquire swapchain image. Error " + result);
		return false;
	}

	if (data.imageInFlight[imgIndex] != VK_NULL_HANDLE)
	{
		init.disp.waitForFences(1, &data.imageInFlight[imgIndex], VK_TRUE, UINT64_MAX);
	}
	data.imageInFlight[imgIndex] = data.inFlightFences[data.currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { data.availableSemaphores[data.currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &data.commandBuffers[imgIndex];

	VkSemaphore signalSemaphores[] = { data.finishedSemaphore[imgIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	init.disp.resetFences(1, &data.inFlightFences[data.currentFrame]);

	if (init.disp.queueSubmit(data.graphicsQueue, 1, &submitInfo, data.inFlightFences[data.currentFrame]) != VK_SUCCESS)
	{
		SendErrorPopup("failed to submit draw command buffer");
		return false;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { init.swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imgIndex;

	result = init.disp.queuePresentKHR(data.presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized)
	{
		return RecreateSwapchain(init, data);
	}
	else if (result != VK_SUCCESS)
	{
		SendErrorPopup("failed to present swapchain image");
		return false;
	}

	data.currentFrame = (data.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}

void RenderThread::Cleanup(AppData &init, RenderData &data)
{
	for (u32 i = 0; i < init.swapchain.image_count; i++)
	{
		init.disp.destroySemaphore(data.finishedSemaphore[i], nullptr);
	}
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		init.disp.destroySemaphore(data.availableSemaphores[i], nullptr);
		init.disp.destroyFence(data.inFlightFences[i], nullptr);
	}

	init.disp.destroyCommandPool(data.commandPool, nullptr);

	for (auto framebuffer : data.framebuffers)
	{
		init.disp.destroyFramebuffer(framebuffer, nullptr);
	}

	init.disp.destroyPipeline(data.graphicsPipeline, nullptr);
	init.disp.destroyPipelineLayout(data.pipelineLayout, nullptr);
	init.disp.destroyRenderPass(data.renderPass, nullptr);

	init.swapchain.destroy_image_views(data.swapchainImageViews);

	vkb::destroy_swapchain(init.swapchain);
	vkb::destroy_device(init.device);
	vkb::destroy_surface(init.instance, init.surface);
	vkb::destroy_instance(init.instance);
}
