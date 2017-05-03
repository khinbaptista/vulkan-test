#include "vk_app.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <vector>
#include <set>

#include <thread>
#include <chrono>

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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	//glfwWindowHint(GLFW_REFRESH_RATE, 60);

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, VkApp::OnWindowResized);
}

void VkApp::MainLoop() {
	using namespace std::chrono_literals;

	auto start = std::chrono::high_resolution_clock::now();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		UpdateUniformBuffer();
		DrawFrame();

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = end - start;
		if (elapsed.count() < 15.0f) {
			std::this_thread::sleep_for(10ms);
		}

		start = std::chrono::high_resolution_clock::now();
	}
}

void VkApp::Cleanup() {
	device.waitIdle();

	auto func = (PFN_vkDestroyDebugReportCallbackEXT)
		instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) { func(instance, callback, nullptr); }

	device.destroyDescriptorPool(descriptor_pool);

	device.destroyBuffer(uniform_staging_buffer);
	device.freeMemory(uniform_staging_buffer_memory);

	device.destroyBuffer(uniform_buffer);
	device.freeMemory(uniform_buffer_memory);

	device.destroyBuffer(index_buffer);
	device.freeMemory(index_buffer_memory);

	device.destroyBuffer(vertex_buffer);
	device.freeMemory(vertex_buffer_memory);

	device.destroySwapchainKHR(swapchain);
	instance.destroySurfaceKHR(surface);
	device.destroyCommandPool(command_pool);

	size_t views_count = swapchain_imageviews.size();
	for (size_t i = 0; i < views_count; i++) {
		device.destroyImageView(swapchain_imageviews.back());
		swapchain_imageviews.pop_back();
	}

	size_t framebuffer_count = swapchain_framebuffers.size();
	for (size_t i = 0; i < framebuffer_count; i++) {
		device.destroyFramebuffer(swapchain_framebuffers.back());
		swapchain_framebuffers.pop_back();
	}

	device.destroySemaphore(semaphore_render_finished);
	device.destroySemaphore(semaphore_image_available);

	device.destroyDescriptorSetLayout(descriptor_set_layout);

	device.destroyPipelineLayout(pipeline_layout);
	device.destroyRenderPass(render_pass);
	device.destroyPipeline(graphics_pipeline);

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
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPool();

	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateCommandBuffers();

	CreateSemaphores();
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
	instance_info.enabledExtensionCount = (uint32_t) extensions.size();
	instance_info.ppEnabledExtensionNames = extensions.data();

	if (validation_enabled) {
		instance_info.enabledLayerCount = (uint32_t) validationLayers.size();
		instance_info.ppEnabledLayerNames = validationLayers.data();
	}

	if (instance) instance.destroy();
	instance = vk::createInstance(instance_info);
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
	vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();

	cout << "Looking for validation layers:" << endl;
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
		SwapChainSupportDetails swapchain_support = QuerySwapchainSupport(device);

		swapchain_adequate =
		!swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
	}

	return indices.isComplete() && extensions_supported && swapchain_adequate;
}

bool QueueFamilyIndices::isComplete() {
	return graphics_family >= 0 && present_family >= 0;
}

QueueFamilyIndices VkApp::FindQueueFamilies(vk::PhysicalDevice device) {
	QueueFamilyIndices indices;

	vector<vk::QueueFamilyProperties> queue_families;
	queue_families = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queue_family : queue_families) {
		if (	queue_family.queueCount > 0 &&
			queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
		{
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
	device_info.enabledExtensionCount = (uint32_t) deviceExtensions.size();
	device_info.ppEnabledExtensionNames = deviceExtensions.data();
	if (validation_enabled) {
		device_info.enabledLayerCount = (uint32_t) validationLayers.size();
		device_info.ppEnabledLayerNames = validationLayers.data();
	}

	device = physical_device.createDevice(device_info);

	graphics_queue = device.getQueue(indices.graphics_family, 0);
	presentation_queue = device.getQueue(indices.present_family, 0);
}

bool VkApp::CheckDeviceExtensionSupport(vk::PhysicalDevice device) {
	vector<vk::ExtensionProperties> available_extensions
		= device.enumerateDeviceExtensionProperties();
	set<string> required_extensions(deviceExtensions.begin(), deviceExtensions.end());

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

vk::SurfaceFormatKHR
VkApp::ChooseSwapSurfaceFormat(const vector<vk::SurfaceFormatKHR>& available_formats) {
	if (
		available_formats.size() == 1 &&
		available_formats[0].format == vk::Format::eUndefined
	) {
		return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	for (const auto& format : available_formats) {
		if (
			format.format == vk::Format::eB8G8R8A8Unorm &&
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
	if (	support.capabilities.maxImageCount > 0 &&
		image_count > support.capabilities.maxImageCount)
	{
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

	vk::SwapchainKHR old_swapchain;
	if (swapchain) {
		old_swapchain = swapchain;
		swapchain_info.oldSwapchain = old_swapchain;
	}

	vk::SwapchainKHR new_swapchain;
	new_swapchain = device.createSwapchainKHR(swapchain_info);

	if (old_swapchain) device.destroySwapchainKHR(old_swapchain);
	swapchain = new_swapchain;
	swapchain_images = device.getSwapchainImagesKHR(swapchain);
}

void VkApp::RecreateSwapchain() {
	device.waitIdle();

	CreateSwapchain();
	CreateImageViews();
	//CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandBuffers();
}

void VkApp::OnWindowResized(GLFWwindow* window, int w, int h) {
	if (w == 0 || h == 0) return;

	VkApp *app = reinterpret_cast<VkApp*>(glfwGetWindowUserPointer(window));
	app->width = w;
	app->height = h;
	app->RecreateSwapchain();
}

void VkApp::CreateImageViews() {
	swapchain_imageviews.resize(swapchain_images.size());

	for (uint32_t i = 0; i < swapchain_images.size(); i++) {
		vk::ImageViewCreateInfo view_info = vk::ImageViewCreateInfo()
		.setImage(swapchain_images[i])
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(swapchain_format);

		view_info.subresourceRange
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)	// optional
			.setLevelCount(1)
			.setBaseArrayLayer(0)	// optional
			.setLayerCount(1);

		if (swapchain_imageviews[i]) device.destroyImageView(swapchain_imageviews[i]);
		swapchain_imageviews[i] = device.createImageView(view_info);
	}
}

void VkApp::CreateGraphicsPipeline() {
	#ifdef _WIN32
		auto vertex_shader_code = ReadFile("../../../shaders/vertex-v.spv");
		auto fragment_shader_code = ReadFile("../../../shaders/fragment-f.spv");
	#else
		auto vertex_shader_code	  = ReadFile("shaders/vertex-v.spv");
		auto fragment_shader_code = ReadFile("shaders/fragment-f.spv");
	#endif
	vk::ShaderModule vertex_smodule;
	vk::ShaderModule fragment_smodule;

	CreateShaderModule(vertex_shader_code, vertex_smodule);
	CreateShaderModule(fragment_shader_code, fragment_smodule);

	auto vert_pipeline_info = vk::PipelineShaderStageCreateInfo()
	.setStage(vk::ShaderStageFlagBits::eVertex)
	.setModule(vertex_smodule)
	.setPName("main");

	auto frag_pipeline_info = vk::PipelineShaderStageCreateInfo()
	.setStage(vk::ShaderStageFlagBits::eFragment)
	.setModule(fragment_smodule)
	.setPName("main");

	vk::PipelineShaderStageCreateInfo shader_stages[] = {
		vert_pipeline_info, frag_pipeline_info
	};

	auto binding_description = Vertex::GetBindingDescription();
	auto attribute_descriptions = Vertex::GetAttributeDescriptions();

	auto vert_input_info = vk::PipelineVertexInputStateCreateInfo()
	.setVertexBindingDescriptionCount(1)
	.setPVertexBindingDescriptions(&binding_description)
	.setVertexAttributeDescriptionCount(attribute_descriptions.size())
	.setPVertexAttributeDescriptions(attribute_descriptions.data());

	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
	.setTopology(vk::PrimitiveTopology::eTriangleList)
	.setPrimitiveRestartEnable(false);

	auto viewport = vk::Viewport()
	.setX(0.0f)
	.setY(0.0f)
	.setWidth((float) swapchain_extent.width)
	.setHeight((float) swapchain_extent.height)
	.setMinDepth(0.0f)
	.setMaxDepth(1.0f);

	auto scissor = vk::Rect2D()
	.setOffset({ 0, 0 })
	.setExtent(swapchain_extent);

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
	.setViewportCount(1)
	.setPViewports(&viewport)
	.setScissorCount(1)
	.setPScissors(&scissor);

	auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
	.setDepthClampEnable(false)
	.setRasterizerDiscardEnable(false)
	.setPolygonMode(vk::PolygonMode::eFill)
	.setLineWidth(1.0f)
	.setCullMode(vk::CullModeFlagBits::eBack)
	.setFrontFace(vk::FrontFace::eCounterClockwise)
	.setDepthBiasEnable(false)
	.setDepthBiasConstantFactor(0.0f)
	.setDepthBiasClamp(0.0f)
	.setDepthBiasSlopeFactor(0.0f);

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
	.setSampleShadingEnable(false)
	.setRasterizationSamples(vk::SampleCountFlagBits::e1)
	.setMinSampleShading(1.0f)
	.setPSampleMask(nullptr)
	.setAlphaToCoverageEnable(false)
	.setAlphaToOneEnable(false);

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
	.setColorWriteMask(
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA )
	.setBlendEnable(false)
	.setSrcColorBlendFactor(vk::BlendFactor::eOne)
	.setDstColorBlendFactor(vk::BlendFactor::eZero)
	.setColorBlendOp(vk::BlendOp::eAdd)
	.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
	.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
	.setAlphaBlendOp(vk::BlendOp::eAdd);

	auto color_blending = vk::PipelineColorBlendStateCreateInfo()
	.setLogicOpEnable(false)
	.setLogicOp(vk::LogicOp::eCopy)
	.setAttachmentCount(1)
	.setPAttachments(&color_blend_attachment)
	.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	vk::DescriptorSetLayout layouts[] = { descriptor_set_layout };
	auto layout_info = vk::PipelineLayoutCreateInfo()
	.setSetLayoutCount(1)
	.setPSetLayouts(layouts)
	.setPushConstantRangeCount(0)
	.setPPushConstantRanges(nullptr);

	if (pipeline_layout) device.destroyPipelineLayout(pipeline_layout);
	pipeline_layout = device.createPipelineLayout(layout_info);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
	.setStageCount(2)
	.setPStages(shader_stages)
	.setPVertexInputState(&vert_input_info)
	.setPInputAssemblyState(&input_assembly)
	.setPViewportState(&viewport_state)
	.setPRasterizationState(&rasterizer)
	.setPMultisampleState(&multisampling)
	.setPDepthStencilState(nullptr)
	.setPColorBlendState(&color_blending)
	.setPDynamicState(nullptr)
	.setLayout(pipeline_layout)
	.setRenderPass(render_pass)
	.setSubpass(0)
	.setBasePipelineHandle(nullptr)
	.setBasePipelineIndex(-1);

	if (graphics_pipeline) device.destroyPipeline(graphics_pipeline);
	graphics_pipeline = device.createGraphicsPipeline(nullptr, pipeline_info);

	device.destroyShaderModule(fragment_smodule);
	device.destroyShaderModule(vertex_smodule);
}

void VkApp::CreateFramebuffers() {
	swapchain_framebuffers.resize(swapchain_imageviews.size());

	for (size_t i = 0; i < swapchain_imageviews.size(); i++) {
		vk::ImageView attachments[] = { swapchain_imageviews[i] };

		auto framebuffer_info = vk::FramebufferCreateInfo()
		.setRenderPass(render_pass)
		.setAttachmentCount(1)
		.setPAttachments(attachments)
		.setWidth(swapchain_extent.width)
		.setHeight(swapchain_extent.height)
		.setLayers(1);

		if (swapchain_framebuffers[i]) device.destroyFramebuffer(swapchain_framebuffers[i]);
		swapchain_framebuffers[i] = device.createFramebuffer(framebuffer_info);
	}
}

vector<char> VkApp::ReadFile(const string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file " + filename);
	}

	size_t file_size = (size_t) file.tellg();
	vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

void VkApp::CreateShaderModule(const vector<char>& code, vk::ShaderModule& module) {
	vk::ShaderModuleCreateInfo module_info = vk::ShaderModuleCreateInfo()
	.setCodeSize(code.size())
	.setPCode((uint32_t*) code.data());

	module = device.createShaderModule(module_info);
}

void VkApp::CreateRenderPass() {
	auto color_attachment = vk::AttachmentDescription()
	.setFormat(swapchain_format)
	.setSamples(vk::SampleCountFlagBits::e1)
	.setLoadOp(vk::AttachmentLoadOp::eClear)
	.setStoreOp(vk::AttachmentStoreOp::eStore)
	.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	.setInitialLayout(vk::ImageLayout::eUndefined)
	.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	auto color_attachment_ref = vk::AttachmentReference()
	.setAttachment(0)
	.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass;
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	.setColorAttachmentCount(1)
	.setPColorAttachments(&color_attachment_ref);

	auto dependency = vk::SubpassDependency()
	.setSrcSubpass(VK_SUBPASS_EXTERNAL)
	.setDstSubpass(0)
	.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
	.setDstAccessMask(
		vk::AccessFlagBits::eColorAttachmentRead |
		vk::AccessFlagBits::eColorAttachmentWrite
	);

	vk::RenderPassCreateInfo renderpass_info;
	renderpass_info.setAttachmentCount(1)
	.setPAttachments(&color_attachment)
	.setSubpassCount(1)
	.setPSubpasses(&subpass)
	.setDependencyCount(1)
	.setPDependencies(&dependency);

	if (render_pass) device.destroyRenderPass(render_pass);
	render_pass = device.createRenderPass(renderpass_info);
}

void VkApp::CreateCommandPool() {
	QueueFamilyIndices queue_families_indices = FindQueueFamilies(physical_device);

	auto command_pool_info = vk::CommandPoolCreateInfo()
	.setQueueFamilyIndex(queue_families_indices.graphics_family);

	if (command_pool) device.destroyCommandPool(command_pool);
	command_pool = device.createCommandPool(command_pool_info);
}

void VkApp::CreateCommandBuffers() {
	if (command_buffers.size() > 0) {
		device.freeCommandBuffers(command_pool, command_buffers);
	}

	command_buffers.resize(swapchain_framebuffers.size());

	auto allocate_info = vk::CommandBufferAllocateInfo()
	.setCommandPool(command_pool)
	.setLevel(vk::CommandBufferLevel::ePrimary)
	.setCommandBufferCount((uint32_t) command_buffers.size());

	command_buffers = device.allocateCommandBuffers(allocate_info);

	for (size_t i = 0; i < command_buffers.size(); i++) {
		auto begin_info = vk::CommandBufferBeginInfo()
		.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
		.setPInheritanceInfo(nullptr);

		command_buffers[i].begin(begin_info);

		auto clear_color = vk::ClearValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
		auto renderpass_info = vk::RenderPassBeginInfo()
		.setRenderPass(render_pass)
		.setFramebuffer(swapchain_framebuffers[i])
		.setRenderArea({ { 0, 0 }, swapchain_extent })
		.setClearValueCount(1)
		.setPClearValues(&clear_color);

		command_buffers[i].beginRenderPass(renderpass_info, vk::SubpassContents::eInline);
		command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);

		vk::Buffer vertex_buffers[] = { vertex_buffer };
		vk::DeviceSize offsets[] = { 0 };
		command_buffers[i].bindVertexBuffers(0, 1, vertex_buffers, offsets);
		command_buffers[i].bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint16);

		command_buffers[i].bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipeline_layout,
			0,
			{ descriptor_set },
			{}
		);

		// vertex count, instance count, first vertex and first instance
		//command_buffers[i].draw(vertices.size(), 1, 0, 0);
		command_buffers[i].drawIndexed(indices.size(), 1, 0, 0, 0);

		command_buffers[i].endRenderPass();

		command_buffers[i].end();
	}
}

void VkApp::DrawFrame() {
	uint32_t image_index;
	vk::Result r = device.acquireNextImageKHR(
		swapchain,
		std::numeric_limits<uint64_t>::max(),
		semaphore_image_available,
		nullptr,
		&image_index
	);

	if (r == vk::Result::eErrorOutOfDateKHR) {
		RecreateSwapchain();
		return;
	} else if (r != vk::Result::eSuccess && r != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error("Failed to acquire swapchain image");
	}

	vk::Semaphore wait_semaphores[]   = { semaphore_image_available };
	vk::Semaphore signal_semaphores[] = { semaphore_render_finished };
	vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	auto submit_info = vk::SubmitInfo()
	.setWaitSemaphoreCount(1)
	.setPWaitSemaphores(wait_semaphores)
	.setPWaitDstStageMask(wait_stages)
	.setCommandBufferCount(1)
	.setPCommandBuffers(&command_buffers[image_index])
	.setSignalSemaphoreCount(1)
	.setPSignalSemaphores(signal_semaphores);

	graphics_queue.submit({ submit_info }, nullptr);

	vk::SwapchainKHR swapchains[] = { swapchain };
	auto present_info = vk::PresentInfoKHR()
	.setWaitSemaphoreCount(1)
	.setPWaitSemaphores(signal_semaphores)
	.setSwapchainCount(1)
	.setPSwapchains(swapchains)
	.setPImageIndices(&image_index);

	r = presentation_queue.presentKHR(present_info);
	if (r == vk::Result::eErrorOutOfDateKHR || r == vk::Result::eSuboptimalKHR) {
		RecreateSwapchain();
	} else if (r != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to present swapchain image");
	}
}

void VkApp::CreateSemaphores() {
	semaphore_image_available = device.createSemaphore({});
	semaphore_render_finished = device.createSemaphore({});
}

vk::VertexInputBindingDescription Vertex::GetBindingDescription() {
	auto description = vk::VertexInputBindingDescription()
	.setBinding(0)
	.setStride(sizeof(Vertex))
	.setInputRate(vk::VertexInputRate::eVertex);

	return description;
}

std::array<vk::VertexInputAttributeDescription, 2>
Vertex::GetAttributeDescriptions() {
	std::array<vk::VertexInputAttributeDescription, 2> descriptions = {};

	descriptions[0].setBinding(0)
	.setLocation(0)
	.setFormat(vk::Format::eR32G32Sfloat)
	.setOffset(offsetof(Vertex, pos));

	descriptions[1].setBinding(0)
	.setLocation(1)
	.setFormat(vk::Format::eR32G32B32Sfloat)
	.setOffset(offsetof(Vertex, color));

	return descriptions;
}

uint32_t VkApp::FindMemoryType(uint32_t filter, vk::MemoryPropertyFlags properties) {
	vk::PhysicalDeviceMemoryProperties mem_properties;
	mem_properties = physical_device.getMemoryProperties();

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (filter & ((uint32_t)(1 << i)) && ((mem_properties.memoryTypes[i].propertyFlags & properties) == properties)) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
	return 0;
}

vk::Buffer VkApp::CreateBuffer(
	vk::DeviceSize size, vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags properties, vk::DeviceMemory& memory
) {
	vk::Buffer buffer;

	auto buffer_info = vk::BufferCreateInfo()
	.setSize(size)
	.setUsage(usage)
	.setSharingMode(vk::SharingMode::eExclusive);
	buffer = device.createBuffer(buffer_info);

	vk::MemoryRequirements mem_requirements;
	mem_requirements = device.getBufferMemoryRequirements(buffer);

	auto alloc_info = vk::MemoryAllocateInfo()
	.setAllocationSize(mem_requirements.size)
	.setMemoryTypeIndex(
		FindMemoryType(mem_requirements.memoryTypeBits, properties)
	);
	memory = device.allocateMemory(alloc_info);

	device.bindBufferMemory(buffer, memory, 0);
	return buffer;
}

void VkApp::CreateVertexBuffer() {
	vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	vk::Buffer staging_buffer;
	vk::DeviceMemory staging_buffer_memory;
	staging_buffer = CreateBuffer(
		buffer_size,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent,
		staging_buffer_memory
	);

	void* data;
	data = device.mapMemory(staging_buffer_memory, 0, buffer_size, {});
	memcpy(data, vertices.data(), (size_t) buffer_size);
	device.unmapMemory(staging_buffer_memory);

	vertex_buffer = CreateBuffer(
		buffer_size,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vertex_buffer_memory
	);

	CopyBuffer(staging_buffer, vertex_buffer, buffer_size);

	device.destroyBuffer(staging_buffer);
	device.freeMemory(staging_buffer_memory);
}

void VkApp::CreateIndexBuffer() {
	vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();

	vk::Buffer staging_buffer;
	vk::DeviceMemory staging_buffer_memory;
	staging_buffer = CreateBuffer(
		buffer_size,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
		vk::MemoryPropertyFlagBits::eHostCoherent,
		staging_buffer_memory
	);

	void* data;
	data = device.mapMemory(staging_buffer_memory, 0, buffer_size, {});
	memcpy(data, indices.data(), (size_t) buffer_size);
	device.unmapMemory(staging_buffer_memory);

	index_buffer = CreateBuffer(
		buffer_size,
		vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		index_buffer_memory
	);

	CopyBuffer(staging_buffer, index_buffer, buffer_size);

	device.destroyBuffer(staging_buffer);
	device.freeMemory(staging_buffer_memory);
}

void VkApp::CopyBuffer(vk::Buffer source, vk::Buffer destination, vk::DeviceSize size) {
	auto alloc_info = vk::CommandBufferAllocateInfo()
	.setLevel(vk::CommandBufferLevel::ePrimary)
	.setCommandPool(command_pool)
	.setCommandBufferCount(1);

	vk::CommandBuffer command_buffer;
	device.allocateCommandBuffers(&alloc_info, &command_buffer);

	command_buffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	auto region = vk::BufferCopy()
	.setSrcOffset(0)
	.setDstOffset(0)
	.setSize(size);

	command_buffer.copyBuffer(source, destination, 1, &region);
	command_buffer.end();

	auto submit_info = vk::SubmitInfo()
	.setCommandBufferCount(1)
	.setPCommandBuffers(&command_buffer);

	graphics_queue.submit({ submit_info }, nullptr);
	graphics_queue.waitIdle();

	device.freeCommandBuffers(command_pool, { command_buffer });
}

void VkApp::CreateDescriptorSetLayout() {
	auto ubo_layout_binding = vk::DescriptorSetLayoutBinding()
	.setDescriptorType(vk::DescriptorType::eUniformBuffer)
	.setDescriptorCount(1)
	.setStageFlags(vk::ShaderStageFlagBits::eVertex)
	.setPImmutableSamplers(nullptr);

	auto layout_info = vk::DescriptorSetLayoutCreateInfo()
	.setBindingCount(1)
	.setPBindings(&ubo_layout_binding);
	descriptor_set_layout = device.createDescriptorSetLayout(layout_info);
}

void VkApp::CreateUniformBuffer() {
	vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

	uniform_staging_buffer = CreateBuffer(
		buffer_size,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent,
		uniform_staging_buffer_memory
	);

	uniform_buffer = CreateBuffer(
		buffer_size,
		vk::BufferUsageFlagBits::eTransferDst
		| vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		uniform_buffer_memory
	);
}

void VkApp::UpdateUniformBuffer() {
	static auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(
		glm::mat4(),
		time * glm::radians(90.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),	// eye
		glm::vec3(0.0f, 0.0f, 0.0f),	// target
		glm::vec3(0.0f, 0.0f, 1.0f)		// up
	);
	ubo.proj = glm::perspective(
		glm::radians(45.0f),	// vertical field-of-view
		swapchain_extent.width / (float) swapchain_extent.height,	// aspect ratio
		0.1f,		// near
		10.0f		// far
	);
	ubo.proj[1][1] *= -1.0f;

	void* data;
	data = device.mapMemory(uniform_staging_buffer_memory, 0, sizeof(ubo), {});
	memcpy(data, &ubo, sizeof(ubo));
	device.unmapMemory(uniform_staging_buffer_memory);

	CopyBuffer(uniform_staging_buffer, uniform_buffer, sizeof(ubo));
}

void VkApp::CreateDescriptorPool() {
	auto pool_size = vk::DescriptorPoolSize()
	.setType(vk::DescriptorType::eUniformBuffer)
	.setDescriptorCount(1);

	auto pool_info = vk::DescriptorPoolCreateInfo()
	.setPoolSizeCount(1)
	.setPPoolSizes(&pool_size)
	.setMaxSets(1);
	descriptor_pool = device.createDescriptorPool(pool_info);
}

void VkApp::CreateDescriptorSet() {
	vk::DescriptorSetLayout layouts[] = { descriptor_set_layout };

	auto alloc_info = vk::DescriptorSetAllocateInfo()
	.setDescriptorPool(descriptor_pool)
	.setDescriptorSetCount(1)
	.setPSetLayouts(layouts);
	descriptor_set = device.allocateDescriptorSets(alloc_info)[0];

	auto buffer_info = vk::DescriptorBufferInfo()
	.setBuffer(uniform_buffer)
	.setOffset(0)
	.setRange(sizeof(UniformBufferObject));

	auto descriptor_write = vk::WriteDescriptorSet()
	.setDstSet(descriptor_set)
	.setDstBinding(0)
	.setDstArrayElement(0)
	.setDescriptorType(vk::DescriptorType::eUniformBuffer)
	.setDescriptorCount(1)
	.setPBufferInfo(&buffer_info)
	.setPImageInfo(nullptr)
	.setPTexelBufferView(nullptr);
	device.updateDescriptorSets({ descriptor_write }, {});
}
