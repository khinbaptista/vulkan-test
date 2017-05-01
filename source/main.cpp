#include "vk_app.hpp"

#include <iostream>
#include <stdexcept>

int main() {
	VkApp app("Vulkan");

	try {
		app.Run();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return 0;
	}

	#ifdef _WIN32

		// Don't close the console immediately
		getchar();

	#endif // _WIN32

	return 0;
}
