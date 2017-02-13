//#include "vk_application.hpp"
#include "vk_app.hpp"

#include <iostream>
#include <stdexcept>

int main() {
	//VulkanApp app;
	VkApp app("Vulkan-HPP");

	try {
		app.Run();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return 0;
	}

	return 0;
}
