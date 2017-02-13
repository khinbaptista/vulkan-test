#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <string>

class VkApp {
public:
	std::vector<const char*> validationLayers;

	VkApp(
		std::string title = "VkApp",
		int width = 800, int height = 600,
		bool enable_validation_layers = false
	);

	void Run();

protected:
	// ##############################
	// Window handle and variables

	GLFWwindow* window;
	std::string title;
	int width;
	int height;

	bool validation_enabled;


	void InitWindow();
	void MainLoop();
	void Cleanup();

	// ##############################
	// Vulkan stuff

	vk::Instance instance;
	VkDebugReportCallbackEXT callback;

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
		void* userData
	);
};
