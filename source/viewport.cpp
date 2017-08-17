#include "viewport.hpp"
#include "application.hpp"

Viewport::~Viewport() { }

Viewport::Viewport() { }

Viewport::Viewport(
	const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface,
	uint32_t width, uint32_t height
)
:	_width(width)
,	_height(height)
,	_swapchain(device, surface, width, height)
{
	size_t image_count = _swapchain.images().size();
	_views.resize(image_count);

	for (size_t i = 0; i < image_count; i++) {
		auto view_info = vk::ImageViewCreateInfo()
		.setImage(_swapchain.images()[i])
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(_swapchain.format())
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
			.setLayerCount(1)
		);
		/*
			If we were creating a stereoscopic 3D application, we'd create a
			swapchain with multiple layers, and create multiple image views
			for each image representing the views of the left and right eyes
			by accessing different layers.
		*/

		_views[i] = Application::get_device().createImageView(view_info);
		/*
			An image view is sufficient to be used as a texture, but not to be
			used as a render target (still needs a framebuffer)
		*/
	}

	_viewport.x = 0.0f;
	_viewport.y = 0.0f;
	_viewport.width  = (float)_swapchain.extent().width;
	_viewport.height = (float)_swapchain.extent().height;
	_viewport.minDepth = 0.0f;
	_viewport.maxDepth = 1.0f;

	scissor.offset = vk::Offset2D(0, 0);
	scissor.extent = _swapchain.extent();
}

vk::Viewport& Viewport::vk() { return _viewport; }

void Viewport::DestroySwapchain() {
	Application::get_device().destroySwapchainKHR(_swapchain.vk());
}

void Viewport::DestroyImageViews() {
	vk::Device device = Application::get_device();
	for (size_t i = 0; i < _views.size(); i++) {
		device.destroyImageView(_views[i]);
	}
}

Swapchain& Viewport::swapchain() { return _swapchain; }
uint32_t Viewport::width() const { return _width; }
uint32_t Viewport::height() const { return _height; }
