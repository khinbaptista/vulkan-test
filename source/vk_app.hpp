#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

struct QueueFamilyIndices {
	int graphics_family = -1;
	int present_family  = -1;

	bool isComplete();
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;

	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> present_modes;
};

class VkApp {
public:
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;

	VkApp(
		std::string title = "VkApp",
		uint32_t width = 800, uint32_t height = 600,
		bool enable_validation_layers = false
	);

	void Run();
	static void OnWindowResized(GLFWwindow*, int width, int height);

protected:
	// ##############################
	// Window handle and variables

	GLFWwindow* window;
	std::string title;
	uint32_t width;
	uint32_t height;

	bool validation_enabled;


	void InitWindow();
	void MainLoop();
	void DrawFrame();
	void Cleanup();

	// ##############################
	// Vulkan stuff

	vk::Instance 			instance;
	VkDebugReportCallbackEXT	callback;

	vk::PhysicalDevice		physical_device;
	vk::Device			device;
	vk::Queue			graphics_queue;
	vk::Queue			presentation_queue;

	vk::SurfaceKHR			surface;
	vk::SwapchainKHR		swapchain;
	vk::Format			swapchain_format;
	vk::Extent2D			swapchain_extent;
	std::vector<vk::Image>		swapchain_images;
	std::vector<vk::ImageView>	swapchain_imageviews;
	std::vector<vk::Framebuffer>	swapchain_framebuffers;

	vk::PipelineLayout		pipeline_layout;
	vk::RenderPass			render_pass;
	vk::Pipeline			graphics_pipeline;

	vk::CommandPool			command_pool;
	std::vector<vk::CommandBuffer>	command_buffers;

	vk::Semaphore			semaphore_image_available;
	vk::Semaphore			semaphore_render_finished;

	void InitVulkan();

	void CreateInstance();
	std::vector<const char*> GetRequiredExtensions();
	bool CheckExtensionsSupport(std::vector<const char*>);
	bool CheckValidationLayerSupport();
	void SetupDebugCallback();
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);

	void CreateSurface();

	void PickPhysicalDevice();
	bool isDeviceSuitable(vk::PhysicalDevice);
	QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice);

	void CreateLogicalDevice();
	bool CheckDeviceExtensionSupport(vk::PhysicalDevice);

	void CreateSwapchain();
	void RecreateSwapchain();
	SwapChainSupportDetails QuerySwapchainSupport(vk::PhysicalDevice);
	vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
		const std::vector<vk::SurfaceFormatKHR>& available_formats);
	vk::PresentModeKHR ChooseSwapPresentMode(
		const std::vector<vk::PresentModeKHR>& available_modes);
	vk::Extent2D ChooseSwapExtent(
		const vk::SurfaceCapabilitiesKHR& capabilities);

	void CreateImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();

	static std::vector<char> ReadFile(const std::string& filename);
	void CreateShaderModule(const std::vector<char>& code, vk::ShaderModule&);

	void CreateCommandPool();
	void CreateCommandBuffers();

	void CreateSemaphores();
};
