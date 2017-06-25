#include "window.hpp"

using std::string;

Window::~Window() {
	glfwDestroyWindow(_window);
}

Window::Window(string t, uint32_t w, uint32_t h)
:	_title(t)
,	_width(w)
,	_height(h)
{ }

void Window::Initialize() {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	_window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
}

string		Window::title()		{ return _title;	}
uint32_t	Window::width()		{ return _width;	}
uint32_t	Window::height()	{ return _height;	}
GLFWwindow*	Window::glfw()		{ return _window;	}

VkResult Window::CreateSurface(const vk::Instance &instance, vk::SurfaceKHR *surface) {
	return glfwCreateWindowSurface(instance, _window, nullptr, reinterpret_cast<VkSurfaceKHR_T**>(surface));
}

bool Window::should_close() { return glfwWindowShouldClose(_window); }
