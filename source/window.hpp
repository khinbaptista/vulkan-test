#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <string>
#include <array>

class Window {
protected:
	GLFWwindow*	_window;
	std::string	_title;
	uint32_t	_width;
	uint32_t	_height;

public:
	Window(
		std::string title,
		uint32_t width = 1280,
		uint32_t height = 720
	);

	~Window();

	void Initialize();

	std::string title();
	uint32_t width();
	uint32_t height();
	GLFWwindow* glfw();

	VkResult CreateSurface(const vk::Instance&, vk::SurfaceKHR*);

	bool should_close();
};
