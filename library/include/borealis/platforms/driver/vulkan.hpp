/*
    Copyright 2024 Nika

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include <volk.h>

#include <vector>

#ifdef __GLFW__
#include <GLFW/glfw3.h>
#elif defined(__SDL3__)
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#endif

#include <nanovg_vk.h>

namespace brls
{

class VulkanContext
{
  public:
#ifdef __GLFW__
    VulkanContext(GLFWwindow* window, int width, int height);
#elif defined(__SDL3__)
    VulkanContext(SDL_Window* window, int width, int height);
#endif
    ~VulkanContext();

    double getScaleFactor();
    void clear(NVGcolor color);
    void beginFrame();
    void endFrame();
    void setSwapInterval(int interval);
    bool onFramebufferSize(int width, int height);

    VKNVGCreateInfo createVKInfo();

    VkPhysicalDevice getPhysicalDevice() { return m_physicalDevice; }
    VkDevice getDevice() { return m_device; }

    VkQueue getGraphicsQueue() { return m_graphicsQueue; }

  private:
    VkInstance m_instance             = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device                 = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface            = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue             = VK_NULL_HANDLE;
    VkQueue m_presentQueue              = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamilyIndex = 0;
    uint32_t m_presentQueueFamilyIndex  = 0;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    uint32_t m_swapchainImageCount = 0;
    VkFormat m_swapchainFormat     = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D m_swapchainExtent   = {};

    VkImage m_depthImage         = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat       = VK_FORMAT_D24_UNORM_S8_UINT;

    VkRenderPass m_renderPass           = VK_NULL_HANDLE;
    VkCommandPool m_commandPool         = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffers[3] = {};
    VkCommandBuffer m_uploadCmdBuffer   = VK_NULL_HANDLE;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    uint32_t m_currentFrame      = 0;
    uint32_t m_currentImageIndex = 0;
    bool m_framebufferResized    = false;
    bool m_frameStarted          = false;

    int m_swapInterval             = 1;
    VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkNvgExt m_nvgExt          = {};
    bool m_enableDynamicState  = false;
    bool m_enableDynamicState3 = false;

    int m_framebufferWidth  = 0;
    int m_framebufferHeight = 0;

    NVGcolor m_clearColor = {};

    VkPhysicalDeviceMemoryProperties m_memoryProperties = {};

#ifdef __GLFW__
    GLFWwindow* m_window = nullptr;
#elif defined(__SDL3__)
    SDL_Window* m_window = nullptr;
#endif

    bool createInstance();
    bool createSurface();
    bool selectPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    bool createImageViews();
    bool createDepthResources();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    void cleanupSwapchain();
    void recreateSwapchain();
    VkPresentModeKHR choosePresentMode();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

} // namespace brls
