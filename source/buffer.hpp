#pragma once

#include <vulkan/vulkan.hpp>

class Buffer {
public:
	~Buffer();
	Buffer();

	Buffer(
		vk::Device device,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage
	);

	void BindMemory(const vk::PhysicalDevice&, vk::MemoryPropertyFlags);
	vk::Buffer&			get_buffer();
	vk::DeviceMemory&	get_memory();
	vk::DeviceSize&		get_size();

	Buffer& operator = (const Buffer&);
	operator vk::Buffer&();

protected:
	vk::Device			_device;
	vk::Buffer			_buffer;
	vk::DeviceMemory	_memory;
	vk::DeviceSize		_size;

	uint32_t FindMemoryType(
		const vk::PhysicalDevice& physical_device,
		uint32_t filter,
		vk::MemoryPropertyFlags properties
	);
};
