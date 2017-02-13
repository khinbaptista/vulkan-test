#include "vk_app.hpp"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <set>

using std::string;
using std::vector;
using std::cout;
using std::endl;

VkApp::VkApp(string t, int w, int h, bool enable_validation) {
	title	= t;
	width	= w;
	height	= h;

	validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");

#ifdef NDEBUG
	validation_enabled = enable_validation;
#else
	validation_enabled = true;
#endif
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
	glfwDestroyWindow(window);

	auto func = (PFN_vkDestroyDebugReportCallbackEXT)
		instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) { func(instance, callback, nullptr); }

	instance.destroy();
}

// #############################################################################
// Vulkan stuff

void VkApp::InitVulkan() {
	CreateInstance();
	SetupDebugCallback();
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
