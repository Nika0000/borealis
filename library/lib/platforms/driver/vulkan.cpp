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

#include <algorithm>
#include <borealis/core/logger.hpp>
#include <borealis/platforms/driver/vulkan.hpp>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef __GLFW__
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#endif

namespace brls
{

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

[[maybe_unused]]
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        Logger::error("Vulkan: {}", pCallbackData->pMessage);
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        Logger::warning("Vulkan: {}", pCallbackData->pMessage);
    return VK_FALSE;
}

#ifdef __GLFW__
VulkanContext::VulkanContext(GLFWwindow* window, int width, int height)
#elif defined(__SDL3__)
VulkanContext::VulkanContext(SDL_Window* window, int width, int height)
#endif
    : m_framebufferWidth(width), m_framebufferHeight(height), m_window(window)
{
    m_clearColor = nvgRGBAf(0.0f, 0.0f, 0.0f, 1.0f);

    if (!createInstance())
        return;
    setupDebugMessenger();
    if (!createSurface())
        return;
    if (!selectPhysicalDevice())
        return;
    if (!createLogicalDevice())
        return;
    if (!createCommandPool())
        return;
    if (!createSwapchain())
        return;
    if (!createImageViews())
        return;
    if (!createDepthResources())
        return;
    if (!createCommandBuffers())
        return;
    if (!createSyncObjects())
        return;

    Logger::info("Vulkan: initialized successfully");
}

VulkanContext::~VulkanContext()
{
    if (m_device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(m_device);

    cleanupSwapchain();

    if (m_debugMessenger != VK_NULL_HANDLE)
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (m_imageAvailableSemaphores.size() > i && m_imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        if (m_renderFinishedSemaphores.size() > i && m_renderFinishedSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        if (m_inFlightFences.size() > i && m_inFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }

    if (m_commandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    if (m_device != VK_NULL_HANDLE)
        vkDestroyDevice(m_device, nullptr);

    if (m_surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    if (m_instance != VK_NULL_HANDLE)
        vkDestroyInstance(m_instance, nullptr);
}

bool VulkanContext::createInstance()
{
    VkResult volkRes = volkInitialize();
    if (volkRes != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to initialize volk ({})", (int) volkRes);
        return false;
    }

    VkApplicationInfo appInfo  = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName   = "Borealis";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Borealis";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    std::vector<const char*> extensions;

#ifdef __GLFW__
    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    for (uint32_t i = 0; i < glfwExtCount; i++)
        extensions.push_back(glfwExts[i]);
#elif defined(__SDL3__)
    uint32_t sdlExtCount       = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
    for (uint32_t i = 0; i < sdlExtCount; i++)
        extensions.push_back(sdlExts[i]);
#endif

    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    std::vector<const char*> layers;
#if defined(DEBUG) || defined(_DEBUG) || defined(__DEBUG__)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (auto& layer : availableLayers)
    {
        if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0)
        {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            break;
        }
    }
#endif

    VkInstanceCreateInfo createInfo    = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = (uint32_t) extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount       = (uint32_t) layers.size();
    createInfo.ppEnabledLayerNames     = layers.data();

    VkResult res = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create instance ({})", (int) res);
        return false;
    }

    volkLoadInstance(m_instance);
    return true;
}

void VulkanContext::setupDebugMessenger()
{
#if defined(DEBUG) || defined(_DEBUG) || defined(__DEBUG__)
    if (!vkCreateDebugUtilsMessengerEXT)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = vulkanDebugCallback;

    if (vkCreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        Logger::warning("Vulkan: failed to set up debug messenger");

    Logger::debug("Vulkan: successfully initialized debug messenger");
#endif
}

bool VulkanContext::createSurface()
{
#ifdef __GLFW__
    VkResult res = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create window surface ({})", (int) res);
        return false;
    }
#elif defined(__SDL3__)
    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface))
    {
        Logger::error("Vulkan: failed to create SDL Vulkan surface");
        return false;
    }
#endif
    return true;
}

VKNVGCreateInfo VulkanContext::createVKInfo()
{
    VKNVGCreateInfo vkCreateInfo;
    vkCreateInfo.gpu                 = m_physicalDevice;
    vkCreateInfo.device              = m_device;
    vkCreateInfo.cmdBuffer           = m_commandBuffers;
    vkCreateInfo.uploadCmdBuffer     = m_uploadCmdBuffer;
    vkCreateInfo.swapchainImageCount = MAX_FRAMES_IN_FLIGHT;
    vkCreateInfo.currentFrame        = &m_currentFrame;
    vkCreateInfo.ext                 = m_nvgExt;

    vkCreateInfo.allocator = nullptr;

    vkCreateInfo.colorFormat        = m_swapchainFormat;
    vkCreateInfo.depthStencilFormat = m_depthFormat;
    return vkCreateInfo;
}

bool VulkanContext::selectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        Logger::error("Vulkan: no GPUs with Vulkan support");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    bool foundDiscrete          = false;

    for (auto& gpu : devices)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
        if (queueFamilyCount < 1)
            continue;

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, m_surface, &presentSupport);

            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
            {
                VkPhysicalDeviceProperties props;
                vkGetPhysicalDeviceProperties(gpu, &props);

                bestDevice = gpu;
                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                    foundDiscrete = true;
                break;
            }
        }

        if (foundDiscrete)
            break;
    }

    if (bestDevice == VK_NULL_HANDLE)
    {
        Logger::error("Vulkan: no suitable GPU found");
        return false;
    }

    m_physicalDevice = bestDevice;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    Logger::info("Vulkan: GPU: {}", props.deviceName);
    Logger::info(
        "Vulkan: API version: {}.{}.{}",
        VK_VERSION_MAJOR(props.apiVersion),
        VK_VERSION_MINOR(props.apiVersion),
        VK_VERSION_PATCH(props.apiVersion)
    );

    if (props.apiVersion < VK_API_VERSION_1_3)
    {
        Logger::error(
            "Vulkan: device API version {}.{} is below the required 1.3",
            VK_VERSION_MAJOR(props.apiVersion),
            VK_VERSION_MINOR(props.apiVersion)
        );
        return false;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    m_graphicsQueueFamilyIndex = UINT32_MAX;
    m_presentQueueFamilyIndex  = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            m_graphicsQueueFamilyIndex = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
        if (presentSupport)
            m_presentQueueFamilyIndex = i;

        if (m_graphicsQueueFamilyIndex != UINT32_MAX && m_presentQueueFamilyIndex != UINT32_MAX)
            break;
    }

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures
        = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT };
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features
        = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT };

    extendedDynamicStateFeatures.pNext = &extendedDynamicState3Features;

    VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features2.pNext                     = &extendedDynamicStateFeatures;
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExts(extCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extCount, availableExts.data());

    m_enableDynamicState  = false;
    m_enableDynamicState3 = false;

    for (uint32_t i = 0; i < extCount; i++)
    {
        if (strcmp(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, availableExts[i].extensionName) == 0)
        {
            m_enableDynamicState  = true;
            m_nvgExt.dynamicState = extendedDynamicStateFeatures.extendedDynamicState;
        }
        if (strcmp(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME, availableExts[i].extensionName) == 0)
        {
            m_enableDynamicState3       = true;
            m_nvgExt.colorBlendEquation = extendedDynamicState3Features.extendedDynamicState3ColorBlendEquation;
            m_nvgExt.colorWriteMask     = extendedDynamicState3Features.extendedDynamicState3ColorWriteMask;
        }
    }

    Logger::debug(
        "Vulkan: ext.dynamicState={} ext.colorBlendEquation={} ext.colorWriteMask={}",
        (int) m_nvgExt.dynamicState,
        (int) m_nvgExt.colorBlendEquation,
        (int) m_nvgExt.colorWriteMask
    );

    return true;
}

bool VulkanContext::createLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo graphicsQueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    graphicsQueueInfo.queueFamilyIndex        = m_graphicsQueueFamilyIndex;
    graphicsQueueInfo.queueCount              = 1;
    graphicsQueueInfo.pQueuePriorities        = &queuePriority;
    queueCreateInfos.push_back(graphicsQueueInfo);

    if (m_presentQueueFamilyIndex != m_graphicsQueueFamilyIndex)
    {
        VkDeviceQueueCreateInfo presentQueueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        presentQueueInfo.queueFamilyIndex        = m_presentQueueFamilyIndex;
        presentQueueInfo.queueCount              = 1;
        presentQueueInfo.pQueuePriorities        = &queuePriority;
        queueCreateInfos.push_back(presentQueueInfo);
    }

    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (m_enableDynamicState)
        deviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    if (m_enableDynamicState3)
        deviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extDsFeatures
        = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT };
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extDs3Features
        = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT };

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
    VkPhysicalDeviceSynchronization2Features synchronization2Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
    dynamicRenderingFeatures.dynamicRendering                         = VK_TRUE;
    synchronization2Features.synchronization2                         = VK_TRUE;
    dynamicRenderingFeatures.pNext                                    = &synchronization2Features;

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    physicalDeviceFeatures2.pNext                     = &dynamicRenderingFeatures;

    if (m_enableDynamicState)
    {
        synchronization2Features.pNext = &extDsFeatures;
        if (m_enableDynamicState3)
            extDsFeatures.pNext = &extDs3Features;
    }

    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &physicalDeviceFeatures2);
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
    synchronization2Features.synchronization2 = VK_TRUE;

    if (!m_enableDynamicState)
        synchronization2Features.pNext = nullptr;
    if (!m_enableDynamicState3)
        extDsFeatures.pNext = nullptr;

    VkDeviceCreateInfo deviceInfo      = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceInfo.queueCreateInfoCount    = (uint32_t) queueCreateInfos.size();
    deviceInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount   = (uint32_t) deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.pEnabledFeatures        = nullptr;
    deviceInfo.pNext                   = &physicalDeviceFeatures2;

    VkResult res = vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create logical device ({})", (int) res);
        return false;
    }

    volkLoadDevice(m_device);

    vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentQueueFamilyIndex, 0, &m_presentQueue);

    return true;
}

bool VulkanContext::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex        = m_graphicsQueueFamilyIndex;
    poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult res = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create command pool ({})", (int) res);
        return false;
    }

    VkCommandBufferAllocateInfo uploadAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    uploadAllocInfo.commandPool                 = m_commandPool;
    uploadAllocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    uploadAllocInfo.commandBufferCount          = 1;

    res = vkAllocateCommandBuffers(m_device, &uploadAllocInfo, &m_uploadCmdBuffer);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to allocate upload command buffer ({})", (int) res);
        return false;
    }

    return true;
}

bool VulkanContext::createSwapchain(VkSwapchainKHR oldSwapchain)
{
    VkSurfaceCapabilitiesKHR surfCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfCaps);

    if (surfCaps.currentExtent.width != UINT32_MAX)
    {
        m_swapchainExtent = surfCaps.currentExtent;
    }
    else
    {
        m_swapchainExtent.width = std::clamp(
            (uint32_t) m_framebufferWidth,
            surfCaps.minImageExtent.width,
            surfCaps.maxImageExtent.width //
        );
        m_swapchainExtent.height = std::clamp(
            (uint32_t) m_framebufferHeight,
            surfCaps.minImageExtent.height,
            surfCaps.maxImageExtent.height //
        );
    }

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    m_swapchainFormat          = formats.empty() ? VK_FORMAT_B8G8R8A8_UNORM : formats[0].format;
    VkColorSpaceKHR colorSpace = formats.empty() ? VK_COLOR_SPACE_SRGB_NONLINEAR_KHR : formats[0].colorSpace;
    for (auto& fmt : formats)
    {
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            m_swapchainFormat = fmt.format;
            colorSpace        = fmt.colorSpace;
            break;
        }
    }

    m_presentMode = choosePresentMode();

    uint32_t imageCount = std::max(surfCaps.minImageCount + 1, (uint32_t) MAX_FRAMES_IN_FLIGHT);
    if (surfCaps.maxImageCount > 0 && imageCount > surfCaps.maxImageCount)
        imageCount = surfCaps.maxImageCount;

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else
        preTransform = surfCaps.currentTransform;

    VkSwapchainCreateInfoKHR swapInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapInfo.surface                  = m_surface;
    swapInfo.minImageCount            = imageCount;
    swapInfo.imageFormat              = m_swapchainFormat;
    swapInfo.imageColorSpace          = colorSpace;
    swapInfo.imageExtent              = m_swapchainExtent;
    swapInfo.imageArrayLayers         = 1;

    swapInfo.imageUsage     = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.preTransform   = preTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode    = m_presentMode;
    swapInfo.clipped        = VK_TRUE;
    swapInfo.oldSwapchain   = oldSwapchain;

    if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex)
    {
        uint32_t indices[]             = { m_graphicsQueueFamilyIndex, m_presentQueueFamilyIndex };
        swapInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapInfo.queueFamilyIndexCount = 2;
        swapInfo.pQueueFamilyIndices   = indices;
    }
    else
    {
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkResult res = vkCreateSwapchainKHR(m_device, &swapInfo, nullptr, &m_swapchain);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create swapchain ({})", (int) res);
        return false;
    }

    if (oldSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImageCount, nullptr);
    m_swapchainImages.resize(m_swapchainImageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImageCount, m_swapchainImages.data());

    return true;
}

bool VulkanContext::createImageViews()
{
    m_swapchainImageViews.resize(m_swapchainImageCount);

    for (uint32_t i = 0; i < m_swapchainImageCount; i++)
    {
        VkImageViewCreateInfo viewInfo           = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image                           = m_swapchainImages[i];
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = m_swapchainFormat;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        VkResult res = vkCreateImageView(m_device, &viewInfo, nullptr, &m_swapchainImageViews[i]);
        if (res != VK_SUCCESS)
        {
            Logger::error("Vulkan: failed to create image view ({})", (int) res);
            return false;
        }
    }
    return true;
}

bool VulkanContext::createDepthResources()
{
    const VkFormat depthFormats[] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT };
    VkImageTiling imageTiling     = VK_IMAGE_TILING_OPTIMAL;

    m_depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    for (auto fmt : depthFormats)
    {
        VkFormatProperties fprops;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, fmt, &fprops);
        if (fprops.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            m_depthFormat = fmt;
            imageTiling   = VK_IMAGE_TILING_OPTIMAL;
            break;
        }
        if (fprops.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            m_depthFormat = fmt;
            imageTiling   = VK_IMAGE_TILING_LINEAR;
            break;
        }
    }

    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = m_depthFormat;
    imageInfo.tiling            = imageTiling;
    imageInfo.extent.width      = m_swapchainExtent.width;
    imageInfo.extent.height     = m_swapchainExtent.height;
    imageInfo.extent.depth      = 1;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create depth image ({})", (int) res);
        return false;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_device, m_depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize       = memReqs.size;
    allocInfo.memoryTypeIndex      = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    res = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthMemory);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to allocate depth memory ({})", (int) res);
        return false;
    }

    vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0);

    VkImageViewCreateInfo viewInfo           = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image                           = m_depthImage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = m_depthFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    res = vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthImageView);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to create depth image view ({})", (int) res);
        return false;
    }

    return true;
}

bool VulkanContext::createCommandBuffers()
{
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool                 = m_commandPool;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = MAX_FRAMES_IN_FLIGHT;

    VkResult res = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers);
    if (res != VK_SUCCESS)
    {
        Logger::error("Vulkan: failed to allocate command buffers ({})", (int) res);
        return false;
    }
    return true;
}

bool VulkanContext::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo   = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags               = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
        {
            Logger::error("Vulkan: failed to create sync objects");
            return false;
        }
    }
    return true;
}

void VulkanContext::cleanupSwapchain()
{
    if (m_commandBuffers[0] != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_device, m_commandPool, MAX_FRAMES_IN_FLIGHT, m_commandBuffers);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            m_commandBuffers[i] = VK_NULL_HANDLE;
    }

    if (m_depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }
    if (m_depthMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_depthMemory, nullptr);
        m_depthMemory = VK_NULL_HANDLE;
    }

    for (auto view : m_swapchainImageViews)
    {
        if (view != VK_NULL_HANDLE)
            vkDestroyImageView(m_device, view, nullptr);
    }
    m_swapchainImageViews.clear();
}

void VulkanContext::recreateSwapchain()
{
    if (m_framebufferWidth == 0 || m_framebufferHeight == 0)
        return;

    vkDeviceWaitIdle(m_device);

    VkSwapchainKHR oldSwapchain = m_swapchain;
    m_swapchain                 = VK_NULL_HANDLE;

    cleanupSwapchain();

    // Recreate the sync objects. The per-frame binary semaphores can be left signaled or pending in
    // a way that no longer matches the new swapchain's image indexing (e.g. after a present-mode
    // change via setSwapInterval), which wedges the acquire -> submit -> present chain and leaves the
    // last presented frame stuck on screen. vkDeviceWaitIdle above guarantees none are still in use,
    // so it is safe to destroy and rebuild them for a clean state.
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (m_imageAvailableSemaphores.size() > i && m_imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        if (m_renderFinishedSemaphores.size() > i && m_renderFinishedSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        if (m_inFlightFences.size() > i && m_inFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_inFlightFences.clear();

    createSwapchain(oldSwapchain);
    createImageViews();
    createDepthResources();
    createCommandBuffers();
    createSyncObjects();

    m_currentFrame = 0;
}

VkPresentModeKHR VulkanContext::choosePresentMode()
{
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &modeCount, nullptr);

    std::vector<VkPresentModeKHR> modes(modeCount);

    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &modeCount, modes.data());

    if (m_swapInterval == 0)
    {
        // True vsync off. May tear.
        for (auto mode : modes)
        {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        // No tearing, but not true "vsync off".
        // Renderer may run faster than display refresh.
        for (auto mode : modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    Logger::error("Vulkan: failed to find suitable memory type");
    return 0;
}

double VulkanContext::getScaleFactor()
{
#ifdef _WIN32
    static auto getDpiForWindow = (UINT(WINAPI*)(HWND)) GetProcAddress(GetModuleHandleA("user32.dll"), "GetDpiForWindow");
    if (getDpiForWindow)
    {
#ifdef __GLFW__
        HWND hwnd = glfwGetWin32Window(m_window);
#elif defined(__SDL3__)
        HWND hwnd = (HWND) SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif
        if (hwnd)
            return getDpiForWindow(hwnd) / 96.0;
    }
    return 1.0;
#elif defined(__GLFW__)
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(m_window, &xscale, &yscale);
    return xscale;
#elif defined(__SDL3__)
        return SDL_GetWindowDisplayScale(m_window);
#else
        return 1.0;
#endif
}

void VulkanContext::clear(NVGcolor color) { m_clearColor = color; }

void VulkanContext::beginFrame()
{
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult res = vkAcquireNextImageKHR(
        m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_currentImageIndex
    );

    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    m_frameStarted = true;

    m_currentImage          = m_swapchainImages[m_currentImageIndex];
    m_currentImageView      = m_swapchainImageViews[m_currentImageIndex];
    m_currentDepthImageView = m_depthImageView;

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier2 barriers[2] = {};
    barriers[0].sType                 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[0].srcStageMask          = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    barriers[0].srcAccessMask         = 0;
    barriers[0].dstStageMask          = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barriers[0].dstAccessMask         = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barriers[0].oldLayout             = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[0].newLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barriers[0].srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image                 = m_currentImage;
    barriers[0].subresourceRange      = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    barriers[1].sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barriers[1].srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    barriers[1].srcAccessMask       = 0;
    barriers[1].dstStageMask        = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    barriers[1].dstAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barriers[1].oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[1].newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image               = m_depthImage;
    barriers[1].subresourceRange    = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };

    VkDependencyInfo depInfo        = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount = 2;
    depInfo.pImageMemoryBarriers    = barriers;
    vkCmdPipelineBarrier2(cmd, &depInfo);

    VkRenderingAttachmentInfo colorAttachment   = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    colorAttachment.imageView                   = m_currentImageView;
    colorAttachment.imageLayout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp                      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp                     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color.float32[0] = m_clearColor.r;
    colorAttachment.clearValue.color.float32[1] = m_clearColor.g;
    colorAttachment.clearValue.color.float32[2] = m_clearColor.b;
    colorAttachment.clearValue.color.float32[3] = m_clearColor.a;

    VkRenderingAttachmentInfo depthAttachment       = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    depthAttachment.imageView                       = m_depthImageView;
    depthAttachment.imageLayout                     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp                          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                         = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth   = 1.0f;
    depthAttachment.clearValue.depthStencil.stencil = 0;

    VkRenderingInfo renderingInfo      = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea.offset    = { 0, 0 };
    renderingInfo.renderArea.extent    = m_swapchainExtent;
    renderingInfo.layerCount           = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments    = &colorAttachment;
    renderingInfo.pDepthAttachment     = &depthAttachment;
    renderingInfo.pStencilAttachment   = &depthAttachment;
    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = (float) m_swapchainExtent.width;
    viewport.height     = (float) m_swapchainExtent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset   = { 0, 0 };
    scissor.extent   = m_swapchainExtent;
    vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &scissor);
}

void VulkanContext::endFrame()
{
    if (!m_frameStarted)
        return;
    m_frameStarted = false;

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    vkCmdEndRendering(cmd);

    VkImageMemoryBarrier2 presentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    presentBarrier.srcStageMask          = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    presentBarrier.srcAccessMask         = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstStageMask          = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    presentBarrier.dstAccessMask         = 0;
    presentBarrier.oldLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.image                 = m_currentImage;
    presentBarrier.subresourceRange      = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkDependencyInfo depInfo        = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers    = &presentBarrier;
    vkCmdPipelineBarrier2(cmd, &depInfo);

    vkEndCommandBuffer(cmd);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo         = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &m_imageAvailableSemaphores[m_currentFrame];
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &m_commandBuffers[m_currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_renderFinishedSemaphores[m_currentFrame];

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);

    VkPresentInfoKHR presentInfo   = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_renderFinishedSemaphores[m_currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain;
    presentInfo.pImageIndices      = &m_currentImageIndex;

    VkResult res = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        recreateSwapchain();
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::setSwapInterval(int interval)
{
    if (m_swapInterval == interval)
        return;
    m_swapInterval = interval;
    recreateSwapchain();
}

bool VulkanContext::onFramebufferSize(int width, int height)
{
    if (width == 0 || height == 0)
        return false;

    m_framebufferWidth   = width;
    m_framebufferHeight  = height;
    m_framebufferResized = true;
    return true;
}

} // namespace brls
