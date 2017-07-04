#include "application.hpp"

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <set>
#include <algorithm>

using std::cout;
using std::cerr;
using std::endl;

using std::runtime_error;

using std::string;
using std::vector;
using std::set;

Application::Application(string title, bool validate) {
	window = new Window(title);
	enable_validation_layers = validate;
}

Application::~Application() {
	{	// Delete debug report callback object
		auto destroy_callback_func = (PFN_vkDestroyDebugReportCallbackEXT)
			instance.getProcAddr("vkDestroyDebugReportCallbackEXT");

		if (destroy_callback_func != nullptr) {
			destroy_callback_func(instance, callback, nullptr);
		}
	}

	// Destroy logical device
	device.destroy();

	// Destroy window surface and vulkan instance
	instance.destroySurfaceKHR(surface);
	instance.destroy();

	// Destroy window and terminate GLFW
	delete window;
	glfwTerminate();
}

void Application::Run() {
	glfwInit();

	window->Initialize();
	InitializeVulkan();

	MainLoop();
}

void Application::InitializeVulkan() {
	CreateVulkanInstance();
	CreateSurface();
	SetupDebugCallback();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapchain();
}

void Application::MainLoop() {
	while (!window->should_close()) {
		glfwPollEvents();
	}
}

void Application::CreateVulkanInstance() {
	if (enable_validation_layers && !CheckValidationLayerSupport()) {
		throw runtime_error("Validation layers not available");
	}

	auto app_info = vk::ApplicationInfo()
	.setPApplicationName(window->title().c_str())
	.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
	.setPEngineName("Godot Engine")
	.setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
	.setApiVersion(VK_API_VERSION_1_0);

	auto extensions = GetRequiredExtensions();
	auto instance_info = vk::InstanceCreateInfo()
	.setPApplicationInfo(&app_info)
	.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
	.setPpEnabledExtensionNames(extensions.data());

	if (enable_validation_layers) {
		instance_info.setEnabledLayerCount(static_cast<uint32_t>(validation_layers.size()));
		instance_info.setPpEnabledLayerNames(validation_layers.data());
	} else {
		instance_info.setEnabledLayerCount(0);
	}

	instance = vk::createInstance(instance_info);
}

bool Application::CheckValidationLayerSupport() {
	vector<vk::LayerProperties> available_layers;
	available_layers = vk::enumerateInstanceLayerProperties();

	for (const char* layer_name : validation_layers) {
		bool layer_found = false;

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) return false;
	}

	return true;
}

vector<const char*> Application::GetRequiredExtensions() {
	vector<const char*> extensions;

	unsigned int glfw_extensions_count = 0;
	const char** glfw_extensions = nullptr;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

	for (unsigned int i = 0; i < glfw_extensions_count; i++) {
		extensions.push_back(glfw_extensions[i]);
	}

	if (enable_validation_layers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

void Application::CreateSurface() {
	//if (glfwCreateWindowSurface(instance, window->glfw(), nullptr, &surface) != VK_SUCCESS) {
	if (window->CreateSurface(instance, &surface) != VK_SUCCESS) {
		throw runtime_error("Failed to create window surface");
	}
}

VkBool32 Application::DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData
) {
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}

void Application::SetupDebugCallback() {
	if (!enable_validation_layers) return;

	auto callback_info = vk::DebugReportCallbackCreateInfoEXT()
	.setFlags(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning)
	.setPfnCallback(DebugCallback);

	auto func = (PFN_vkCreateDebugReportCallbackEXT)
		instance.getProcAddr("vkCreateDebugReportCallbackEXT");

	VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
	if (func != nullptr) {
		result = func(
			instance,
			(VkDebugReportCallbackCreateInfoEXT*) &callback_info,
			nullptr,
			(VkDebugReportCallbackEXT*) &callback
		);
	}

	if (result != VK_SUCCESS) {
		throw runtime_error("Failed to setup debug callback");
	}
}

bool Application::is_device_suitable(vk::PhysicalDevice device) {
	QueueFamilyIndices indices	= FindQueueFamilies(device);
	bool extensions_supported	= CheckDeviceExtensions(device)

	bool swapchain_adequate = false;
	if (extensions_supported) {
		SwapchainSupportDetails swapchain_support = QuerySwapchainSupport(device);
		swapchain_adequate =
			!swapchain_support.formats.empty() &&
			!swapchain_support.present_modes.empty();
	}

	return indices.is_complete() && extensions_supported && swapchain_adequate;
}

bool Application::CheckDeviceExtensions(vk::PhysicalDevice device) {
	vector<vk::ExtensionProperties> available_extensions;
	available_extensions = device.enumerateDeviceExtensionProperties();

	set<string> required_extensions(
		device_extensions.begin(). device_extensions.end()
	);

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

void Application::PickPhysicalDevice() {
	vector<vk::PhysicalDevice> devices;
	devices = instance.enumeratePhysicalDevices();

	if (devices.size() == 0) {
		throw runtime_error("Filed to find GPUs with Vulkan support");
	}

	for (const auto& device : devices) {
		if (is_device_suitable(device)) {
			physical_device = device;
			break;
		}
	}

	if (!physical_device) {
		throw runtime_error("Failed to find a suitable GPU");
	}
}

bool QueueFamilyIndices::is_complete() {
	return graphics >= 0 && present >= 0;
}

QueueFamilyIndices Application::FindQueueFamilies(vk::PhysicalDevice device) {
	QueueFamilyIndices indices;
	vector<vk::QueueFamilyProperties> families;
	families = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& family : families) {
		if (
			family.queueCount > 0 &&
			family.queueFlags & vk::QueueFlagBits::eGraphics
		) {
			indices.graphics = i;
		}

		if (
			device.getSurfaceSupportKHR(i, surface) &&
			family.queueCount > 0
		) {
			indices.present = i;
		}

		if (indices.is_complete()) { break; }

		i++;
	}

	return indices;
}

SwapchainSupportDetails Application::QuerySwapchainSupport(vk::PhysicalDevice device) {
	SwapchainSupportDetails details;
	details.capabilities	= device.getSurfaceCapabilitiesKHR(surface);
	details.formats			= device.getSurfaceFormatsKHR(surface);
	details.present_modes	= device.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR Application::ChooseSwapchainSurfaceFormat(
	const vector<vk::SurfaceFormatKHR>& available_formats
) {
	if (
		available_formats.size() == 1 &&
		available_formats[0].format == vk::Format::eUndefined
	) {
		return { vk::Format::eB8G8R8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear }
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

vk::PresentModeKHR Application::ChooseSwapchainPresentMode(
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

vk::Extent2D Application::ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	vk::Extent2D extent = { window.width(), window.height() };

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

void Application::CreateLogicalDevice() {
	QueueFamilyIndices indices = FindQueueFamilies(physical_device);

	vector<vk::DeviceQueueCreateInfo> queue_infos;
	set<int> unique_queue_families = {
		indices.graphics, indices.present
	};

	float queue_priority = 1.0f;
	for (int family : unique_queue_families) {
		auto queue_info = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(family)
		.setQueueCount(1)
		.setPQueuePriorities(&queue_priority);
		queue_infos.push_back(queue_info);
	}

	auto device_features = vk::PhysicalDeviceFeatures();	// TODO

	auto device_info = vk::DeviceCreateInfo()
	.setQueueCreateInfoCount(static_cast<uint32_t>(queue_infos.size()))
	.setPQueueCreateInfos(queue_infos.data())
	.setPEnabledFeatures(&device_features)
	.setEnabledExtensionCount(static_cast<uint32_t>(device_extensions.size()))
	.setPpEnabledExtensionNames(device_extensions.data());

	if (enable_validation_layers) {
		device_info.setEnabledLayerCount(static_cast<uint32_t>(validation_layers.size()));
		device_info.setPpEnabledLayerNames(validation_layers.data());
	} else {
		device_info.setEnabledLayerCount(0);
	}

	device			= physical_device.createDevice(device_info);
	graphics_queue	= device.getQueue(indices.graphics, 0);
	present_queue	= device.getQueue(indices.present, 0);
}

void Application::CreateSwapchain() {
	SwapchainSupportDetails support = QuerySwapchainSupport(physical_device);

	vk::SurfaceFormatKHR format = ChooseSwapchainSurfaceFormat(support.formats);
	vk::PresentModeKHR present_mode = ChooseSwapchainPresentMode(support.presentModes);
	vk::Extent2D extent = ChooseSwapchainExtent(support.capabilities);

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

	QueueFamilyIndices indices = FindQueueFamilies(physical_device);
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
		.setQueueFamilyIndexCount(0)		// Optional
		.setPQueueFamilyIndices(nullptr);	// Optional
	}

	
}
