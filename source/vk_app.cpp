#include "vk_app.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <vector>
#include <set>

using std::string;
using std::vector;
using std::set;
using std::cout;
using std::endl;

VkApp::VkApp(string t, uint32_t w, uint32_t h, bool enable_validation) {
	title	= t;
	width	= w;
	height	= h;

	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");

	#ifdef NDEBUG
	validation_enabled = enable_validation;
	#else
	validation_enabled = true;
	#endif

	if (validation_enabled) {
		cout << "Validation layers enabled!" << endl;
	}
}

void VkApp::Run() {
	InitWindow();
	InitVulkan();

	MainLoop();

	Cleanup();
}

void VkApp::InitWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

void VkApp::MainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void VkApp::Cleanup() {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)
		instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) { func(instance, callback, nullptr); }

	device.destroySwapchainKHR(swapchain);
	instance.destroySurfaceKHR(surface);

	device.destroy();
	instance.destroy();

	glfwDestroyWindow(window);
}

// #############################################################################
// Vulkan stuff

void VkApp::InitVulkan() {
	CreateInstance();
	SetupDebugCallback();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapchain();
}

void VkApp::CreateInstance() {
	if (validation_enabled && !CheckValidationLayerSupport()) {
		throw std::runtime_error("Validation layers not available");
	}

	vk::ApplicationInfo app_info;
	app_info.pApplicationName = title.c_str();
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.apiVersion = VK_API_VERSION_1_0;

	auto extensions = GetRequiredExtensions();
	if (!CheckExtensionsSupport(extensions)) {
		throw std::runtime_error("Required extensions not supported");
	}

	vk::InstanceCreateInfo instance_info;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = extensions.size();
	instance_info.ppEnabledExtensionNames = extensions.data();

	if (validation_enabled) {
		instance_info.enabledLayerCount = validationLayers.size();
		instance_info.ppEnabledLayerNames = validationLayers.data();
	}

	instance.destroy();
	vk::Result r = vk::createInstance(&instance_info, nullptr, &instance);
	if (r != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

vector<const char*> VkApp::GetRequiredExtensions() {
	vector<const char*> extensions;

	unsigned int glfw_ext_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

	for (unsigned int i = 0; i < glfw_ext_count; i++) {
		extensions.push_back(glfw_extensions[i]);
	}

	if (validation_enabled) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool VkApp::CheckExtensionsSupport(vector<const char*> extensions) {
	using std::set;
	set<string> required_extensions(extensions.begin(), extensions.end());

	cout << "Looking for extensions:" << endl;
	for (auto ext : extensions) cout << "\t" << ext << endl;

	vector<vk::ExtensionProperties> available_extensions =
		vk::enumerateInstanceExtensionProperties();

	for (const auto& ext : available_extensions) {
		required_extensions.erase(ext.extensionName);
	}

	if (!required_extensions.empty()) {
		cout << "Required extensions not found:" << endl;
		for (auto ext : required_extensions) {
			cout << "\t" << ext << endl;
		}
		cout << endl;
	} else {
		cout << "-- All extensions were found." << endl << endl;
	}

	return required_extensions.empty();
}

bool VkApp::CheckValidationLayerSupport() {
	vector<vk::LayerProperties> available_layers =
		vk::enumerateInstanceLayerProperties();

	cout << "Looking or validation layers:" << endl;
	for (const char* layer_name : validationLayers) {
		bool layer_found = false;

		cout << "\t" << layer_name << "... ";

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName)) {
				layer_found = true;
				cout << "found." << endl;
				break;
			}
		}

		if (!layer_found) {
			cout << "not found." << endl << endl;
			return false;
		}
	}

	cout << endl;
	return true;
}

void VkApp::SetupDebugCallback() {
	if (!validation_enabled) return;

	VkDebugReportCallbackCreateInfoEXT callback_info;
	callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	callback_info.pfnCallback = DebugCallback;

	VkResult r;
	auto func = (PFN_vkCreateDebugReportCallbackEXT)
		instance.getProcAddr("vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		r = func(
			instance,
			&(callback_info),
			nullptr, &callback
		);
	}

	if (r != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkApp::DebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData
) {
	std::cerr << "Validation layer: " << msg << std::endl;
	return VK_FALSE;
}

void VkApp::CreateSurface() {
	instance.destroySurfaceKHR(surface, nullptr);
	VkSurfaceKHR s = (VkSurfaceKHR) surface;
	VkResult r = glfwCreateWindowSurface(instance, window, nullptr, &s);

	if (r != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}

	surface = s;
}

void VkApp::PickPhysicalDevice() {
	vector<vk::PhysicalDevice> devices;
	devices = instance.enumeratePhysicalDevices();

	if (devices.size() == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}

	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physical_device = device;
		}
	}

	if (!physical_device) {
		throw std::runtime_error("Failed to find a suitable GPU");
	}
}

bool VkApp::isDeviceSuitable(vk::PhysicalDevice device) {
	QueueFamilyIndices indices = FindQueueFamilies(device);
	bool extensions_supported = CheckDeviceExtensionSupport(device);

	bool swapchain_adequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swapchain_support =
			QuerySwapchainSupport(device);

		swapchain_adequate = !swapchain_support.formats.empty() and
			!swapchain_support.present_modes.empty();
	}

	return indices.isComplete() and extensions_supported and swapchain_adequate;
}

bool QueueFamilyIndices::isComplete() {
	return graphics_family >= 0 and present_family >= 0;
}

QueueFamilyIndices VkApp::FindQueueFamilies(vk::PhysicalDevice device) {
	QueueFamilyIndices indices;

	vector<vk::QueueFamilyProperties> queue_families;
	queue_families = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queue_family : queue_families) {
		if (queue_family.queueCount > 0 && queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphics_family = i;
		}

		VkBool32 presentation_support = false;
		device.getSurfaceSupportKHR(i, surface, &presentation_support);
		if (queue_family.queueCount > 0  && presentation_support) {
			indices.present_family = i;
		}

		if (indices.isComplete()) break;
		i++;
	}

	return indices;
}

void VkApp::CreateLogicalDevice() {
	QueueFamilyIndices indices = FindQueueFamilies(physical_device);

	vector<vk::DeviceQueueCreateInfo> queue_infos;
	set<int> unique_queue_families = {
		indices.graphics_family, indices.present_family
	};

	float queue_priority = 1.0f;
	for (int queue_family : unique_queue_families) {
		vk::DeviceQueueCreateInfo queue_info;
		queue_info.queueFamilyIndex = queue_family;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queue_priority;

		queue_infos.push_back(queue_info);
	}

	vk::PhysicalDeviceFeatures device_features;

	vk::DeviceCreateInfo device_info;
	device_info.queueCreateInfoCount = (uint32_t) queue_infos.size();
	device_info.pQueueCreateInfos = queue_infos.data();
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = deviceExtensions.size();
	device_info.ppEnabledExtensionNames = deviceExtensions.data();
	if (validation_enabled) {
		device_info.enabledLayerCount = validationLayers.size();
		device_info.ppEnabledLayerNames = validationLayers.data();
	}

	vk::Result r = physical_device.createDevice(&device_info, nullptr, &device);
	if (r != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create logical device");
	}

	graphics_queue = device.getQueue(indices.graphics_family, 0);
	presentation_queue = device.getQueue(indices.present_family, 0);
}

bool VkApp::CheckDeviceExtensionSupport(vk::PhysicalDevice device) {
	vector<vk::ExtensionProperties> available_extensions
		= device.enumerateDeviceExtensionProperties();

	set<string> required_extensions(
		deviceExtensions.begin(), deviceExtensions.end()
	);

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

SwapChainSupportDetails VkApp::QuerySwapchainSupport(vk::PhysicalDevice device) {
	SwapChainSupportDetails details;

	details.capabilities	= device.getSurfaceCapabilitiesKHR(surface);
	details.formats		= device.getSurfaceFormatsKHR(surface);
	details.present_modes	= device.getSurfacePresentModesKHR(surface);

	return details;
}

vk::SurfaceFormatKHR VkApp::ChooseSwapSurfaceFormat(
	const vector<vk::SurfaceFormatKHR>& available_formats
) {
	if (
		available_formats.size() == 1 and
		available_formats[0].format == vk::Format::eUndefined
	) {
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& format : available_formats) {
		if (
			format.format == vk::Format::eB8G8R8A8Unorm and
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
		) {
			return format;
		}
	}

	return available_formats[0];
}

vk::PresentModeKHR VkApp::ChooseSwapPresentMode(
	const vector<vk::PresentModeKHR>& available_present_modes
) {
	for (const auto& mode : available_present_modes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VkApp::ChooseSwapExtent(
	const vk::SurfaceCapabilitiesKHR& capabilities
) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	vk::Extent2D extent = { width, height };
	extent.width = std::max(capabilities.minImageExtent.width,
		std::min(capabilities.maxImageExtent.width, extent.width));
	extent.height = std::max(capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, extent.height));

	return extent;
}

void VkApp::CreateSwapchain() {
	SwapChainSupportDetails support = QuerySwapchainSupport(physical_device);
	vk::SurfaceFormatKHR format = ChooseSwapSurfaceFormat(support.formats);
	vk::PresentModeKHR mode	= ChooseSwapPresentMode(support.present_modes);

	swapchain_extent = ChooseSwapExtent(support.capabilities);
	swapchain_format = format.format;

	uint32_t image_count = support.capabilities.minImageCount + 1;
	if (
		support.capabilities.maxImageCount > 0 and
		image_count > support.capabilities.maxImageCount
	) {
		image_count = support.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapchain_info;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = image_count;
	swapchain_info.imageFormat = format.format;
	swapchain_info.imageColorSpace = format.colorSpace;
	swapchain_info.imageExtent = swapchain_extent;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	QueueFamilyIndices indices = FindQueueFamilies(physical_device);
	uint32_t queue_families_indices[] = {
		(uint32_t) indices.graphics_family,
		(uint32_t) indices.present_family
	};
	if (indices.graphics_family != indices.present_family) {
		swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchain_info.queueFamilyIndexCount = 2;
		swapchain_info.pQueueFamilyIndices = queue_families_indices;
	} else {
		swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
	}
	swapchain_info.preTransform = support.capabilities.currentTransform;
	swapchain_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapchain_info.presentMode = mode;
	swapchain_info.clipped = true;

	vk::Result r = device.createSwapchainKHR(&swapchain_info, nullptr, &swapchain);
	if (r != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create swapchain");
	}

	swapchain_images = device.getSwapchainImagesKHR(swapchain);
}
