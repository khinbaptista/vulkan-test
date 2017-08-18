#pragma once

#include <vulkan/vulkan.hpp>
#include "viewport.hpp"

class Material {
public:
	vk::RenderPass		render_pass;
	vk::PipelineLayout	pipeline_layout;
	vk::Pipeline		graphics_pipeline;

	vk::Format color_attachment_format;
	vk::PrimitiveTopology topology;

	void Dispose(vk::Device);

	void CreateRenderpass();
	void CreateGraphicsPipeline(Viewport&);
};
