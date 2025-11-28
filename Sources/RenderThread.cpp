#include "RenderThread.hpp"

#include "Resource/Texture.hpp"

#include <filesystem>
#include <time.h>
#include <fstream>

using namespace Maths;

const char *alphaBitStrings[] =
{
	"VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR",
    "VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR",
    "VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR",
    "VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR"
};

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

void RenderThread::Init(HWND hwnd, HINSTANCE hinstance, GameThread *gm, Maths::IVec2 resIn, u32 targetDevice)
{
	appData.hWnd = hwnd;
	appData.hInstance = hinstance;
	appData.gm = gm;
	res = resIn;
	thread = std::thread(&RenderThread::ThreadFunc, this, targetDevice);
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

void RenderThread::HandleResize()
{
	res.x = (s32)(storedRes & 0xffffffff);
	res.y = (s32)(storedRes >> 32);
	if (res.x != swapRes.x || res.y != swapRes.y)
		resized = true;
}

void RenderThread::InitThread()
{
	SetThreadDescription(GetCurrentThread(), L"Render Thread");
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	start = now.time_since_epoch();
}

void RenderThread::ThreadFunc(u32 targetDevice)
{
	InitThread();
	LoadAssets();

	if (!InitVulkan(targetDevice))
	{
		crashed = true;
		return;
	}

	u32 counter = 0;
	u32 tm0 = 0;
	while (!exit)
	{
		std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch() - start;
		auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
		f64 iTime = micros / 1000000.0;
		f32 deltaTime = static_cast<f32>(iTime - appTime);
		appTime = iTime;
		u32 tm1 = (u32)(iTime);
		if (tm0 != tm1)
		{
			tm0 = tm1;
			GameThread::LogMessage("FPS: " + std::to_string(counter) + "\n");
			counter = 0;
		}
		counter++;
		
		HandleResize();

		if (!DrawFrame())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	appData.disp.deviceWaitIdle();
	UnloadAssets();
	Cleanup();

	if (!exit)
		crashed = true;
}

bool RenderThread::InitVulkan(u32 targetDevice)
{
	return	InitDevice(targetDevice) &&
			CreateSwapchain() &&
			GetQueues() &&
			CreateRenderPass() &&
			CreateDescriptorSetLayout() &&
			CreateGraphicsPipeline() &&
			CreateFramebuffers() &&
			CreateCommandPool() &&
			CreateVertexBuffer(sceneData.mesh) &&
			CreateObjectBuffer(OBJECT_COUNT) &&
			CreateDescriptorPool() &&
			CreateDescriptorSets() &&
			CreateCommandBuffers() &&
			CreateSyncObjects();
}

void RenderThread::LoadAssets()
{
	sceneData.mesh.CreateDefaultCube();
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
				text += s;
			else
				text += L"Unknown error";
			GameThread::SendErrorPopup(text);
		}
		surface = VK_NULL_HANDLE;
	}
	return surface;
}

bool RenderThread::InitDevice(u32 targetDevice)
{
	GameThread::LogMessage("Initializing Vulkan...\n");
	auto systemInfoRes = vkb::SystemInfo::get_system_info();
	if (!systemInfoRes)
	{
		GameThread::SendErrorPopup("Error creating vulkan instance: " + systemInfoRes.error().message());
		return false;
	}
	auto &systemInfo = systemInfoRes.value();
	GameThread::LogMessage("Available extensions:\n");
	for (u32 i = 0; i < systemInfo.available_extensions.size(); i++)
		GameThread::LogMessage(std::string("- ") + systemInfo.available_extensions[i].extensionName + "\n");

	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.enable_extension(VK_KHR_SURFACE_EXTENSION_NAME);
	instanceBuilder.enable_extension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	instanceBuilder.set_app_name("Vulkan Demo").set_app_version(VK_MAKE_VERSION(1, 0, 0));
	instanceBuilder.set_engine_name("Ligma Engine").request_validation_layers();

	instanceBuilder.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void*) -> VkBool32 {
			if (messageSeverity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
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
		GameThread::SendErrorPopup("Error creating vulkan instance: " + instanceRet.error().message());
		return false;
	}
	appData.instance = instanceRet.value();
	appData.instDisp = appData.instance.make_table();
	appData.surface = CreateSurfaceWin32(appData.instance, appData.hInstance, appData.hWnd);

	vkb::PhysicalDeviceSelector physDeviceSelector(appData.instance);
	auto devices = physDeviceSelector.set_surface(appData.surface).select_devices();
	if (!devices)
	{
		std::string err = "No suitable GPU found: " + devices.error().message() + '\n';
		if (devices.error() == vkb::PhysicalDeviceError::no_suitable_device)
		{
			const auto &detailedReasons = devices.detailed_failure_reasons();
			if (!detailedReasons.empty())
			{
				err += "GPU Selection failure reasons:\n";
				for (u32 i = 0; i < detailedReasons.size(); i++)
					err += detailedReasons[i] + '\n';
			}
		}
		GameThread::SendErrorPopup(err);
		return false;
	}
	GameThread::LogMessage("Suitable GPU(s):\n");
	auto &deviceList = devices.value();
	for (u32 i = 0; i < deviceList.size(); i++)
	{
		GameThread::LogMessage(std::to_string(i) + ": " + deviceList[i].name + "\n");
	}
	if (targetDevice >= deviceList.size())
	{
		GameThread::LogMessage("Requested GPU id " + std::to_string(targetDevice) + " is not available. Defaulting to first device.\n");
		targetDevice = 0;
	}
	GameThread::LogMessage("Using GPU device " + std::to_string(targetDevice) + ": " + deviceList[targetDevice].name + "\n");
	vkb::PhysicalDevice physicalDevice = deviceList[targetDevice];
	VkPhysicalDeviceFeatures features = {};
	features.logicOp = 1;
	physicalDevice.enable_features_if_present(features);
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	auto deviceRet = deviceBuilder.build();
	if (!deviceRet)
	{
		GameThread::SendErrorPopup("Error creating device: " + deviceRet.error().message());
		return false;
	}
	appData.device = deviceRet.value();
	appData.disp = appData.device.make_table();

	return true;
}

bool init = false;
bool RenderThread::CreateSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder = vkb::SwapchainBuilder(appData.device.physical_device, appData.device, appData.surface);
	u32 x, y;
	VkSurfaceFormatKHR format = {};
	format.colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	format.format = VK_FORMAT_B8G8R8A8_UNORM;
	swapchainBuilder.set_desired_format(format);
	swapchainBuilder.set_composite_alpha_flags(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
	if (!init)
	{
		init = true;
		VkSurfaceCapabilitiesKHR surfaceCaps = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(appData.device.physical_device, appData.surface, &surfaceCaps);
		GameThread::LogMessage("Supported alpha composite mode(s):\n");
		for (u32 i = 0; i < 4; i++)
		{
			if ((1 << i) & surfaceCaps.supportedCompositeAlpha)
				GameThread::LogMessage(std::string("- ") + alphaBitStrings[i] + "\n");
		}
	}
	auto swapRet = swapchainBuilder.set_old_swapchain(appData.swapchain).build(x, y);
	swapRes.x = x;
	swapRes.y = y;
	if (x == 0 || y == 0)
	{
		res.x = x;
		res.y = y;
		return false;
	}
	if (!swapRet)
	{
		GameThread::SendErrorPopup("Error creating swapchain: " + swapRet.error().message() + ' ' + std::to_string(swapRet.vk_result()));
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
		GameThread::SendErrorPopup("failed to get graphics queue: " + gq.error().message());
		return false;
	}
	renderData.graphicsQueue = gq.value();

	auto pq = appData.device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		GameThread::SendErrorPopup("failed to get present queue: " + pq.error().message());
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
		GameThread::SendErrorPopup("failed to create render pass");
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

bool RenderThread::CreateImage(	IVec2 res, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
								VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &memory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = (u32)(res.x);
	imageInfo.extent.height = (u32)(res.y);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (appData.disp.createImage(&imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to create image!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	appData.disp.getImageMemoryRequirements(image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (appData.disp.allocateMemory(&allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to allocate image memory!");
		return false;
	}

	appData.disp.bindImageMemory(image, memory, 0);
	return true;
}

VkVertexInputBindingDescription RenderThread::GetBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Resource::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> RenderThread::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

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

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Resource::Vertex, norm);

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
    objectLayoutBinding.binding = 1;
    objectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectLayoutBinding.descriptorCount = 1;
	objectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	objectLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	VkDescriptorSetLayoutBinding bindings[2] = { uboLayoutBinding, objectLayoutBinding };
	layoutInfo.pBindings = bindings;
	
	if (appData.disp.createDescriptorSetLayout(&layoutInfo, nullptr, &renderData.descriptorSetLayout) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to create descriptor set layout");
		return false;
	}

	return true;
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
		GameThread::SendErrorPopup("failed to create shader module");
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
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &renderData.descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (appData.disp.createPipelineLayout(&pipelineLayoutInfo, nullptr, &renderData.pipelineLayout) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to create pipeline layout");
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
		GameThread::SendErrorPopup("failed to create pipline");
		return false;
	}

	appData.disp.destroyShaderModule(fragModule, nullptr);
	appData.disp.destroyShaderModule(vertModule, nullptr);
	return true;
}

bool RenderThread::CreateObjectBuffer(const u32 objectCount)
{
	VkDeviceSize bufferSize = sizeof(Vec4) * objectCount * 2 + sizeof(Mat4);
	renderData.objectBuffers.resize(renderData.swapchainImageViews.size());
	renderData.objectBuffersMemory.resize(renderData.swapchainImageViews.size());
	renderData.objectBuffersMapped.resize(renderData.swapchainImageViews.size());

	bool success = true;
	for (u32 i = 0; i < renderData.swapchainImageViews.size(); i++)
	{
		success &= CreateBuffer(bufferSize,
								VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
								VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
								renderData.objectBuffers[i],
								renderData.objectBuffersMemory[i]);
	
		success &= appData.disp.mapMemory(	renderData.objectBuffersMemory[i],
											0,
											bufferSize,
											0,
											reinterpret_cast<void**>(&renderData.objectBuffersMapped[i])) == VK_SUCCESS;
	}
	return success;
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
		GameThread::SendErrorPopup("failed to create command pool");
		return false;
	}

	VkCommandPoolCreateInfo poolInfo1 = {};
	poolInfo1.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	auto transfer = appData.device.get_queue_index(vkb::QueueType::transfer);
	poolInfo1.queueFamilyIndex = transfer.has_value() ? transfer.value() : poolInfo0.queueFamilyIndex;

	if (appData.disp.createCommandPool(&poolInfo1, nullptr, &renderData.transfertCommandPool) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to create command pool");
		return false;
	}
	return true;
}

bool RenderThread::CreateTextureImage()
{
	IVec2 res;
	u8 *pixels = Resource::Texture::ReadTexture("Assets/Textures/FISH.png", res);
	if (!pixels)
	{
		GameThread::SendErrorPopup("failed to load texture");
		return false;
	}
	u64 imageSize = sizeof(u32) * res.x * res.y;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void *data;
	appData.disp.mapMemory(stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	appData.disp.unmapMemory(stagingBufferMemory);

	Resource::Texture::FreeTextureData(pixels);

	CreateImage(res, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderData.textureImage, renderData.textureImageMemory);

	return true;
}

VkCommandBuffer RenderThread::BeginSingleTimeCommands(VkCommandPool targetPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = targetPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
	appData.disp.allocateCommandBuffers(&allocInfo, &commandBuffer);
    
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	appData.disp.beginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void RenderThread::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool targetPool, VkQueue targetQueue)
{
	appData.disp.endCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	appData.disp.queueSubmit(targetQueue, 1, &submitInfo, VK_NULL_HANDLE);
	appData.disp.queueWaitIdle(targetQueue);
	appData.disp.freeCommandBuffers(targetPool, 1, &commandBuffer);
}

bool RenderThread::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();
	//CreateImage(res.x, res.y, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderData.depthImage, renderData.depthImageMemory);
	//renderData.depthImageView = CreateImageView(depthImage, depthFormat);

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
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(renderData.transfertCommandPool);
	
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	appData.disp.cmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer, renderData.transfertCommandPool, renderData.transferQueue);

	return true;
}

bool RenderThread::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(renderData.commandPool);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	appData.disp.cmdPipelineBarrier(
	    commandBuffer,
	    0 /* TODO */, 0 /* TODO */,
	    0,
	    0, nullptr,
	    0, nullptr,
	    1, &barrier
	);

    EndSingleTimeCommands(commandBuffer, renderData.commandPool, renderData.graphicsQueue);

	return true;
}

void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    //VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    //endSingleTimeCommands(commandBuffer);
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
		GameThread::SendErrorPopup("failed to allocate command buffers");
		return false;
	}

	VkCommandBufferAllocateInfo allocInfoTr = {};
	allocInfoTr.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfoTr.commandPool = renderData.commandPool;
	allocInfoTr.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfoTr.commandBufferCount = 1;

	if (appData.disp.allocateCommandBuffers(&allocInfoTr, &renderData.transferCommandBuffer) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to allocate transfer command buffer");
		return false;
	}

	for (u32 i = 0; i < renderData.commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (appData.disp.beginCommandBuffer(renderData.commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			GameThread::SendErrorPopup("failed to begin recording command buffer");
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderData.renderPass;
		renderPassInfo.framebuffer = renderData.framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = appData.swapchain.extent;
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 0.0f } } };
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

		appData.disp.cmdBindDescriptorSets(renderData.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, renderData.pipelineLayout, 0, 1, &renderData.descriptorSets[renderData.currentFrame], 0, nullptr);

		appData.disp.cmdDraw(renderData.commandBuffers[i], (u32)(sceneData.mesh.GetVertices().size()), OBJECT_COUNT, 0, 0);

		appData.disp.cmdEndRenderPass(renderData.commandBuffers[i]);

		if (appData.disp.endCommandBuffer(renderData.commandBuffers[i]) != VK_SUCCESS)
		{
			GameThread::SendErrorPopup("failed to record command buffer");
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
	auto transfer = appData.device.get_queue_index(vkb::QueueType::transfer);
	auto graph = appData.device.get_queue_index(vkb::QueueType::graphics);
	u32 queueFamilies[2] = {appData.device.get_queue_index(vkb::QueueType::graphics).value(), transfer.has_value() ? transfer.value() : graph.has_value()};
	bufferInfo.pQueueFamilyIndices = queueFamilies;

    if (appData.disp.createBuffer(&bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
        GameThread::SendErrorPopup("failed to create buffer!");
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
		GameThread::SendErrorPopup("failed to allocate buffer memory!");
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
			GameThread::SendErrorPopup("failed to create sync objects");
			return false;
		}
	}

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (appData.disp.createSemaphore(&semaphoreInfo, nullptr, &renderData.availableSemaphores[i]) != VK_SUCCESS ||
			appData.disp.createFence(&fenceInfo, nullptr, &renderData.inFlightFences[i]) != VK_SUCCESS)
		{
			GameThread::SendErrorPopup("failed to create sync objects");
			return false;
		}
	}
	return true;
}

bool RenderThread::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize0 = {};
	poolSize0.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize0.descriptorCount = 256;
	VkDescriptorPoolSize poolSize1 = {};
	poolSize1.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSize1.descriptorCount = 256;

	VkDescriptorPoolSize pools[2] = {poolSize0, poolSize1};
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = pools;
	poolInfo.maxSets = 256;

	
	if (appData.disp.createDescriptorPool(&poolInfo, nullptr, &renderData.descriptorPool) != VK_SUCCESS)
	{
	    GameThread::SendErrorPopup("failed to create descriptor pool");
		return false;
	}

	return true;
}

bool RenderThread::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, renderData.descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = renderData.descriptorPool;
	allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	allocInfo.pSetLayouts = layouts.data();

	renderData.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (appData.disp.allocateDescriptorSets(&allocInfo, renderData.descriptorSets.data()) != VK_SUCCESS)
	{
		GameThread::SendErrorPopup("failed to allocate descriptor sets");
		return false;
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo0 = {};
		bufferInfo0.buffer = renderData.objectBuffers[i];
		bufferInfo0.offset = 0;
		bufferInfo0.range = sizeof(Mat4);

		VkDescriptorBufferInfo bufferInfo1 = {};
		bufferInfo1.buffer = renderData.objectBuffers[i];
		bufferInfo1.offset = sizeof(Mat4);
		bufferInfo1.range = sizeof(Vec4) * OBJECT_COUNT * 2;

		VkWriteDescriptorSet descriptorWrite0 = {};
		descriptorWrite0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite0.dstSet = renderData.descriptorSets[i];
		descriptorWrite0.dstBinding = 0;
		descriptorWrite0.dstArrayElement = 0;
		descriptorWrite0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite0.descriptorCount = 1;
		descriptorWrite0.pBufferInfo = &bufferInfo0;
		descriptorWrite0.pImageInfo = nullptr; // Optional
		descriptorWrite0.pTexelBufferView = nullptr; // Optional

		VkWriteDescriptorSet descriptorWrite1 = {};
		descriptorWrite1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite1.dstSet = renderData.descriptorSets[i];
		descriptorWrite1.dstBinding = 1;
		descriptorWrite1.dstArrayElement = 0;
		descriptorWrite1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrite1.descriptorCount = 1;
		descriptorWrite1.pBufferInfo = &bufferInfo1;
		descriptorWrite1.pImageInfo = nullptr; // Optional
		descriptorWrite1.pTexelBufferView = nullptr; // Optional


		VkWriteDescriptorSet descriptorArray[2] = {descriptorWrite0, descriptorWrite1};
		appData.disp.updateDescriptorSets(2, descriptorArray, 0, nullptr);
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

bool RenderThread::UpdateUniformBuffer(u32 image)
{
	Vec4 *dataPtr = renderData.objectBuffersMapped[image];
	const auto &mat = appData.gm->GetViewProjectionMatrix();
	const Vec4 *matPtr = reinterpret_cast<const Vec4*>(mat.content);
	for (u32 i = 0; i < 4; i++)
	{
		dataPtr[i] = matPtr[i];
	}

	const auto& data = appData.gm->GetSimulationData();
	if (data.size() < OBJECT_COUNT * 2)
		return true;
	std::copy(data.data(), data.data() + OBJECT_COUNT * 2, dataPtr + 4);

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

	GameThread::SendErrorPopup("failed to find suitable memory type!");
	return -1;
}

VkFormat RenderThread::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(appData.device.physical_device, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		    return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		    return format;
	}

	GameThread::SendErrorPopup("failed to find supported format!");
	return VK_FORMAT_UNDEFINED;
}

VkFormat RenderThread::FindDepthFormat()
{
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool RenderThread::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool RenderThread::DrawFrame()
{
	if (resized)
	{
		HandleResize();
		if (res.x <= 0 || res.y <= 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			return true;
		}
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
		GameThread::SendErrorPopup("failed to acquire swapchain image. Error " + result);
		return false;
	}

	if (renderData.imageInFlight[imgIndex] != VK_NULL_HANDLE)
	{
		appData.disp.waitForFences(1, &renderData.imageInFlight[imgIndex], VK_TRUE, UINT64_MAX);
	}
	renderData.imageInFlight[imgIndex] = renderData.inFlightFences[renderData.currentFrame];

	UpdateUniformBuffer(renderData.currentFrame);

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
		GameThread::SendErrorPopup("failed to submit draw command buffer");
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
		GameThread::SendErrorPopup("failed to present swapchain image");
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
	appData.disp.destroyDescriptorPool(renderData.descriptorPool, nullptr);
	appData.disp.destroyDescriptorSetLayout(renderData.descriptorSetLayout, nullptr);
	appData.disp.freeMemory(renderData.vertexBufferMemory, nullptr);

	appData.swapchain.destroy_image_views(renderData.swapchainImageViews);

	vkb::destroy_swapchain(appData.swapchain);
	vkb::destroy_device(appData.device);
	vkb::destroy_surface(appData.instance, appData.surface);
	vkb::destroy_instance(appData.instance);
}
