#pragma once

#include <vulkan/vulkan.hpp>
#include "swapchain.hpp"

class Viewport {
protected:
	Swapchain _swapchain;
	std::vector<vk::ImageView> _views;

	uint32_t _width;
	uint32_t _height;

	vk::Viewport _viewport;

public:
	~Viewport();
	Viewport();
	Viewport(
		const vk::PhysicalDevice&, const vk::SurfaceKHR&,
		uint32_t width, uint32_t height
	);

	vk::Rect2D scissor;

	Swapchain& swapchain();
	const std::vector<vk::ImageView>& views() const;

	vk::Viewport& vk();

	uint32_t width() const;
	uint32_t height() const;
	void width(uint32_t);
	void height(uint32_t);

	void DestroySwapchain();
	void DestroyImageViews();
};
