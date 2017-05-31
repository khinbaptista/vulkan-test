#### Application
- Instance
	- Application info
	- Extensions
- Validation layers
	- Message callback
- Physical device
	- Properties
	- Features
	- Supported family queues
- Logical device
	- Queues
	- Device features

#### Presentation
 Optional, if off-screen rendering is desired.

- **Window surface**
	- `VK_KHR_surface` extension and `vk::SurfaceKHR` object
	- Platform-dependent
	- Presentation queue
- **Swapchain**
	- **Description:**
	Infrastructure that provide images to render to; these images can later be presented to the screen.
	- Swapchain support
	- Swapchain support details
	- Physical device surface capabilities
	- Physical device surface format
	- Physical device surface present modes
	- Swapchain extent
	- Retrieving the swapchain images
- Image views
	- **Description:**
	An image view describes how to access the image and which part of the image to access.

#### Graphics pipeline
Graphics pipelines in Vulkan are almost immutable. This means that if you want
to change any given setting, you are not able to do so by just calling a function
or setting a member: you need to recreate the pipeline object;
Some pipeline stages can be disabled for performance (eg, disable fragment shader
stage for shadow map generation).
The graphics pipeline object will be required in a per-material basis.

- Shader modules
	- Compiling shaders
	- Loading shaders
	- Shader module objects
	- Shader stage objects
- Fixed functions
	- You have to be explicit about everything, from viewport size to color blending function.
	- Vertex input
	- Input assembly (primitive type)
	- Rasterizer
		- Polygon mode (line, fill, point)
		- Line width
		- Cull mode and front face
	- Multisampling
	- Depth and Stencil testing
	- Color blending
	- Dynamic state: allows the dynamic change of some parameters such as
	viewport size, line width and blend constants.
	- Pipeline layout
- Render passes
	- Specifies how many color and depth buffers there will be, how many samples
	to use for each of them and how their contents should be handled
	- Attachment description
	- Subpasses: A render pass can consist of multiple subpasses which are
	subsequent rendering operations that depend on the contents of framebuffers
	in previous passes (e.g., a sequence of post-processing effects).
	- Attachment references: used by subpasses to access the attachments.

#### Drawing
- **Framebuffer:**
The attachments specified during render pass creation are bound by wrapping them
into a `vk::Framebuffer` object, which references all of the `vk::ImageView`
objects that represent the attachments.
- **Command buffers:**
Commands like drawing operations and memory transfers are not executed directly
using function calls - all the operation are recorded in command buffer objects.
This allows for the setting up of drawing commands in advance and in multiple
threads, and just telling Vulkan to execute them in the main loop.
	- Command pools: memory management based on usage.
	- Command buffer allocation
	- Command buffer recording, starting the render pass and drawing commands
- Rendering and presentation
	- Synchronization: semaphores and fences
	- Acquiring an image from the swapchain
	- Submitting the command buffer
	- Subpass dependencies
	- Presentation: finally, the image is rendered to screen.
	- Memory leak

There's more yet to go: vertex buffers, uniform buffers, texture mapping
and depth buffering, but this is enough to go on this week.
