#pragma once

#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "vkref.hpp"
#include "window.hpp"

class Application {
public:
	Application(
		std::string title = "",
		bool enable_validation_layers = false
	);

	~Application();

	void Run();

protected:
	const std::vector<const char*> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	Window *window;

	vkref<vk::Instance> instance { vkDestroyInstance };

	void InitializeVulkan();
	void MainLoop();

	void CreateVulkanInstance();
};
