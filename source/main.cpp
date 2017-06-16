#include "application.hpp"

#include <iostream>
#include <stdexcept>

int main() {
	Application app("Vulkan"
		#ifndef NDEBUG
			, true
		#endif
	);

	try {
		app.Run();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return 0;
	}

	#ifdef _WIN32
		// Don't close the console immediately
		getchar();
	#endif // _WIN32*/

	return 0;
}
