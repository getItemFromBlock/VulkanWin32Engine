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

#include "GameThread.hpp"

const u32 MAX_FRAMES_IN_FLIGHT = 2;

struct UBO
{
	Maths::Vec2 invRes;
	Maths::Vec2 scale;
};

struct AppData
{
	HWND hWnd;
	HINSTANCE hInstance;
	GameThread *gm;
	vkb::Instance instance;
	vkb::InstanceDispatchTable instDisp;
	VkSurfaceKHR surface;
	vkb::Device device;
	vkb::DispatchTable disp;
	vkb::Swapchain swapchain;
	f32 maxSamplerAnisotropy = 0;
};

struct RenderData
{
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> framebuffers;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	VkCommandPool transfertCommandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandBuffer transferCommandBuffer;

	std::vector<VkSemaphore> availableSemaphores;
	std::vector<VkSemaphore> finishedSemaphore;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imageInFlight;
	
	std::vector<VkBuffer> objectBuffers;
	std::vector<VkDeviceMemory> objectBuffersMemory;
	std::vector<Maths::Vec4*> objectBuffersMapped;

	std::vector<VkBuffer> computeBuffers;
	std::vector<VkDeviceMemory> computeBuffersMemory;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	u32 mainBufSize = 0;
	u32 sizeObjects = 0;
	u32 sizeSortBuf = 0;
	u32 sizeMergeBuf = 0;
	u32 currentFrame = 0;
};

struct SceneData
{
	Resource::Mesh mesh;
};

class RenderThread
{
public:
	RenderThread() = default;
	~RenderThread() = default;

	void Init(HWND hwnd, HINSTANCE hInstance, GameThread *gm, Maths::IVec2 res, u32 targetDevice = 0);
	void Resize(s32 x, s32 y);
	bool HasFinished() const;
	bool HasCrashed() const;
	void Quit();

private:
	std::thread thread;
	std::chrono::system_clock::duration start = std::chrono::system_clock::duration();
	std::atomic_bool exit;
	std::atomic_bool crashed;
	std::atomic_bool resized;
	AppData appData = {};
	RenderData renderData = {};
	SceneData sceneData = {};
	Maths::IVec2 res;
	Maths::IVec2 swapRes;
	u64 lastRes = 0;
	std::atomic<u64> storedRes;
	Maths::Vec2 storedDelta;
	Maths::Vec3 position = Maths::Vec3(-5.30251f, 6.38824f, -7.8891f);
	Maths::Vec2 rotation = Maths::Vec2(static_cast<f32>(M_PI_2) - 1.059891f, 0.584459f);
	f32 fov = 3.55f;
	f64 appTime = 0;

	void ThreadFunc(u32 targetDevice);
	void HandleResize();
	void InitThread();
	void LoadAssets();
	void UnloadAssets();

	VkSurfaceKHR CreateSurfaceWin32(VkInstance instance, HINSTANCE hInstance, HWND window, VkAllocationCallbacks *allocator = nullptr);
	VkShaderModule CreateShaderModule(const std::string &code);
	bool CreateImage(Maths::IVec2 res, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &memory);
	VkVertexInputBindingDescription GetBindingDescription();
	std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();
	bool InitVulkan(u32 targetDevice);
	bool InitDevice(u32 targetDevice);
	bool CreateSwapchain();
	bool GetQueues();
	bool CreateRenderPass();
	bool CreateDescriptorSetLayout();
	bool CreateGraphicsPipeline();
	bool CreateFramebuffers();
	bool CreateCommandPool();
	bool CreateTextureImage();
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	bool CreateTextureImageView();
	bool CreateTextureSampler();
	bool CreateDepthResources();
	bool CreateVertexBuffer(const Resource::Mesh &m);
	bool CreateObjectBuffers(u32 objectCount);
	bool CreateCommandBuffers();
	bool CreateSyncObjects();
    bool CreateDescriptorPool();
	bool CreateDescriptorSets();
	bool RecreateSwapchain();
	VkCommandBuffer BeginSingleTimeCommands(VkCommandPool targetPool);
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool targetPool, VkQueue targetQueue);
	bool TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	bool UpdateUniformBuffer(u32 image);
	u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	bool CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	bool DrawFrame();
	void Cleanup();
};