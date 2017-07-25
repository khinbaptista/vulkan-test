#include "viewport.hpp"
#include "application.hpp"

Viewport::Viewport() { }

Viewport::Viewport(
	const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface,
	uint32_t width, uint32_t height
)
:	_width(width)
,	_height(height)
,	swapchain(device, surface, width, height)
{
	size_t image_count = swapchain.images().size();
	_views.resize(image_count);

	for (size_t i = 0; i < image_count; i++) {
		auto view_info = vk::ImageViewCreateInfo()
		.setImage(swapchain.images()[i])
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(swapchain.format())
		.setComponents({
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		}).setSubresourceRange(
			vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
			/*
			If we were creating a stereoscopic 3D application, we'd create a
			swapchain with multiple layers, and create multiple image views
			for each image representing the views of the left and right eyes
			by accessing different layers.
			*/
		);

		_views[i] = Application::get_device().createImageView(view_info);
	}

	/*
	An image view is sufficient to be used as a texture, but not to be used as
	a render target (still needs a framebuffer)
	*/
}

Viewport::~Viewport() {
	vk::Device device = Application::get_device();
	for (size_t i = 0; i < _views.size(); i++) {
		device.destroyImageView(_view[i]);
	}
}
