#pragma once

#include <Windows.h>

#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <bitset>
#include <atomic>
#include <array>

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan.h"
#include "VkBootstrap.h"

#include "Types.hpp"
#include "Maths/Maths.hpp"
#include "Resource/Mesh.hpp"

enum LaunchParams : u8
{
	NONE =			0x0,
	ADVANCED =		0x1,
	INVERTED_RB =	0x2,
	BOXDEBUG =		0x4,
	DENOISE =		0x8,
	CLEAR =			0x10,
};

struct AppData
{
	HWND hWnd;
	HINSTANCE hInstance;
	vkb::Instance instance;
	vkb::InstanceDispatchTable instDisp;
	VkSurfaceKHR surface;
	vkb::Device device;
	vkb::DispatchTable disp;
	vkb::Swapchain swapchain;
};

struct RenderData
{
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> framebuffers;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> availableSemaphores;
	std::vector<VkSemaphore> finishedSemaphore;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageInFlight;
	size_t currentFrame = 0;
};

struct SceneData
{
	Resource::Mesh mesh;
};

class RenderThread
{
public:
	RenderThread() {};
	~RenderThread() {};

	void Init(HWND hwnd, HINSTANCE hInstance, Maths::IVec2 res);
	void Resize(s32 x, s32 y);
	bool HasFinished() const;
	bool HasCrashed() const;
	void Quit();
	void MoveMouse(Maths::Vec2 delta);
	void SetKeyState(u8 key, bool state);

private:
	std::thread thread;
	std::chrono::system_clock::duration start = std::chrono::system_clock::duration();
	std::atomic_bool exit;
	std::atomic_bool crashed;
	std::atomic_bool queueLock;
	std::atomic_bool resized;
	std::mutex mouseLock;
	std::mutex keyLock;
	std::bitset<256> keyDown = 0;
	std::bitset<256> keyPress = 0;
	std::bitset<256> keyToggle = 0;
	AppData appData = {};
	RenderData renderData = {};
	SceneData sceneData = {};
	Maths::IVec2 res;
	u64 lastRes = 0;
	std::atomic<u64> storedRes;
	Maths::Vec2 storedDelta;
	Maths::Vec3 position = Maths::Vec3(-5.30251f, 6.38824f, -7.8891f);
	Maths::Vec2 rotation = Maths::Vec2(static_cast<f32>(M_PI_2) - 1.059891f, 0.584459f);
	f32 fov = 3.55f;

	void ThreadFunc();
	void HandleResize();
	void InitThread();
	void LoadAssets();
	void UnloadAssets();

	VkSurfaceKHR CreateSurfaceWin32(VkInstance instance, HINSTANCE hInstance, HWND window, VkAllocationCallbacks *allocator = nullptr);
	VkShaderModule CreateShaderModule(AppData &init, const std::string &code);
	VkVertexInputBindingDescription GetBindingDescription();
	std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
	bool InitVulkan();
	bool InitDevice(AppData &init);
	bool CreateSwapchain(AppData &init);
	bool GetQueues(AppData &init, RenderData &data);
	bool CreateRenderPass(AppData &init, RenderData &data);
	bool CreateGraphicsPipeline(AppData &init, RenderData &data);
	bool CreateFramebuffers(AppData &init, RenderData &data);
	bool CreateCommandPool(AppData &init, RenderData &data);
	bool CreateCommandBuffers(AppData &init, RenderData &data);
	bool CreateSyncObjects(AppData &init, RenderData &data);
	bool RecreateSwapchain(AppData &init, RenderData &data);
	bool DrawFrame(AppData &init, RenderData &data);
	void Cleanup(AppData &init, RenderData &data);
	void SendErrorPopup(const std::wstring &err);
	void SendErrorPopup(const std::string &err);
	void LogMessage(const std::wstring& msg);
	void LogMessage(const std::string& msg);
};