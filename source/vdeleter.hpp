#include <vulkan/vulkan.hpp>
#include <functional>

VkResult CreateDebugReportCallbackEXT(
	VkInstance instance,
	const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugReportCallbackEXT* pCallback
);

void DestroyDebugReportCallbackEXT(
	VkInstance instance,
	VkDebugReportCallbackEXT callback,
	const VkAllocationCallbacks* pAllocator
);

template <typename T>
class VDeleter {
public:
	VDeleter() : VDeleter([](T, VkAllocationCallbacks*) {}) {}
	//VDeleter() : VDeleter([](T, vk::AllocationCallbacks*) {}) {}

	VDeleter(std::function<void(T, VkAllocationCallbacks*)> deletef) {
	//VDeleter(std::function<void(T, vk::AllocationCallbacks*)> deletef) {
		this->deleter = [=](T obj) { deletef(obj, nullptr); };
	}

	VDeleter(const VDeleter<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&instance, deletef](T obj) { deletef(instance, obj, nullptr); };
	}

	VDeleter(const VDeleter<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		this->deleter = [&device, deletef](T obj) { deletef(device, obj, nullptr); };
	}

	~VDeleter() {
		cleanup();
	}

	const T* operator &() const {
		return &object;
	}

	T* replace() {
		cleanup();
		return &object;
	}

	operator T() const {
		return object;
	}

	void operator=(T rhs) {
		cleanup();
		object = rhs;
	}

	template<typename V>
	bool operator==(V rhs) {
		return object == T(rhs);
    	}

private:
	T object{ VK_NULL_HANDLE };
	std::function<void(T)> deleter;

	void cleanup() {
    		if (object != VK_NULL_HANDLE) { deleter(object); }
		object = VK_NULL_HANDLE;
	}
};
