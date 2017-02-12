#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <functional>
#include "vdeleter.hpp"

const int WIDTH = 800;
const int HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	bool isComplete();
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanApp {
public:
	void Run();

private:
	GLFWwindow* window;
	VDeleter<VkInstance> instance { vkDestroyInstance };
	//VDeleter<vk::Instance> instance { &vk::Instance::destroy };

	VDeleter<VkDebugReportCallbackEXT> callback {
		instance, DestroyDebugReportCallbackEXT
	};

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VDeleter<VkDevice> device { vkDestroyDevice };
	VDeleter<VkSurfaceKHR> surface { instance, vkDestroySurfaceKHR };
	VDeleter<VkSwapchainKHR> swapChain{ device, vkDestroySwapchainKHR };

	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	void InitWindow();
	void InitVulkan();
	void CreateInstance();
	void CreateSurface();
	bool CheckValidationLayerSupport();
	void CheckExtensionsSupport();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice);
	void SetupDebugCallback();
	void CreateLogicalDevice();
	void CreateSwapChain();

	std::vector<const char*> GetRequiredExtensions();

	void PickPhysicalDevice();
	bool _isDeviceSuitable(VkPhysicalDevice);
	QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice);
	SwapChainSupportDetails _querySwapChainSupport(VkPhysicalDevice);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>&);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR&);

	void MainLoop();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData
	);
};
