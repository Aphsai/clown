#include <array>
#include <chrono>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>

#include "renderer.hpp"
#include "window.hpp"
#include "shared.hpp"
#include "vertex.hpp"



int main() {
	Renderer r;
	auto w = r.openWindow( 800, 600, "CLOWN" );
    r.initialize();
    
    VkSemaphore image_available_semaphore = VK_NULL_HANDLE;
	VkSemaphore render_complete_semaphore = VK_NULL_HANDLE;
	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore( r.device, &semaphore_create_info, nullptr, &render_complete_semaphore );
	vkCreateSemaphore( r.device, &semaphore_create_info, nullptr, &image_available_semaphore );


	while(r.run()) {

		r.beginRender();

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore wait_semaphores [] = { image_available_semaphore }; 
        VkPipelineStageFlags wait_stages [] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &(r.command_buffers[r.active_swapchain_image_id]);
        
        VkSemaphore signal_semaphores[] = { render_complete_semaphore };
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        errorCheck(vkQueueSubmit(r.queue, 1, &submit_info, VK_NULL_HANDLE));

		r.endRender({ render_complete_semaphore });

	}

	vkQueueWaitIdle(r.queue);
    r.destroy();

	vkDestroySemaphore(r.device, render_complete_semaphore, nullptr );
	vkDestroySemaphore(r.device, image_available_semaphore, nullptr );

	return 0;
}
