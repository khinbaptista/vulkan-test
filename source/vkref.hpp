#include <vulkan/vulkan.hpp>
#include <functional>

using std::function;

template< typename T >
class vkref {
private:
	T object;
	function<void(T)> deleter;

	void cleanup() {
		if (object) { deleter(object); }
		object = nullptr;
	}

public:
	vkref() : vkref([](T, VkAllocationCallbacks*) {}) {}

	vkref(
		function<void(T, VkAllocationCallbacks*)> deletef,
		VkAllocationCallbacks* callbacks = nullptr
	) {
		this->deleter = [=](T obj) { deletef(obj, callbacks); };
	}

	vkref(
		const vkref<vk::Instance>& instance,
		function<void(VkInstance, T, VkAllocationCallbacks*)> deletef,
		VkAllocationCallbacks* callbacks = nullptr
	) {
		this->deleter = [&instance, deletef, callbacks](T obj) {
			deletef(instance, obj, callbacks);
		};
	}

	vkref(
		const vkref<vk::Device>& device,
		function<void(VkDevice, T, VkAllocationCallbacks*)> deletef,
		VkAllocationCallbacks* callbacks = nullptr
	) {
		this->deleter = [&device, deletef, callbacks](T obj) {
			deletef(device, obj, callbacks);
		};
	}

	~vkref() { cleanup(); }

	const T* operator &() const { return &object; }

	T* replace() {
		cleanup();
		return &object;
	}

	operator T() const { return object; }

	void operator =(T other) {
		if (other != object) {
			cleanup();
			object = other;
		}
	}

	template< typename V >
	bool operator ==(V other) { return object == T(other); }
};
