#include "vk_application.hpp"

#include <iostream>
#include <stdexcept>

int main() {
	VulkanApp app;

	try {
		app.Run();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
