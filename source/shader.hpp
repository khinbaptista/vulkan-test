#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

using byte = char;

class Shader {
protected:
	vk::ShaderModule _module;
	void CreateModule(const std::vector<byte> &code);

public:
	Shader();
	Shader(const std::string& filename);

	void LoadSourceFile(const std::string& filename);

	vk::ShaderModule module();
	void DestroyModule();
};

std::vector<byte> ReadFile(const std::string& filename);
