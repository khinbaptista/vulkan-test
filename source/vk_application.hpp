#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
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

class VulkanApp {
public:
	void Run();

private:
	GLFWwindow* window;
	VDeleter<VkInstance> instance { vkDestroyInstance };

	VDeleter<VkDebugReportCallbackEXT> callback {
		instance, DestroyDebugReportCallbackEXT
	};

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	void InitWindow();
	void InitVulkan();
	void CreateInstance();
	bool CheckValidationLayerSupport();
	void CheckExtensionsSupport();
	void SetupDebugCallback();

	std::vector<const char*> GetRequiredExtensions();

	void PickPhysicalDevice();
	bool _isDeviceSuitable(VkPhysicalDevice);

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
