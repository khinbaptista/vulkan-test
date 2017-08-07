#include "shader.hpp"
#include "application.hpp"

#include <fstream>

using byte = char;
using std::vector;
using std::string;

Shader::Shader() {}

Shader::Shader(const string& filename) {
	LoadSourceFile(filename);
}

void Shader::LoadSourceFile(const string &filename) {
	vector<byte> code = ReadFile(filename);
	CreateModule(code);
}

void Shader::CreateModule(const vector<byte> &code) {
	auto module_info = vk::ShaderModuleCreateInfo()
	.setCodeSize(code.size())
	.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

	_module = Application::get_device().createShaderModule(module_info);
}

void Shader::DestroyModule() {
	Application::get_device().destroyShaderModule(_module);
}

vk::ShaderModule Shader::module() { return _module; }

vector<byte> ReadFile(const string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file " + filename);
	}

	size_t file_size = file.tellg();
	vector<byte> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), file_size);

	file.close();
	return buffer;
}
