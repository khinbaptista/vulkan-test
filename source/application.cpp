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
using std::shared_ptr;

Application* Application::singleton = nullptr;

Application::Application(string title, bool validate) {
	window = std::make_shared<Window>(title);
	enable_validation_layers = validate;
	singleton = this;
}

Application::~Application() {
	device.destroyPipeline(graphics_pipeline);
	device.destroyPipelineLayout(pipeline_layout);
	device.destroyRenderPass(render_pass);

	viewport.DestroySwapchain();

	{	// Delete debug report callback object
		auto destroy_callback_func = (PFN_vkDestroyDebugReportCallbackEXT)
			instance.getProcAddr("vkDestroyDebugReportCallbackEXT");

		if (destroy_callback_func != nullptr) {
			destroy_callback_func(instance, callback, nullptr);
		}
	}

	viewport.DestroyImageViews();

	// Destroy logical device
	device.destroy();

	// Destroy window surface and vulkan instance
	instance.destroySurfaceKHR(surface);
	instance.destroy();

	glfwTerminate();
}

Application*			Application::get_singleton()	{ return singleton; }
shared_ptr<Window>		Application::get_window()		{ return singleton->window; }
vk::Device				Application::get_device()		{ return singleton->device; }

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
	viewport = Viewport(physical_device, surface, window->width(), window->height());
	CreateRenderPass();
	CreateGraphicsPipeline();
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

bool Application::CheckDeviceExtensionSupport(vk::PhysicalDevice device) {
	vector<vk::ExtensionProperties> available_extensions;
	available_extensions = device.enumerateDeviceExtensionProperties();

	set<string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
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
	bool extensions_supported	= CheckDeviceExtensions(device);

	bool swapchain_adequate = false;
	if (extensions_supported) {
		SwapchainSupportDetails swapchain_support = Swapchain::QuerySupport(device, surface);
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
		device_extensions.begin(), device_extensions.end()
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

void Application::CreateRenderPass() {
	// How many color and depth buffers will there be?
	// How many samples to use for each of them and how should
	// their contents be handled throughout the rendering operations?

	auto color_attachment = vk::AttachmentDescription()
	.setFormat(viewport.swapchain().format())
	.setSamples(vk::SampleCountFlagBits::e1)
	.setLoadOp(vk::AttachmentLoadOp::eClear)
	.setStoreOp(vk::AttachmentStoreOp::eStore)
	.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)	// not using stencil
	.setInitialLayout(vk::ImageLayout::eUndefined)
	.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	auto color_attachment_ref = vk::AttachmentReference()
	.setAttachment(0)	// index of our (single) color attachment
	.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	auto subpass = vk::SubpassDescription()
	.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
	.setColorAttachmentCount(1)
	.setPColorAttachments(&color_attachment_ref);
	// the index of the attachment in this array is directly
	// referenced from the fragment shader with the
	// `layout(location = 0) out vec4 out_color` directive

	auto render_pass_info = vk::RenderPassCreateInfo()
	.setAttachmentCount(1)
	.setPAttachments(&color_attachment)
	.setSubpassCount(1)
	.setPSubpasses(&subpass);
	render_pass = device.createRenderPass(render_pass_info);
}

void Application::CreateGraphicsPipeline() {
	vertex_shader.LoadSourceFile("shaders/shader-v.spv");
	fragment_shader.LoadSourceFile("shaders/shader-f.spv");

	auto vertex_stage_info = vk::PipelineShaderStageCreateInfo()
	.setStage(vk::ShaderStageFlagBits::eVertex)
	.setModule(vertex_shader.module())
	.setPName("main");

	auto fragment_stage_info = vk::PipelineShaderStageCreateInfo()
	.setStage(vk::ShaderStageFlagBits::eFragment)
	.setModule(fragment_shader.module())
	.setPName("main");

	vk::PipelineShaderStageCreateInfo shader_stages[] = {
		vertex_stage_info, fragment_stage_info
	};

	auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo()
	.setVertexBindingDescriptionCount(0)	// shaders have hard-coded vertex info for now
	.setPVertexBindingDescriptions(nullptr)
	.setVertexAttributeDescriptionCount(0)
	.setPVertexAttributeDescriptions(nullptr);

	// Material?
	auto input_assembly_info = vk::PipelineInputAssemblyStateCreateInfo()
	.setTopology(vk::PrimitiveTopology::eTriangleList)
	.setPrimitiveRestartEnable(false);

	auto viewport_state_info = vk::PipelineViewportStateCreateInfo()
	.setViewportCount(1)
	.setPViewports(&viewport.vk())
	.setScissorCount(1)
	.setPScissors(&viewport.scissor);

	auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo()
	.setDepthClampEnable(false) 		// useful for shadow maps
	.setRasterizerDiscardEnable(false)	// disables output to the framebuffer
	.setPolygonMode(vk::PolygonMode::eFill)	// other modes require GPU features
	.setLineWidth(1.0f)
	.setCullMode(vk::CullModeFlagBits::eBack)
	.setFrontFace(vk::FrontFace::eClockwise)
	.setDepthBiasEnable(false)			// useful for shadow maps, maybe
	.setDepthBiasConstantFactor(0.0f)
	.setDepthBiasClamp(0.0f)
	.setDepthBiasSlopeFactor(0.0f);

	auto multisampling_info = vk::PipelineMultisampleStateCreateInfo()
	.setSampleShadingEnable(false)
	.setRasterizationSamples(vk::SampleCountFlagBits::e1)
	.setMinSampleShading(1.0f)
	.setPSampleMask(nullptr)
	.setAlphaToCoverageEnable(false)
	.setAlphaToOneEnable(false);

	// Depth buffer?
	// ...

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
	.setColorWriteMask(
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	)
	.setBlendEnable(false)
	.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)		// Alpha blending
	.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
	.setColorBlendOp(vk::BlendOp::eAdd)
	.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
	.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
	.setAlphaBlendOp(vk::BlendOp::eAdd);

	auto color_blending_info = vk::PipelineColorBlendStateCreateInfo()
	.setLogicOpEnable(false)
	.setLogicOp(vk::LogicOp::eCopy)
	.setAttachmentCount(1)
	.setPAttachments(&color_blend_attachment)
	.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	// Pass uniforms and push constants to shaders
	auto pipeline_layout_info = vk::PipelineLayoutCreateInfo()
	.setSetLayoutCount(0)
	.setPSetLayouts(nullptr)
	.setPushConstantRangeCount(0)
	.setPPushConstantRanges(nullptr);
	pipeline_layout = device.createPipelineLayout(pipeline_layout_info);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
	.setStageCount(2)
	.setPStages(shader_stages)
	.setPVertexInputState(&vertex_input_info)
	.setPInputAssemblyState(&input_assembly_info)
	.setPViewportState(&viewport_state_info)
	.setPRasterizationState(&rasterizer_info)
	.setPMultisampleState(&multisampling_info)
	.setPDepthStencilState(nullptr)
	.setPColorBlendState(&color_blending_info)
	.setPDynamicState(nullptr)
	.setLayout(pipeline_layout)
	.setRenderPass(render_pass)
	.setSubpass(0)
	.setBasePipelineHandle(nullptr)
	.setBasePipelineIndex(-1);
	graphics_pipeline = device.createGraphicsPipeline(nullptr, pipeline_info);

	vertex_shader.DestroyModule();
	fragment_shader.DestroyModule();
}
