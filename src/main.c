#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// typedefs
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
#define UNUSED(x) (void)(x)

const char* WIN_TITLE = "Vulkan";
const u32 WIN_WIDTH = 800;
const u32 WIN_HEIGHT = 600;

const u32 validation_layers_count = 1;
const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

#if NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

// TODO structs
typedef struct QueueFamilyIndices QueueFamilyIndices;
struct QueueFamilyIndices {
    u32 graphics_family;
    u8 is_graphics_family_set; // HACK: mimic the optional in C++?
};

typedef struct App App;
struct App {
    GLFWwindow* window;
    VkInstance vk_instance;
    VkDebugUtilsMessengerEXT vk_debugmessenger;
    VkPhysicalDevice vk_physical_device;
    QueueFamilyIndices vk_queue_family_indices;
    VkQueue vk_graphics_queue;
    VkDevice vk_device; // logical device
};

// declarations
void init_window(App* pApp);
void init_vulkan(App* pApp);
void main_loop(App* pApp);
void cleanup(App* pApp);

void create_instance(App* pApp);

// debug messenger
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                              VkDebugUtilsMessageTypeFlagsEXT message_type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pcallback_data,
                                              void* puser_data);

void setup_debug_messenger(App* pApp);
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* create_info);

u32 rate_device_suitability(VkPhysicalDevice device);
void pick_graphics_card(App* pApp);
QueueFamilyIndices find_families_queue(VkPhysicalDevice device);

void create_logical_device(App* pApp);

// main
int main(void)
{
    App app = {0};

    init_window(&app);
    init_vulkan(&app);
    main_loop(&app);
    cleanup(&app);

    return 0;
}

// implementations
void init_window(App* pApp)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pApp->window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
}

void init_vulkan(App* pApp)
{
    create_instance(pApp);
    setup_debug_messenger(pApp);
    pick_graphics_card(pApp);
    create_logical_device(pApp);
}
void main_loop(App* pApp)
{
    while (!glfwWindowShouldClose(pApp->window)) {
        glfwPollEvents();
    }
}
void cleanup(App* pApp)
{
    printf("Cleaning...\n");

    vkDestroyDevice(pApp->vk_device, NULL);
    printf("Logical Device destroyed.\n");

    if (enable_validation_layers) {
        DestroyDebugUtilsMessengerEXT(pApp->vk_instance, pApp->vk_debugmessenger, NULL);
        printf("Debug messenger destroyed.\n");
    }

    vkDestroyInstance(pApp->vk_instance, NULL);
    printf("Vulkan instance destroyed.\n");

    glfwDestroyWindow(pApp->window);
    glfwTerminate();
    printf("Destroying GLFW window and terminating.\n");
}

void create_instance(App* pApp)
{
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
        .pNext = NULL,
    };

    // check all available instance extensions
    u32 all_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &all_extension_count, NULL);
    VkExtensionProperties all_extensions[all_extension_count];
    vkEnumerateInstanceExtensionProperties(NULL, &all_extension_count, all_extensions);
    for (u32 i = 0; i < all_extension_count; i += 1) {
        printf("\tExtension: %s\n", all_extensions[i].extensionName);
    }

    // get required extensions from glfw (window stuff related, surface)
    u32 glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // add the debug util extension
    if (enable_validation_layers) {
        glfw_extension_count += 1;
    }
    printf("glfw_extension_count: %u\n", glfw_extension_count);

    const char* extensions[glfw_extension_count];
    for (u32 i = 0; i < glfw_extension_count; i += 1) {
        extensions[i] = glfw_extensions[i];
    }

    if (enable_validation_layers) {
        extensions[glfw_extension_count - 1] = "VK_EXT_debug_utils";
    }

    // check that the required extensions are in the available
    u32 found_extensions = 0;
    for (u32 i = 0; i < glfw_extension_count; i += 1) {
        for (u32 j = 0; j < all_extension_count; j += 1) {
            if (strcmp(all_extensions[j].extensionName, extensions[i]) == 0) {
                printf("%s vs %s\n", all_extensions[j].extensionName, extensions[i]);
                found_extensions += 1;
            }
        }
    }
    if (found_extensions == glfw_extension_count) {
        printf("All required extensions found!\n");
    } else {
        printf("Extension failed\n");
    }

    // check for layer support
    u32 available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
    VkLayerProperties available_layers[available_layer_count];
    vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);
    u32 found_validation_layers = 0;
    for (u32 i = 0; i < validation_layers_count; i += 1) {
        for (u32 j = 0; j < available_layer_count; j += 1) {
            if (strcmp(available_layers[j].layerName, validation_layers[i]) == 0) {
                printf("%s vs %s\n", available_layers[j].layerName, validation_layers[i]);
                found_validation_layers += 1;
            }
        }
    }
    if (found_validation_layers == validation_layers_count) {
        printf("All required validation layers found!\n");
    }

    // create info
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};

    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layers_count;
        create_info.ppEnabledLayerNames = validation_layers;

        populateDebugMessengerCreateInfo(&debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debug_create_info);

    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = NULL;
    }

    if (vkCreateInstance(&create_info, NULL, &pApp->vk_instance) != VK_SUCCESS) {
        printf("Failed to create Instance\n");
        exit(1);
    }

    printf("Vulkan instance created succesfully.\n");
}

// DEBUG MESSENGER
// TODO: move to source file
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                              VkDebugUtilsMessageTypeFlagsEXT message_type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pcallback_data,
                                              void* puser_data)
{
    UNUSED(message_severity);
    UNUSED(message_type);
    UNUSED(puser_data);
    printf("[DEBUG MESSENGER]: Validation layer: %s\n", pcallback_data->pMessage);
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* create_info)
{
    create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info->pfnUserCallback = debug_callback;
    create_info->pUserData = NULL;
}

void setup_debug_messenger(App* pApp)
{
    if (!enable_validation_layers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    populateDebugMessengerCreateInfo(&create_info);

    if (CreateDebugUtilsMessengerEXT(pApp->vk_instance, &create_info, NULL, &pApp->vk_debugmessenger) != VK_SUCCESS) {
        printf("Failed to setup debug messenger!\n");
        exit(1);
    }

    printf("Debug messenger set up.\n");
}

u32 rate_device_suitability(VkPhysicalDevice device)
{
    // get the properties of the device (to get the names, etc...)
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    // get the features of the device (to get the names, etc...)
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);
    printf("\nLooking for device: %s \n", device_properties.deviceName);

    u32 score = 0;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // maximum size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;

    // app cannot function without geometry shaders
    if (!device_features.geometryShader) {
        score = 0;
    }

    return score;
}

void pick_graphics_card(App* pApp)
{

    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(pApp->vk_instance, &physical_device_count, NULL);
    if (physical_device_count == 0) {
        printf("There are no devices available!");
        exit(1);
    }
    printf("Physical devices count: %u\n", physical_device_count);

    // get the physical devices
    VkPhysicalDevice physical_devices[physical_device_count];
    vkEnumeratePhysicalDevices(pApp->vk_instance, &physical_device_count, physical_devices);

    VkPhysicalDevice chosen_physical_device = VK_NULL_HANDLE;
    u32 physical_device_score = 0;
    for (u32 i = 0; i < physical_device_count; i += 1) {
        u32 score = rate_device_suitability(physical_devices[i]);
        if (score > physical_device_score) {
            physical_device_score = score;
            chosen_physical_device = physical_devices[i];
        }
    }
    pApp->vk_physical_device = chosen_physical_device;
    if (pApp->vk_physical_device == VK_NULL_HANDLE) {
        printf("Could not find any suitable device!\n");
        exit(1);
    }

    // check queue families and look for the graphics bit (for now)
    printf("Checking queue families...\n");
    QueueFamilyIndices queue_family_index = find_families_queue(pApp->vk_physical_device);
    if (!queue_family_index.is_graphics_family_set) {
        printf("Graphics Family not supported!\n");
        exit(1);
    }

    // set the queue family index
    printf("Setting the queue family index...,\n");
    pApp->vk_queue_family_indices = queue_family_index;

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(pApp->vk_physical_device, &device_properties);
    printf("Selected Physical Device: %s (with score %u)\n", device_properties.deviceName, physical_device_score);
}

// A helper function
void print_queue_family_to_string(u32 queue_family)
{
    const char* QueueFlagNames[] = {
        "VK_QUEUE_GRAPHICS_BIT",        "VK_QUEUE_COMPUTE_BIT",   "VK_QUEUE_TRANSFER",
        "VK_QUEUE_SPARSE_BINDING_BIT",  "VK_QUEUE_PROTECTED_BIT", "VK_QUEUE_VIDEO_DECODE_BIT_KHR",
        "VK_QUEUE_OPTICAL_FLOW_BIT_NV",
    };
    uint32_t QueueFlagMasks[] = {
        0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000100,
    };
    uint32_t number_of_flags = sizeof(QueueFlagNames) / sizeof(QueueFlagNames[0]);

    printf("Queue Family %u (binary 0x%08b) has the flags: ", queue_family, queue_family);
    for (uint32_t i = 0; i < number_of_flags; i += 1) {
        if (queue_family & QueueFlagMasks[i]) {
            printf("%s, ", QueueFlagNames[i]);
        }
    }
    printf("\n");
}

QueueFamilyIndices find_families_queue(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_family_properties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

    indices.is_graphics_family_set = 0;

    for (u32 i = 0; i < queue_family_count; i += 1) {
        u32 queue_family = queue_family_properties[i].queueFlags;
        print_queue_family_to_string(queue_family);
    }
    // set the correct flag
    for (u32 i = 0; i < queue_family_count; i += 1) {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            printf("Family %u has the Graphics Bit", queue_family_properties[i].queueFlags);
            indices.graphics_family = i;
            indices.is_graphics_family_set = 1;
            break;
        }
    }
    printf("\n");

    return indices;
}

void create_logical_device(App* pApp)
{
    // queue info
    float queue_priority = 1.0;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pApp->vk_queue_family_indices.graphics_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &device_features,
        .enabledExtensionCount = 0, // For now we don't need device extensions
    };

    // Device specific layers are not needed anymore in newer versions of Vulkan. Just keep it for backwards
    // compatibilty
    if (enable_validation_layers) {
        device_info.enabledLayerCount = validation_layers_count;
        device_info.ppEnabledLayerNames = validation_layers;
    } else {
        device_info.enabledLayerCount = 0;
    }

    // create the logical device
    if (vkCreateDevice(pApp->vk_physical_device, &device_info, NULL, &pApp->vk_device) != VK_SUCCESS) {
        printf("Could not create logical device!\n");
        exit(1);
    }

    // get the graphics family queue and store the handle (we will need it)
    vkGetDeviceQueue(pApp->vk_device, pApp->vk_queue_family_indices.graphics_family, 0, &pApp->vk_graphics_queue);

    printf("Created logical device!\n");
}
