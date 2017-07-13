#include "swapchain.hpp"
#include "application.hpp"

using std::vector;

Swapchain::Swapchain() {}

Swapchain::~Swapchain() {
	//Application::get_singleton()->get_device().destroySwapchainKHR(vk_swapchain);
}

Swapchain::Swapchain(
	const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface
) {
	SwapchainSupportDetails	support			= QuerySupport(device, surface);
	vk::SurfaceFormatKHR	format			= ChooseSurfaceFormat(support.formats);
	vk::PresentModeKHR		present_mode	= ChoosePresentMode(support.present_modes);
	vk::Extent2D 			extent			= ChooseExtent(support.capabilities);

	// Minimum value + 1 for triple buffering
	uint32_t image_count = support.capabilities.minImageCount + 1;
	if (	// A value of zero means no bounds (besides memory requirements)
		support.capabilities.maxImageCount > 0 &&
		image_count > support.capabilities.maxImageCount
	) {		// Clamp to max value
		image_count = support.capabilities.maxImageCount;
	}

	auto swapchain_info = vk::SwapchainCreateInfoKHR()
	.setSurface(surface)
	.setMinImageCount(image_count)
	.setImageFormat(format.format)
	.setImageColorSpace(format.colorSpace)
	.setImageExtent(extent)
	.setImageArrayLayers(1)	// always 1 unless it's a stereoscopic 3D application
	.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment); // Will render directly into the images; if post-processing, this is different

	QueueFamilyIndices indices = Application::get_singleton()->FindQueueFamilies(device);
	uint32_t queue_family_indices[] =
		{ (uint32_t) indices.graphics, (uint32_t) indices.present };

	if (indices.graphics != indices.present) {
		swapchain_info.setImageSharingMode(vk::SharingMode::eConcurrent)
		.setQueueFamilyIndexCount(2)
		.setPQueueFamilyIndices(queue_family_indices);
	} else {
		/*	'Exclusive' means the images can only belong to one queue at a time,
			and ownership transfer operations must be explicit. This translates
			to better performance.	*/

		swapchain_info.setImageSharingMode(vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(0)		// Optisonal
		.setPQueueFamilyIndices(nullptr);	// Optional
	}

	swapchain_info
	.setPreTransform(support.capabilities.currentTransform)
	.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
	.setPresentMode(present_mode)
	.setClipped(true)
	.setOldSwapchain(nullptr);

	vk_swapchain = Application::get_singleton()->get_device().createSwapchainKHR(swapchain_info);
}

SwapchainSupportDetails Swapchain::QuerySupport(
	vk::PhysicalDevice device, vk::SurfaceKHR surface
) {
	SwapchainSupportDetails details;
	details.capabilities	= device.getSurfaceCapabilitiesKHR(surface);
	details.formats			= device.getSurfaceFormatsKHR(surface);
	details.present_modes	= device.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(
	const vector<vk::SurfaceFormatKHR>& available_formats
) {
	if (
		available_formats.size() == 1 &&
		available_formats[0].format == vk::Format::eUndefined
	) {
		return { vk::Format::eB8G8R8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& format : available_formats) {
		if (
			format.format == vk::Format::eB8G8R8Unorm &&
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
		) {
			return format;
		}
	}

	return available_formats[0];
}

vk::PresentModeKHR Swapchain::ChoosePresentMode(
	const std::vector<vk::PresentModeKHR>& available_modes
) {
	// FIFO is guaranteed to be available, but may be buggy
	vk::PresentModeKHR best_available = vk::PresentModeKHR::eFifo;

	for (const auto& present_mode : available_modes) {
		if (present_mode == vk::PresentModeKHR::eMailbox) {
			return present_mode;
		} else if (present_mode == vk::PresentModeKHR::eImmediate) {
			best_available = present_mode;
		}
	}

	return best_available;
}

vk::Extent2D Swapchain::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	Window* window = Application::get_singleton()->get_window();
	vk::Extent2D extent = { window->width(), window->height() };

	// Clamp the values to the max allowed by the surface capabilities
	extent.width = std::max(
		capabilities.minImageExtent.width,
		std::min(capabilities.maxImageExtent.width, extent.width)
	);
	extent.height = std::max(
		capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, extent.height)
	);

	return extent;
}

vk::SwapchainKHR Swapchain::vk() { return vk_swapchain; }
