#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

struct SwapchainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> present_modes;
};

class Swapchain {
protected:
	vk::SwapchainKHR vk_swapchain;

	vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>&);
	vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>&);
	vk::Extent2D ChooseExtent(const vk::SurfaceCapabilitiesKHR&);

	uint32_t _width;
	uint32_t _height;

public:
	~Swapchain();
	Swapchain(uint32_t width = 0, uint32_t height = 0);
	Swapchain(
		const vk::PhysicalDevice&,
		const vk::SurfaceKHR&,
		uint32_t width = 0, uint32_t height = 0
	);

	vk::SwapchainKHR vk();

	uint32_t width();
	uint32_t height();
	void width(uint32_t);
	void height(uint32_t);

	void Create(const vk::PhysicalDevice&, const vk::SurfaceKHR&);

	static SwapchainSupportDetails QuerySupport(vk::PhysicalDevice, vk::SurfaceKHR);
};
