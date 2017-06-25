#include "application.hpp"

#include <iostream>
#include <cstring>
#include <stdexcept>

using std::cout;
using std::cerr;
using std::endl;

using std::runtime_error;

using std::string;
using std::vector;

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

	// Destroy vulkan instance
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
	SetupDebugCallback();
	PickPhysicalDevice();
	CreateLogicalDevice();
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

bool _is_device_suitable(vk::PhysicalDevice device) {
	QueueFamilyIndices indices = FindQueueFamilies(device);
	return indices.is_complete();
}

void Application::PickPhysicalDevice() {
	vector<vk::PhysicalDevice> devices;
	devices = instance.enumeratePhysicalDevices();

	if (devices.size() == 0) {
		throw runtime_error("Filed to find GPUs with Vulkan support");
	}

	for (const auto& device : devices) {
		if (_is_device_suitable(device)) {
			physical_device = device;
			break;
		}
	}

	if (!physical_device) {
		throw runtime_error("Failed to find a suitable GPU");
	}
}

bool QueueFamilyIndices::is_complete() {
	return graphics >= 0;
}

QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device) {
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

		if (indices.is_complete()) { break; }

		i++;
	}

	return indices;
}

void Application::CreateLogicalDevice() {
	float queue_priority = 1.0f;
	QueueFamilyIndices indices = FindQueueFamilies(physical_device);

	auto queue_info = vk::DeviceQueueCreateInfo()
	.setQueueFamilyIndex(indices.graphics)
	.setQueueCount(1)
	.setPQueuePriorities(&queue_priority);

	auto device_features = vk::PhysicalDeviceFeatures();	// TODO

	auto device_info = vk::DeviceCreateInfo()
	.setPQueueCreateInfos(&queue_info)
	.setQueueCreateInfoCount(1)
	.setPEnabledFeatures(&device_features)
	.setEnabledExtensionCount(0);

	if (enable_validation_layers) {
		device_info.setEnabledLayerCount(static_cast<uint32_t>(validation_layers.size()));
		device_info.setPpEnabledLayerNames(validation_layers.data());
	} else {
		device_info.setEnabledLayerCount(0);
	}

	device = physical_device.createDevice(device_info);
	graphics_queue = device.getQueue(indices.graphics, 0);
}
