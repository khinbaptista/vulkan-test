#include "material.hpp"

#include "application.hpp"
#include "shader.hpp"

void Material::Dispose(vk::Device device) {
	device.destroyPipeline(graphics_pipeline);
	device.destroyPipelineLayout(pipeline_layout);
	device.destroyRenderPass(render_pass);
}

void Material::CreateRenderpass() {
	// How many color and depth buffers will there be?
	// How many samples to use for each of them and how should
	// their contents be handled throughout the rendering operations?
	auto device = Application::get_device();

	auto color_attachment = vk::AttachmentDescription()
	.setFormat(color_attachment_format)
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

void Material::CreateGraphicsPipeline(Viewport& viewport) {
	auto device = Application::get_device();

	Shader vertex_shader("shaders/shader-v.spv");
	Shader fragment_shader("shaders/shader-f.spv");

	auto vertex_stage_info = vk::PipelineShaderStageCreateInfo()
	.setStage(vk::ShaderStageFlagBits::eVertex)
	.setModule(vertex_shader.module)
	.setPName("main");

	auto fragment_stage_info = vk::PipelineShaderStageCreateInfo()
	.setStage(vk::ShaderStageFlagBits::eFragment)
	.setModule(fragment_shader.module)
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
}
