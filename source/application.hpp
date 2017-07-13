#pragma once

#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "vkref.hpp"
#include "window.hpp"
#include "swapchain.hpp"

struct QueueFamilyIndices {
	int graphics	= -1;
	int present		= -1;

	bool is_complete();
};

class Application {
public:
	Application(
		std::string title = "",
		bool enable_validation_layers = false
	);

	~Application();

	void Run();

	// Singleton access
	static Application*	get_singleton();
	vk::Device			get_device();
	Window*				get_window();

	QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice);

protected:
	static Application* singleton;

	bool enable_validation_layers;
	const std::vector<const char*> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();

	void SetupDebugCallback();
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT,
		uint64_t obj, size_t location, int32_t code,
		const char* layerPrefix, const char* msg, void* userData
	);

	const std::vector<const char*> device_extensions = {
		"VK_KHR_swapchain"
	};
	bool CheckDeviceExtensions(vk::PhysicalDevice);

	Window *window;

	vk::Instance	instance;
	vk::SurfaceKHR	surface;

	vk::DebugReportCallbackEXT callback;
	vk::PhysicalDevice physical_device;

	vk::Device	device;
	vk::Queue	graphics_queue;
	vk::Queue	present_queue;

	void InitializeVulkan();
	void MainLoop();

	bool is_device_suitable(vk::PhysicalDevice);

	void CreateVulkanInstance();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSurface();

	Swapchain swapchain;
};
