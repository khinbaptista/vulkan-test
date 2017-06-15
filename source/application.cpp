#include "application.hpp"

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

using std::string;
using std::vector;

Application::Application(string title, bool validate) {
	window = new Window(title, 800, 600);
}

Application::~Application() {
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
}

void Application::MainLoop() {
	while (!window->should_close()) {
		glfwPollEvents();
	}
}

void Application::CreateVulkanInstance() {
	auto app_info = vk::ApplicationInfo()
	.setPApplicationName(window->title().c_str())
	.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
	.setPEngineName("Godot Engine")
	.setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
	.setApiVersion(VK_API_VERSION_1_0);

	unsigned int glfw_extensions_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

	auto instance_info = vk::InstanceCreateInfo()
	.setPApplicationInfo(&app_info)
	.setEnabledExtensionCount(glfw_extensions_count)
	.setPpEnabledExtensionNames(glfw_extensions)
	.setEnabledLayerCount(0);
	instance = vk::createInstance(instance_info);

	vector<vk::ExtensionProperties> extensions;
	extensions = vk::enumerateInstanceExtensionProperties();

	cout << "Available extensions:" << endl;
	for (const auto& extension : extensions) {
		cout << "\t" << extension.extensionName << endl;
	}
}
