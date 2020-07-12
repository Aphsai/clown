#include <array>
#include <chrono>
#include <iostream>
#include <cmath>

#include "renderer.hpp"
#include "window.hpp"

constexpr double PI				= 3.14159265358979323846;
constexpr double CIRCLE_RAD		= PI * 2;
constexpr double CIRCLE_THIRD	= CIRCLE_RAD / 3.0;
constexpr double CIRCLE_THIRD_1	= 0;
constexpr double CIRCLE_THIRD_2	= CIRCLE_THIRD;
constexpr double CIRCLE_THIRD_3	= CIRCLE_THIRD * 2;

int main()
{
	Renderer r;

	auto w = r.openWindow( 800, 600, "Vulkan API Tutorial 12" );

	VkCommandPool command_pool			= VK_NULL_HANDLE;
	VkCommandPoolCreateInfo pool_create_info {};
	pool_create_info.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.flags				= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_create_info.queueFamilyIndex	= r.graphics_family_index;
	vkCreateCommandPool(r.device, &pool_create_info, nullptr, &command_pool);

	VkCommandBuffer command_buffer					= VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo	command_buffer_allocate_info {};
	command_buffer_allocate_info.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool		= command_pool;
	command_buffer_allocate_info.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount	= 1;
	vkAllocateCommandBuffers(r.device, &command_buffer_allocate_info, &command_buffer);

	VkSemaphore render_complete_semaphore	= VK_NULL_HANDLE;
	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType				= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore( r.device, &semaphore_create_info, nullptr, &render_complete_semaphore );

	float color_rotator		= 0.0f;
	auto timer				= std::chrono::steady_clock();
	auto last_time			= timer.now();
	uint64_t frame_counter	= 0;
	uint64_t fps			= 0;

	while( r.run() ) {
		// CPU logic calculations

		++frame_counter;
		if(last_time + std::chrono::seconds(1) < timer.now() ) {
			last_time		= timer.now();
			fps				= frame_counter;
			frame_counter	= 0;
			std::cout << "FPS: " << fps << std::endl;
		}

		// Begin render
		w->beginRender();
		// Record command buffer
		VkCommandBufferBeginInfo command_buffer_begin_info {};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

		VkRect2D render_area {};
		render_area.offset.x = 0;
		render_area.offset.y = 0;
		render_area.extent = VkExtent2D { w->surface_size_x, w->surface_size_y };

		color_rotator += 0.001;

		std::array<VkClearValue, 2> clear_values {};
		clear_values[ 0 ].depthStencil.depth		= 0.0f;
		clear_values[ 0 ].depthStencil.stencil		= 0;
		clear_values[ 1 ].color.float32[ 0 ]		= std::sin( color_rotator + CIRCLE_THIRD_1 ) * 0.5 + 0.5;
		clear_values[ 1 ].color.float32[ 1 ]		= std::sin( color_rotator + CIRCLE_THIRD_2 ) * 0.5 + 0.5;
		clear_values[ 1 ].color.float32[ 2 ]		= std::sin( color_rotator + CIRCLE_THIRD_3 ) * 0.5 + 0.5;
		clear_values[ 1 ].color.float32[ 3 ]		= 1.0f;

		VkRenderPassBeginInfo render_pass_begin_info {};
		render_pass_begin_info.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass			= w->render_pass;
		render_pass_begin_info.framebuffer			= w->framebuffers[w->active_swapchain_image_id];
		render_pass_begin_info.renderArea			= render_area;
		render_pass_begin_info.clearValueCount		= clear_values.size();
		render_pass_begin_info.pClearValues			= clear_values.data();

		vkCmdBeginRenderPass( command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );

		vkCmdEndRenderPass( command_buffer );

		vkEndCommandBuffer( command_buffer );

		// Submit command buffer
		VkSubmitInfo submit_info {};
		submit_info.sType					= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount		= 0;
		submit_info.pWaitSemaphores			= nullptr;
		submit_info.pWaitDstStageMask		= nullptr;
		submit_info.commandBufferCount		= 1;
		submit_info.pCommandBuffers			= &command_buffer;
		submit_info.signalSemaphoreCount	= 1;
		submit_info.pSignalSemaphores		= &render_complete_semaphore;

		vkQueueSubmit( r.queue, 1, &submit_info, VK_NULL_HANDLE );

		// End render
		w->endRender( { render_complete_semaphore } );
	}

	vkQueueWaitIdle(r.queue );

	vkDestroySemaphore(r.device, render_complete_semaphore, nullptr );
	vkDestroyCommandPool(r.device, command_pool, nullptr );

	return 0;
}
