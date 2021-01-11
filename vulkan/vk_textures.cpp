#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "vk_init.hpp"
#include "vk_textures.hpp"

bool vk_util::load_image_from_file(VulkanEngine& engine, const char* file, AllocatedImage& out_image) {
    int tex_width;
    int tex_height;
    int tex_channels;

    stbi_uc* pixels = stbi_load(file, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "Failed to load texture file : " << file << std::endl;
        return false;
    }

    void* pixel_ptr = pixels;
    VkDeviceSize image_size = tex_width * tex_height * 4;
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    AllocatedBuffer staging_buffer = engine.create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(engine._allocator, staging_buffer._allocation, &data);
    memcpy(data, pixel_ptr, static_cast<size_t>(image_size));
    vmaUnmapMemory(engine._allocator, staging_buffer._allocation);

    stbi_image_free(pixels);

    VkExtent3D image_extent;
    image_extent.width = static_cast<uint32_t> (tex_height);
    image_extent.height = static_cast<uint32_t> (tex_height);
    image_extent.depth = 1;

    VkImageCreateInfo d_img_info = vk_init::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_extent);

    AllocatedImage new_image;
    VmaAllocationCreateInfo d_img_alloc_info {};
    d_img_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(engine._allocator, &d_img_info, &d_img_alloc_info, &new_image._image, &new_image._allocation, nullptr);

    engine.immediate_submit(
            [&](VkCommandBuffer cmd) {
                VkImageSubresourceRange range;
                range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                range.baseMipLevel = 0;
                range.levelCount = 1;
                range.baseArrayLayer = 0;
                range.layerCount = 1;

                VkImageMemoryBarrier image_barrier_to_transfer {};
                image_barrier_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                image_barrier_to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_barrier_to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_barrier_to_transfer.image = new_image._image;
                image_barrier_to_transfer.subresourceRange = range;
                image_barrier_to_transfer.srcAccessMask = 0;
                image_barrier_to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_transfer);

                VkBufferImageCopy copy_region {};
                copy_region.bufferOffset = 0;
                copy_region.bufferRowLength = 0;
                copy_region.bufferImageHeight = 0;
                copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copy_region.imageSubresource.mipLevel = 0;
                copy_region.imageSubresource.baseArrayLayer = 0;
                copy_region.imageSubresource.layerCount = 1;
                copy_region.imageExtent = image_extent;

                vkCmdCopyBufferToImage(cmd, staging_buffer._buffer, new_image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

                VkImageMemoryBarrier image_barrier_to_readable = image_barrier_to_transfer;
                image_barrier_to_readable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_barrier_to_readable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_barrier_to_readable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                image_barrier_to_readable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_readable);
            }
    );

    engine._main_deletion_queue.push_function(
            [=]() {
                vmaDestroyImage(engine._allocator, new_image._image, new_image._allocation);
            }
    );
    
    vmaDestroyBuffer(engine._allocator, staging_buffer._buffer, staging_buffer._allocation);
    std::cout << "Texture loaded successfully " << file << std::endl;

    out_image = new_image;
    return true;
}
