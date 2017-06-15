#include "buffer.hpp"

Buffer::~Buffer() {
	_device.destroyBuffer(_buffer);
	_device.freeMemory(_memory);
}

Buffer::Buffer()
 	: _device(nullptr)
	, _size(0)
{ }

Buffer::Buffer(
	vk::Device device,
	vk::DeviceSize size,
	vk::BufferUsageFlags usage
)
	: _device(device)
	, _size(size)
{
	auto buffer_info = vk::BufferCreateInfo()
	.setSize(_size)
	.setUsage(usage)
	.setSharingMode(vk::SharingMode::eExclusive);
	_buffer = _device.createBuffer(buffer_info);
}

void Buffer::BindMemory(
	const vk::PhysicalDevice& physical_device,
	vk::MemoryPropertyFlags properties
) {
	vk::MemoryRequirements requirements;
	requirements = _device.getBufferMemoryRequirements(_buffer);

	auto alloc_info = vk::MemoryAllocateInfo()
	.setAllocationSize(requirements.size)
	.setMemoryTypeIndex(
		FindMemoryType(physical_device, requirements.memoryTypeBits, properties)
	);
	_memory = _device.allocateMemory(alloc_info);

	_device.bindBufferMemory(_buffer, _memory, 0);
}

uint32_t Buffer::FindMemoryType(
	const vk::PhysicalDevice& physical_device,
	uint32_t filter,
	vk::MemoryPropertyFlags properties
) {
	vk::PhysicalDeviceMemoryProperties mem_properties;
	mem_properties = physical_device.getMemoryProperties();

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if (
			filter & ((uint32_t)(1 << i)) &&
			properties == ((mem_properties.memoryTypes[i].propertyFlags & properties))
		) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
	return 0;
}

vk::Buffer&			Buffer::get_buffer()	{ return _buffer;	}
vk::DeviceMemory&	Buffer::get_memory()	{ return _memory;	}
vk::DeviceSize&		Buffer::get_size()		{ return _size;		}

Buffer::operator vk::Buffer&()	{ return _buffer;	}

Buffer& Buffer::operator = (const Buffer& other) {
	_device	= other._device;
	_buffer	= other._buffer;
	_memory	= other._memory;
	_size	= other._size;
	return *this;
}
