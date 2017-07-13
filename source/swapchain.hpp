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

public:
	Swapchain();
	~Swapchain();
	Swapchain(const vk::PhysicalDevice&, const vk::SurfaceKHR&);

	vk::SwapchainKHR vk();

	static SwapchainSupportDetails QuerySupport(vk::PhysicalDevice, vk::SurfaceKHR);
};
