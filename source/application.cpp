#include "application.hpp"

#include <iostream>
#include <cstring>
#include <stdexcept>

using std::cout;
using std::cerr;
using std::endl;

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
}

void Application::MainLoop() {
	while (!window->should_close()) {
		glfwPollEvents();
	}
}

void Application::CreateVulkanInstance() {
	if (enable_validation_layers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("Validation layers not available");
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
		throw std::runtime_error("Failed to setup debug callback");
	}
}
