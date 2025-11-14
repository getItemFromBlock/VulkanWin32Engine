#include "RenderThread.hpp"

#include <filesystem>
#include <time.h>
#include <assert.h>

#include "VkBootstrap.h"

using namespace Maths;

const u8 MOVEMENT_KEYS[6] =
{
	'A', 'E', 'W', 'D', 'Q', 'S'
};

struct Init
{
    HWND window;
    vkb::Instance instance;
    vkb::InstanceDispatchTable inst_disp;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::DispatchTable disp;
    vkb::Swapchain swapchain;
};

struct RenderData
{
    VkQueue graphics_queue;
    VkQueue present_queue;

    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> available_semaphores;
    std::vector<VkSemaphore> finished_semaphore;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> image_in_flight;
    size_t current_frame = 0;
};

VkSurfaceKHR create_surface_win32(VkInstance instance, HINSTANCE hInstance, HWND window, VkAllocationCallbacks* allocator = nullptr) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
	
	VkWin32SurfaceCreateInfoKHR winSurfInfo = {};
	winSurfInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	winSurfInfo.hwnd = window;
	winSurfInfo.hinstance = hInstance;

    VkResult err = vkCreateWin32SurfaceKHR(instance, &winSurfInfo, allocator, &surface);
    if (err)
	{
		IErrorInfo *e;
        int ret = GetErrorInfo(0, &e);
        if (ret != 0)
		{
			std::wstring text = L"Could not create surface!\n";
			BSTR s = NULL;

			if (e && SUCCEEDED(e->GetDescription(&s)))
				text += L"Unknown error";
			else
				text += s;
			MessageBoxW(NULL, text.c_str(), L"Error!", NULL);
        }
        surface = VK_NULL_HANDLE;
    }
    return surface;
}

std::string GetTime()
{
	time_t timeObj;
	time(&timeObj);
	tm pTime = {};
	gmtime_s(&pTime, &timeObj);
	char buffer[256];
	sprintf_s(buffer, 255, "%d-%d-%d_%d-%d-%d", pTime.tm_year+1900, pTime.tm_mon+1, pTime.tm_mday, pTime.tm_hour, pTime.tm_min, pTime.tm_sec);
	return std::string(buffer);
}

void RenderThread::Init(HWND hwnIn, IVec2 resIn)
{
	hwnd = hwnIn;
	res = resIn;
	thread = std::thread(&RenderThread::ThreadFunc, this);
}

void RenderThread::Resize(IVec2 newRes)
{
	while (resize)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	storedRes = IVec2(Util::MaxI(newRes.x, 32), Util::MaxI(newRes.y, 32));
	resize = true;
}

bool RenderThread::HasFinished() const
{
	return exit;
}

void RenderThread::Quit()
{
	exit = true;
	thread.join();
}

f32 RenderThread::GetElapsedTime()
{
	return elapsedTime;
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

void RenderThread::HandleResize()
{
	if (resize)
	{
		if (colorBuffer.size() < static_cast<u64>(storedRes.x) * storedRes.y)
		{
			colorBuffer.resize(static_cast<u64>(storedRes.x) * storedRes.y);
		}
		res = storedRes;
		//kernels.Resize(res);
		resize = false;
	}
}

void RenderThread::InitThread()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	start = now.time_since_epoch();
	//kernels.InitKernels(res, threadID);
	colorBuffer.resize(static_cast<u64>(res.x) * res.y);
}

void RenderThread::ThreadFunc()
{
	InitThread();
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
		bool screenchot = keyPress.test(VK_F2);
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
		//kernels.RenderMeshes(colorBuffer.data(), static_cast<u32>(meshes.size()), position, q * Vec3(0,0,1), q * Vec3(0,1,0), fov, 1, denoiseStrength, params);
		CopyToScreen((s64)(deltaTime*1000));
		if (screenchot)
		{
			if (!std::filesystem::exists("Screenshots"))
			{
				std::filesystem::create_directory("Screenshots");
			}
			std::string name = "Screenshots/";
			name += GetTime();
			//CudaUtil::SaveFrameBuffer(kernels.GetMainFrameBuffer(), name);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	UnloadAssets();
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

void RenderThread::CreateVKInstance()
{
	VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Demo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Ligma Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;

	/*
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	*/
	
}
