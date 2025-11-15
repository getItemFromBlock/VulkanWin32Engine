#include "RenderThread.hpp"

#include <filesystem>
#include <time.h>
#include <fstream>

using namespace Maths;

const int MAX_FRAMES_IN_FLIGHT = 2;

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

/*
void RenderThread::CopyToScreen(s64 dt)
{
	HDC hdc = GetDC(hwnd);
	BITMAPINFO info = { 0 };
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = res.x;
	info.bmiHeader.biHeight = -res.y; // top-down image 
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biSizeImage = sizeof(u32) * res.x * res.y;
	int t = SetDIBitsToDevice(hdc, 0, 0, res.x, res.y, 0, 0, 0, res.y, colorBuffer.data(), &info, DIB_RGB_COLORS);
	RECT r = {0,0,150,50};
	std::string s = res.toString();
	DrawTextA(hdc, s.c_str(), -1, &r, 0);
	r.top = 25;
	r.bottom = 75;
	s = std::to_string(dt);
	DrawTextA(hdc, s.c_str(), -1, &r, 0);
	ReleaseDC(hwnd, hdc);
}
*/

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
		int ret = GetErrorInfo(0, &e);
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
	vkb::InstanceBuilder instance_builder;
	instance_builder.set_app_name("Vulkan Demo").set_app_version(VK_MAKE_VERSION(1, 0, 0));
	instance_builder.set_engine_name("Ligma Engine").request_validation_layers();

	instance_builder.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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

	auto instance_ret = instance_builder.build();

	if (!instance_ret)
	{
		SendErrorPopup(instance_ret.error().message());
		return false;
	}
	init.instance = instance_ret.value();
	init.inst_disp = init.instance.make_table();
	init.surface = CreateSurfaceWin32(init.instance, init.hInstance, init.hWnd);

	vkb::PhysicalDeviceSelector phys_device_selector(init.instance);

	auto phys_device_ret = phys_device_selector.set_surface(init.surface).select();
	if (!phys_device_ret)
	{
		std::string err = phys_device_ret.error().message() + '\n';
		if (phys_device_ret.error() == vkb::PhysicalDeviceError::no_suitable_device)
		{
			const auto& detailed_reasons = phys_device_ret.detailed_failure_reasons();
			if (!detailed_reasons.empty())
			{
				err += "GPU Selection failure reasons:\n";
				for (const std::string& reason : detailed_reasons)
					err += reason + '\n';
			}
		}
		SendErrorPopup(err);
		return false;
	}
	vkb::PhysicalDevice physical_device = phys_device_ret.value();
	vkb::DeviceBuilder device_builder{ physical_device };

	auto device_ret = device_builder.build();
	if (!device_ret)
	{
		SendErrorPopup(device_ret.error().message());
		return false;
	}
	init.device = device_ret.value();
	init.disp = init.device.make_table();

	return true;
}

bool RenderThread::CreateSwapchain(AppData &init)
{
	vkb::SwapchainBuilder swapchain_builder{ init.device };
	auto swap_ret = swapchain_builder.set_old_swapchain(init.swapchain).build();
	if (!swap_ret)
	{
		SendErrorPopup(swap_ret.error().message() + ' ' + std::to_string(swap_ret.vk_result()));
		return false;
	}
	vkb::destroy_swapchain(init.swapchain);
	init.swapchain = swap_ret.value();
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
	data.graphics_queue = gq.value();

	auto pq = init.device.get_queue(vkb::QueueType::present);
	if (!pq.has_value())
	{
		SendErrorPopup("failed to get present queue: " + pq.error().message());
		return false;
	}
	data.present_queue = pq.value();
	return true;
}

bool RenderThread::CreateRenderPass(AppData &init, RenderData &data)
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = init.swapchain.image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	if (init.disp.createRenderPass(&render_pass_info, nullptr, &data.render_pass) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create render pass");
		return false;
	}
	return true;
}

VkShaderModule RenderThread::CreateShaderModule(AppData &init, const std::string &code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const u32*>(code.data());

	VkShaderModule shaderModule;
	if (init.disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	return shaderModule;
}

bool RenderThread::CreateGraphicsPipeline(AppData &init, RenderData &data)
{
	const std::filesystem::path defaultPath = std::filesystem::current_path();
	auto vert_code = LoadFile(std::filesystem::path(defaultPath).append("Assets/Shaders/triangle.vert.spv").string());
	auto frag_code = LoadFile(std::filesystem::path(defaultPath).append("Assets/Shaders/triangle.frag.spv").string());

	VkShaderModule vert_module = CreateShaderModule(init, vert_code);
	VkShaderModule frag_module = CreateShaderModule(init, frag_code);
	if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
	{
		SendErrorPopup("failed to create shader module");
		return false;
	}

	VkPipelineShaderStageCreateInfo vert_stage_info = {};
	vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_stage_info.module = vert_module;
	vert_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_stage_info = {};
	frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_stage_info.module = frag_module;
	frag_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

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

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

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

	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &colorBlendAttachment;
	color_blending.blendConstants[0] = 0.0f;
	color_blending.blendConstants[1] = 0.0f;
	color_blending.blendConstants[2] = 0.0f;
	color_blending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0;
	pipeline_layout_info.pushConstantRangeCount = 0;

	if (init.disp.createPipelineLayout(&pipeline_layout_info, nullptr, &data.pipeline_layout) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create pipeline layout");
		return false;
	}

	std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic_info = {};
	dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_info.pDynamicStates = dynamic_states.data();

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_info;
	pipeline_info.layout = data.pipeline_layout;
	pipeline_info.renderPass = data.render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

	if (init.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &data.graphics_pipeline) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create pipline");
		return false;
	}

	init.disp.destroyShaderModule(frag_module, nullptr);
	init.disp.destroyShaderModule(vert_module, nullptr);
	return true;
}

bool RenderThread::CreateFramebuffers(AppData &init, RenderData &data)
{
	data.swapchain_images = init.swapchain.get_images().value();
	data.swapchain_image_views = init.swapchain.get_image_views().value();

	data.framebuffers.resize(data.swapchain_image_views.size());

	for (size_t i = 0; i < data.swapchain_image_views.size(); i++)
	{
		VkImageView attachments[] = { data.swapchain_image_views[i] };

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = data.render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = init.swapchain.extent.width;
		framebuffer_info.height = init.swapchain.extent.height;
		framebuffer_info.layers = 1;

		if (init.disp.createFramebuffer(&framebuffer_info, nullptr, &data.framebuffers[i]) != VK_SUCCESS)
		{
			return false;
		}
	}
	return true;
}

bool RenderThread::CreateCommandPool(AppData &init, RenderData &data)
{
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = init.device.get_queue_index(vkb::QueueType::graphics).value();

	if (init.disp.createCommandPool(&pool_info, nullptr, &data.command_pool) != VK_SUCCESS)
	{
		SendErrorPopup("failed to create command pool");
		return false;
	}
	return true;
}

bool RenderThread::CreateCommandBuffers(AppData &init, RenderData &data)
{
	data.command_buffers.resize(data.framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = data.command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)data.command_buffers.size();

	if (init.disp.allocateCommandBuffers(&allocInfo, data.command_buffers.data()) != VK_SUCCESS)
	{
		SendErrorPopup("failed to allocate command buffers");
		return false;
	}

	for (size_t i = 0; i < data.command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (init.disp.beginCommandBuffer(data.command_buffers[i], &begin_info) != VK_SUCCESS)
		{
			SendErrorPopup("failed to begin recording command buffer");
			return false;
		}

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = data.render_pass;
		render_pass_info.framebuffer = data.framebuffers[i];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = init.swapchain.extent;
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clearColor;

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

		init.disp.cmdSetViewport(data.command_buffers[i], 0, 1, &viewport);
		init.disp.cmdSetScissor(data.command_buffers[i], 0, 1, &scissor);

		init.disp.cmdBeginRenderPass(data.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		init.disp.cmdBindPipeline(data.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphics_pipeline);

		init.disp.cmdDraw(data.command_buffers[i], 3, 1, 0, 0);

		init.disp.cmdEndRenderPass(data.command_buffers[i]);

		if (init.disp.endCommandBuffer(data.command_buffers[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to record command buffer");
			return false;
		}
	}
	return true;
}

bool RenderThread::CreateSyncObjects(AppData &init, RenderData &data)
{
	data.available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	data.finished_semaphore.resize(init.swapchain.image_count);
	data.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
	data.image_in_flight.resize(init.swapchain.image_count, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < init.swapchain.image_count; i++)
	{
		if (init.disp.createSemaphore(&semaphore_info, nullptr, &data.finished_semaphore[i]) != VK_SUCCESS)
		{
			SendErrorPopup("failed to create sync objects");
			return false;
		}
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (init.disp.createSemaphore(&semaphore_info, nullptr, &data.available_semaphores[i]) != VK_SUCCESS ||
			init.disp.createFence(&fence_info, nullptr, &data.in_flight_fences[i]) != VK_SUCCESS)
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
	init.disp.destroyCommandPool(data.command_pool, nullptr);

	for (auto framebuffer : data.framebuffers)
	{
		init.disp.destroyFramebuffer(framebuffer, nullptr);
	}

	init.swapchain.destroy_image_views(data.swapchain_image_views);

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

	init.disp.waitForFences(1, &data.in_flight_fences[data.current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index = 0;
	VkResult result = init.disp.acquireNextImageKHR(
		init.swapchain, UINT64_MAX, data.available_semaphores[data.current_frame], VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return RecreateSwapchain(init, data);
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		SendErrorPopup("failed to acquire swapchain image. Error " + result);
		return false;
	}

	if (data.image_in_flight[image_index] != VK_NULL_HANDLE)
	{
		init.disp.waitForFences(1, &data.image_in_flight[image_index], VK_TRUE, UINT64_MAX);
	}
	data.image_in_flight[image_index] = data.in_flight_fences[data.current_frame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { data.available_semaphores[data.current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = wait_semaphores;
	submitInfo.pWaitDstStageMask = wait_stages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &data.command_buffers[image_index];

	VkSemaphore signal_semaphores[] = { data.finished_semaphore[image_index] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signal_semaphores;

	init.disp.resetFences(1, &data.in_flight_fences[data.current_frame]);

	if (init.disp.queueSubmit(data.graphics_queue, 1, &submitInfo, data.in_flight_fences[data.current_frame]) != VK_SUCCESS)
	{
		SendErrorPopup("failed to submit draw command buffer");
		return false;
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapChains[] = { init.swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapChains;

	present_info.pImageIndices = &image_index;

	result = init.disp.queuePresentKHR(data.present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized)
	{
		return RecreateSwapchain(init, data);
	}
	else if (result != VK_SUCCESS)
	{
		SendErrorPopup("failed to present swapchain image");
		return false;
	}

	data.current_frame = (data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}

void RenderThread::Cleanup(AppData &init, RenderData &data)
{
	for (size_t i = 0; i < init.swapchain.image_count; i++)
	{
		init.disp.destroySemaphore(data.finished_semaphore[i], nullptr);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		init.disp.destroySemaphore(data.available_semaphores[i], nullptr);
		init.disp.destroyFence(data.in_flight_fences[i], nullptr);
	}

	init.disp.destroyCommandPool(data.command_pool, nullptr);

	for (auto framebuffer : data.framebuffers)
	{
		init.disp.destroyFramebuffer(framebuffer, nullptr);
	}

	init.disp.destroyPipeline(data.graphics_pipeline, nullptr);
	init.disp.destroyPipelineLayout(data.pipeline_layout, nullptr);
	init.disp.destroyRenderPass(data.render_pass, nullptr);

	init.swapchain.destroy_image_views(data.swapchain_image_views);

	vkb::destroy_swapchain(init.swapchain);
	vkb::destroy_device(init.device);
	vkb::destroy_surface(init.instance, init.surface);
	vkb::destroy_instance(init.instance);
}
