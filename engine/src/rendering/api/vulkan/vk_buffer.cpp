#include "pch.h"
#include "vk_buffer.h"

#include "vk_renderer.h"

namespace engine {
    VkDeviceSize pad_uniform_buffer_size(VkDeviceSize original_size)
    {
        const vk_context& context = get_vulkan_context();

        // Get GPU minimum uniform buffer alignment
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(context.device->gpu, &properties);

        VkDeviceSize minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;

        if (minUboAlignment < 0)
            return original_size;

        // TODO: Figure out how this works
        return (original_size + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    Vk_Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags type)
    {
        Vk_Buffer buffer{};

        const vk_context& rc = get_vulkan_context();

        VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_info.size = size;
        buffer_info.usage = type;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        vk_check(vmaCreateBuffer(rc.allocator,
            &buffer_info,
            &alloc_info,
            &buffer.buffer,
            &buffer.allocation,
            nullptr));

        buffer.usage = buffer_info.usage;
        buffer.size = u32(buffer_info.size);

        return buffer;
    }

    Vk_Buffer create_uniform_buffer(VkDeviceSize buffer_size)
    {
        // Automatically align buffer to correct alignment
        const VkDeviceSize size = static_cast<VkDeviceSize>(frames_in_flight) * pad_uniform_buffer_size(buffer_size);

        Vk_Buffer buffer = create_buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        return buffer;
    }

    std::vector<Vk_Buffer> create_uniform_buffers(VkDeviceSize buffer_size)
    {
        std::vector<Vk_Buffer> buffers(frames_in_flight);

        for (Vk_Buffer& buffer : buffers) {
            buffer = create_uniform_buffer(buffer_size);
        }

        return buffers;
    }

    // Maps/Fills an existing buffer with data.
    void set_buffer_data(std::vector<Vk_Buffer>& buffers, void* data)
    {
        const vk_context& rc = get_vulkan_context();

        const uint32_t current_frame = get_frame_buffer_index();

        void* allocation{};
        vk_check(vmaMapMemory(rc.allocator, buffers[current_frame].allocation, &allocation));
        std::memcpy(allocation, data, buffers[current_frame].size);
        vmaUnmapMemory(rc.allocator, buffers[current_frame].allocation);
    }

    void set_buffer_data(Vk_Buffer& buffer, void* data)
    {
        const vk_context& rc = get_vulkan_context();

        void* allocation{};
        vk_check(vmaMapMemory(rc.allocator, buffer.allocation, &allocation));
        std::memcpy(allocation, data, buffer.size);
        vmaUnmapMemory(rc.allocator, buffer.allocation);
    }


    void set_buffer_data(Vk_Buffer& buffer, void* data, std::size_t size)
    {
        const vk_context& rc = get_vulkan_context();
        const uint32_t current_frame = get_frame_buffer_index();

        char* allocation{};
        vk_check(vmaMapMemory(rc.allocator, buffer.allocation, (void**)&allocation));
        allocation += pad_uniform_buffer_size(size) * current_frame;
        std::memcpy(allocation, data, size);
        vmaUnmapMemory(rc.allocator, buffer.allocation);
    }

    // Creates and fills a buffer that is CPU accessible. A staging
    // buffer is most often used as a temporary buffer when copying
    // data from the CPU to the GPU.
    Vk_Buffer create_staging_buffer(void* data, VkDeviceSize size)
    {
        Vk_Buffer buffer{};

        const vk_context& rc = get_vulkan_context();

        VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vk_check(vmaCreateBuffer(rc.allocator,
            &buffer_info,
            &alloc_info,
            &buffer.buffer,
            &buffer.allocation,
            nullptr));

        buffer.usage = buffer_info.usage;
        buffer.size = buffer_info.size;

        set_buffer_data(buffer, data);

        return buffer;
    }

    // Creates an empty buffer on the GPU that will need to be filled by
    // calling SubmitToGPU.
    Vk_Buffer create_gpu_buffer(VkDeviceSize size, VkBufferUsageFlags type)
    {
        Vk_Buffer buffer{};

        const vk_context& rc = get_vulkan_context();

        VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        buffer_info.size = size;
        buffer_info.usage = type | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

        vk_check(vmaCreateBuffer(rc.allocator,
            &buffer_info,
            &alloc_info,
            &buffer.buffer,
            &buffer.allocation,
            nullptr));

        buffer.usage = buffer_info.usage;
        buffer.size = buffer_info.size;

        return buffer;
    }

    void destroy_buffer(Vk_Buffer& buffer)
    {
        const vk_context& rc = get_vulkan_context();

        vmaDestroyBuffer(rc.allocator, buffer.buffer, buffer.allocation);
    }

    void destroy_buffers(std::vector<Vk_Buffer>& buffers)
    {
        const vk_context& rc = get_vulkan_context();

        for (Vk_Buffer& buffer : buffers)
            vmaDestroyBuffer(rc.allocator, buffer.buffer, buffer.allocation);
    }

}
