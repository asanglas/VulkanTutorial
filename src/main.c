#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
const u32 device_extensions_count = 1;
const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

// Structs
typedef struct QueueFamilyIndices QueueFamilyIndices;
struct QueueFamilyIndices {
    u32 graphics_family;
    u8 is_graphics_family_set; // HACK: mimic the optional in C++?
    u32 present_family;
    u8 is_present_family_set; // HACK: mimic the optional in C++?
    u8 is_complete;
};

typedef struct Shader Shader;
struct Shader {
    char* binary;
    u32 size;
};

// typedef struct SwapChainSupportDetails SwapChainSupportDetails;
// struct SwapChainSupportDetails {
//     VkSurfaceCapabilitiesKHR capabilities;
//     VkSurfaceFormatKHR format;
//     VkPresentModeKHR present_mode;
// };

typedef struct App App;
struct App {
    GLFWwindow* window;
    VkInstance vk_instance;
    VkDebugUtilsMessengerEXT vk_debugmessenger;
    VkSurfaceKHR vk_surface;
    VkPhysicalDevice vk_physical_device;
    QueueFamilyIndices vk_queue_family_indices;
    VkQueue vk_graphics_queue;
    VkQueue vk_present_queue;
    VkDevice vk_device; // logical device
    VkSwapchainKHR vk_swapchain;
    VkImage* vk_images;
    u32 vk_image_count;
    VkFormat vk_format;
    VkExtent2D vk_extent;
    VkImageView* vk_imageviews;
    VkPipelineLayout vk_pipeline_layout;
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

void create_surface(App* pApp);

u32 rate_device_suitability(VkPhysicalDevice device);
void pick_graphics_card(App* pApp);
QueueFamilyIndices find_families_queue(VkPhysicalDevice device, VkSurfaceKHR surface);

void get_unique_values(u32* array, u32 n, u32* unique_array,
                       u32* unique_values_count); // HACK: like set in C++ but much much worse
void create_logical_device(App* pApp);

u32 clamp_u32(u32 value, u32 min, u32 max);
void create_swapchain(App* pApp);
void create_imageviews(App* pApp);

// GRAPHICS STUFF
VkShaderModule create_shader_module(App* pApp, char* binary, u32 size);
Shader read_file(const char* filename);

void create_graphicspipeline(App* pApp);

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
    create_surface(pApp);
    pick_graphics_card(pApp);
    create_logical_device(pApp);
    create_swapchain(pApp);
    create_imageviews(pApp);

    create_graphicspipeline(pApp);
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

    vkDestroyPipelineLayout(pApp->vk_device, pApp->vk_pipeline_layout, NULL);
    printf("Pipeline layout destoyed.\n");

    for (u32 i = 0; i < pApp->vk_image_count; i += 1) {
        vkDestroyImageView(pApp->vk_device, pApp->vk_imageviews[i], NULL);
    }
    printf("Image views destroyed...\n");
    free(pApp->vk_imageviews);
    printf("Freeing vk_imageview...\n");
    free(pApp->vk_images);
    printf("Freeing vk_images...\n");
    vkDestroySwapchainKHR(pApp->vk_device, pApp->vk_swapchain, NULL);
    printf("Swapchain destoyed.\n");

    vkDestroyDevice(pApp->vk_device, NULL);
    printf("Logical Device destroyed.\n");

    if (enable_validation_layers) {
        DestroyDebugUtilsMessengerEXT(pApp->vk_instance, pApp->vk_debugmessenger, NULL);
        printf("Debug messenger destroyed.\n");
    }

    vkDestroySurfaceKHR(pApp->vk_instance, pApp->vk_surface, NULL);
    printf("Vulkan surface destroyed.\n");
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
    // Enumerate all instance extensions
    // for (u32 i = 0; i < all_extension_count; i += 1) {
    //     printf("\tExtension: %s\n", all_extensions[i].extensionName);
    // }

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

void create_surface(App* pApp)
{
    if (glfwCreateWindowSurface(pApp->vk_instance, pApp->window, NULL, &pApp->vk_surface) != VK_SUCCESS) {
        printf("Could not create a surface\n");
        exit(1);
    }
    printf("Created surface.\n");
}

u32 rate_device_suitability(VkPhysicalDevice device)
{
    // get the properties of the device (to get the names, etc...)
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    // get the features of the device (to get the names, etc...)
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    u32 score = 0;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // maximum size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;

    // app cannot function without geometry shaders
    if (!device_features.geometryShader) {
        return 0;
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

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(pApp->vk_physical_device, &device_properties);
    printf("Selected Physical Device: %s (with score %u)\n", device_properties.deviceName, physical_device_score);

    // check queue families and look for the graphics bit (for now)
    printf("Checking queue families...\n");
    QueueFamilyIndices queue_family_index = find_families_queue(pApp->vk_physical_device, pApp->vk_surface);
    if (!queue_family_index.is_graphics_family_set) {
        printf("Graphics Family not supported!\n");
        exit(1);
    }

    // set the queue family index
    printf("Setting the queue family index...,\n");
    pApp->vk_queue_family_indices = queue_family_index;

    // check for swapchain capability
    u32 device_available_extensions_count;
    vkEnumerateDeviceExtensionProperties(pApp->vk_physical_device, NULL, &device_available_extensions_count, NULL);
    VkExtensionProperties device_available_extensions[device_available_extensions_count];
    vkEnumerateDeviceExtensionProperties(pApp->vk_physical_device, NULL, &device_available_extensions_count,
                                         device_available_extensions);

    u32 found_device_extensions = 0;
    for (u32 i = 0; i < device_extensions_count; i += 1) {
        for (u32 j = 0; j < device_available_extensions_count; j += 1) {
            // printf("%s vs %s\n", device_extensions[i], device_available_extensions[j].extensionName);
            if (strcmp(device_extensions[i], device_available_extensions[j].extensionName) == 0) {
                printf("Found extension %s\n", device_extensions[i]);
                found_device_extensions += 1;
                break;
            }
        }
    }
    if (found_device_extensions != device_extensions_count) {
        printf("Not all devices extensions are supported!\n");
        exit(0);
    }
    printf("All required device extensions are supported!\n");
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

QueueFamilyIndices find_families_queue(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;
    indices.is_graphics_family_set = 0;
    indices.is_present_family_set = 0;
    VkBool32 present_support = false;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_family_properties[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);

    indices.is_graphics_family_set = 0;

    for (u32 i = 0; i < queue_family_count; i += 1) {
        u32 queue_family = queue_family_properties[i].queueFlags;
        print_queue_family_to_string(queue_family);
    }
    // set the correct flag for the graphics family queue
    for (u32 i = 0; i < queue_family_count; i += 1) {
        if (!indices.is_graphics_family_set) {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                printf("Family %u has the Graphics Bit\n", queue_family_properties[i].queueFlags);
                indices.graphics_family = i;
                indices.is_graphics_family_set = 1;
            }
        }

        if (!indices.is_present_family_set) {
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (present_support) {
                printf("Family %u has Present support\n", queue_family_properties[i].queueFlags);
                indices.present_family = i;
                indices.is_present_family_set = 1;
            }
        }
    }
    printf("\n");

    return indices;
}

// Comparison function for qsort
int compare_u32(const void* a, const void* b) { return (*(u32*)a - *(u32*)b); }
void get_unique_values(u32* array, u32 n, u32* unique_array, u32* unique_values_count)
{
    // sort the array in increaing order
    qsort(array, n, sizeof(u32), compare_u32);
    // Find the number of unique values

    if (unique_array == NULL) {
        *unique_values_count = 1; // at least we have one, the first element
        for (u32 i = 1; i < n; i++) {
            if (array[i] != array[i - 1]) {
                *unique_values_count += 1;
            }
        }
        return;
    }

    u32 count = 0;
    unique_array[0] = array[0]; // at least the first array
    for (u32 i = 1; i < *unique_values_count; i++) {
        if (array[i] != array[i - 1]) {
            unique_array[count++] = array[i];
        }
    }
    return;
}

void create_logical_device(App* pApp)
{
    // queue info
    float queue_priority = 1.0;
    u32 all_queue_families[] = {pApp->vk_queue_family_indices.graphics_family,
                                pApp->vk_queue_family_indices.present_family};
    u32 all_queue_family_count = sizeof(all_queue_families) / sizeof(all_queue_families[0]); // 2
    u32 unique_queue_families_count;
    get_unique_values(all_queue_families, all_queue_family_count, NULL, &unique_queue_families_count);
    u32 unique_queue_families[unique_queue_families_count];
    get_unique_values(all_queue_families, all_queue_family_count, unique_queue_families, &unique_queue_families_count);
    printf("There are %u unique families.\n", unique_queue_families_count);

    VkDeviceQueueCreateInfo queue_create_infos[unique_queue_families_count];

    for (u32 i = 0; i < unique_queue_families_count; i += 1) {
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = unique_queue_families[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };

        queue_create_infos[i] = queue_create_info;
    }

    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queue_create_infos,
        .queueCreateInfoCount = unique_queue_families_count,
        .pEnabledFeatures = &device_features,
        .ppEnabledExtensionNames = device_extensions,
        .enabledExtensionCount = device_extensions_count, // For now we don't need device extensions
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

    // get the present family queue and store the handle

    printf("Created logical device!\n");
}

void create_swapchain(App* pApp)
{
    // *** CHECK SWAPCHAIN DETAILS
    printf("Creating the swapchain\n");
    // 2. Surface formats (pixel format, color space)
    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pApp->vk_physical_device, pApp->vk_surface, &format_count, NULL);
    if (format_count == 0) {
        printf("\tNot enough formats (0) available\n");
        exit(1);
    }
    printf("\tThere are %u available formats: ", format_count);
    VkSurfaceFormatKHR surface_formats[format_count];
    vkGetPhysicalDeviceSurfaceFormatsKHR(pApp->vk_physical_device, pApp->vk_surface, &format_count, surface_formats);
    for (u32 i = 0; i < format_count; i += 1) {
        printf("%u ", surface_formats[i].format); // VK_FORMAT_B8G8R8A8_UNORM and VK_FORMAT_B8G8R8A8_SRGB
    }

    // 3. Available presentation modes
    u32 present_modes_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pApp->vk_physical_device, pApp->vk_surface, &present_modes_count, NULL);
    if (present_modes_count == 0) {
        printf("\tNot enough present_modes (0) available\n");
        exit(1);
    }
    VkPresentModeKHR present_modes[present_modes_count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(pApp->vk_physical_device, pApp->vk_surface, &present_modes_count,
                                              present_modes);
    printf("\n\tThere are %u presentation modes: ", present_modes_count);
    for (u32 i = 0; i < present_modes_count; i += 1) {
        printf("%u ", present_modes[i]); // VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR,
                                         // VK_PRESENT_MODE_FIFO_RELAXED_KHR
    }
    printf("\n\tThe swapchain is capable\n");

    // Select the format
    VkSurfaceFormatKHR surface_format;
    i32 chosen_format = -1;
    for (u32 i = 0; i < format_count; i += 1) {
        if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen_format = i;
            printf("\tFound requirements. Chosing the format %u and colorspace %u\n",
                   surface_formats[chosen_format].format, surface_formats[chosen_format].colorSpace);
        }
    }
    if (chosen_format == -1) {
        chosen_format = 0;
        printf("\tThere are not the required format and colorspace. Chosing the first available one\n");
        printf("\t\tChosing the format %u and colorspace %u\n", surface_formats[chosen_format].format,
               surface_formats[chosen_format].colorSpace);
    }
    surface_format = surface_formats[chosen_format];

    // Select the present_mode
    VkPresentModeKHR present_mode;
    i32 chosen_present_mode = -1;
    for (u32 i = 0; i < present_modes_count; i += 1) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosen_present_mode = i;
            printf("\tChosen the VK_PRESENT_MODE_MAILBOX_KHR");
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    if (chosen_present_mode == -1) {
        printf("\tThere are not the required present modes. Chosing VK_PRESENT_MODE_FIFO_KHR\n");
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }

    // 1. Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pApp->vk_physical_device, pApp->vk_surface, &capabilities);

    printf("\tCurrent extent (pixels) (%u, %u)\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
    printf("\tMin extent: (%u, %u)\n", capabilities.minImageExtent.width, capabilities.minImageExtent.height);
    printf("\tMax extent: (%u, %u)\n", capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);

    VkExtent2D extent = {0, 0};
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        i32 w, h;
        glfwGetFramebufferSize(pApp->window, &w, &h);
        printf("\tGLFW framebuffer size: (%u, %u)\n", w, h);
        w = clamp_u32(w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        h = clamp_u32(h, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        extent.width = w;
        extent.height = h;
    }
    printf("\tChosen the current extent, which is (%u, %u)\n", extent.width, extent.height);

    printf("\tFormat: %u\n\tColor Space: %u\n", surface_format.format, surface_format.colorSpace);
    printf("\tPresent Mode: %u\n", present_mode);
    printf("\tExtent: (%u, %u)\n", extent.width, extent.height);
    // it is recomended to request at least one more image the minimum capable images
    u32 image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = pApp->vk_surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    // set the image sharing mode between the queues
    u32 queue_family_indeces[] = {pApp->vk_queue_family_indices.graphics_family,
                                  pApp->vk_queue_family_indices.present_family};
    if (pApp->vk_queue_family_indices.graphics_family != pApp->vk_queue_family_indices.present_family) {
        printf("\tThe queue families are not the same, creating two queues\n");
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_family_indeces;
    } else {
        printf("\tThe queue families are the same\n");
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;  // optional
        swapchain_info.pQueueFamilyIndices = NULL; // optional
    }

    // do not transform the image;
    printf("\tCurrent transform: %u\n", capabilities.currentTransform);
    swapchain_info.preTransform = capabilities.currentTransform;
    // the way to blend the window with others. Probably always ommit the alpha
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    // sometimes, if the window is resized we need to create another swapchain, and we need to refer to the old one. For
    // now, we just create one.
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    // create the swapchain
    if (vkCreateSwapchainKHR(pApp->vk_device, &swapchain_info, NULL, &pApp->vk_swapchain) != VK_SUCCESS) {
        printf("Failed to create the swapchain!\n");
        exit(1);
    }
    printf("Swapchain succesfully created!\n");

    // Retrieve the swapchain images
    printf("Retrieve Swapchain images...\n");
    printf("Old image count %u\n", image_count);
    vkGetSwapchainImagesKHR(pApp->vk_device, pApp->vk_swapchain, &image_count, NULL); // we get 3 (2 + 1)
    printf("After image count %u\n", image_count);
    // the problem is that with VLA the memory address gets freed when leaving the scope... we have to malloc,
    // apparently...
    VkImage* swapchain_images = (VkImage*)malloc(image_count * sizeof(VkImage));
    // VkImage swapchain_images[image_count];
    vkGetSwapchainImagesKHR(pApp->vk_device, pApp->vk_swapchain, &image_count, swapchain_images);

    pApp->vk_images = swapchain_images;
    pApp->vk_image_count = image_count;
    pApp->vk_format = surface_format.format;
    pApp->vk_extent = extent;
}

u32 clamp_u32(u32 value, u32 min, u32 max)
{
    if (value <= max)
        return max;
    if (value >= min)
        return min;
    return 0;
}

void create_imageviews(App* pApp)
{
    // the image views defines how the images should be read and interpreted.

    // again... malloc...
    VkImageView* image_views = (VkImageView*)malloc(pApp->vk_image_count * sizeof(VkImageView));

    for (u32 i = 0; i < pApp->vk_image_count; i += 1) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = pApp->vk_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = pApp->vk_format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        if (vkCreateImageView(pApp->vk_device, &create_info, NULL, &image_views[i]) != VK_SUCCESS) {
            printf("failed to create image views!");
            exit(1);
        }
    }

    pApp->vk_imageviews = image_views;
}

Shader read_file(const char* filename)
{
    FILE* file;
    file = fopen(filename, "rb");

    if (file == NULL) {
        printf("The file couldn't be opened!\n");
        exit(1);
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    unsigned long file_size = ftell(file);
    printf("Filename %s has size of: %lu\n", filename, file_size);
    fseek(file, 0, SEEK_SET); // Reset file position indicator to the beginning
    // printf("Postion of the file pointer at: %lu\n", ftell(file));

    char* buffer = (char*)malloc(file_size);
    fread(buffer, file_size, sizeof(char), file);
    fclose(file);

    Shader shader;
    shader.binary = buffer;
    shader.size = file_size;

    return shader;
}

VkShaderModule create_shader_module(App* pApp, char* binary, u32 size)
{
    UNUSED(binary);
    UNUSED(size);

    VkShaderModuleCreateInfo shader_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (u32*)binary,
    };

    VkShaderModule shader_module = {0};
    if (vkCreateShaderModule(pApp->vk_device, &shader_info, NULL, &shader_module) != VK_SUCCESS) {
        printf("Could not create shader module!\n");
        exit(1);
    }

    return shader_module;
}

void create_graphicspipeline(App* pApp)
{
    UNUSED(pApp);

    Shader vert_shader_binary = read_file("build/shaders/vertex.spv");
    Shader frag_shader_binary = read_file("build/shaders/fragment.spv");

    VkShaderModule vert_module = create_shader_module(pApp, vert_shader_binary.binary, vert_shader_binary.size);
    VkShaderModule frag_module = create_shader_module(pApp, frag_shader_binary.binary, frag_shader_binary.size);

    // Assign the shaders to a specific stage in the graphics pipeline
    // Start with the vertex shader
    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT, // here
        .module = vert_module,
        .pName = "main", // the entry point
    };

    // The fragment shader
    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT, // here
        .module = frag_module,
        .pName = "main", // the entry point
    };

    // store here the shader stages, the programable parts
    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    // The fixed functions, non programable

    // what is this? states that can be changed without recreating the pipeline at draw time. We set the viewport and
    // the scissors
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    u32 dynamic_states_count = 2;
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_states_count,
        .pDynamicStates = dynamic_states,
    };

    // The vertex input. How the vertex data will be passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL, // Optional
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL, // Optional
    };

    // vertex assembler how the vertex data will be read
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // set the viewport. The part of the frambuffer that the output will be rendered to. For now (and almost always)
    // will be from (0,0) to (width, height).

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = pApp->vk_extent.width,
        .height = pApp->vk_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    // set the scissor. Any pixel outside the scissor will not be considered for the rasterizer
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = pApp->vk_extent,
    };

    // Since we are doing the viewport and scissor dynamic, we don't need to don't need to pass the viewport and scissor
    // to be generated at the creation of the pipeline (do not pass to the pipeline layout, basically)
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    // the rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp = 0.0f,          // Optional
        .depthBiasSlopeFactor = 0.0f,    // Optional
    };

    // multisampling
#if 0
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,          // Optional
        .pSampleMask = NULL,               // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE,      // optional
    };
#endif

    // Depth and stencil testing
    // for now a NULL ptr to the info struct

    // color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .colorBlendOp = VK_BLEND_OP_ADD,             // Optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,  // Optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
        .alphaBlendOp = VK_BLEND_OP_ADD,             // Optional
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants[0] = 0.0f, // Optional
        .blendConstants[1] = 0.0f, // Optional
        .blendConstants[2] = 0.0f, // Optional
        .blendConstants[3] = 0.0f, // Optional
    };

    // Pipeline layout

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,         // Optional
        .pSetLayouts = NULL,         // Optional
        .pushConstantRangeCount = 0, // Optional
        .pPushConstantRanges = NULL, // Optional
    };
    // create it
    if (vkCreatePipelineLayout(pApp->vk_device, &pipeline_layout_info, NULL, &pApp->vk_pipeline_layout) != VK_SUCCESS) {
        printf("Pipeline Layout couldn't create properly\n");
        exit(1);
    }

    // Clean the modules
    vkDestroyShaderModule(pApp->vk_device, vert_module, NULL);
    vkDestroyShaderModule(pApp->vk_device, frag_module, NULL);

    // TODO: move it
    printf("Freeing the binaries files from the heap\n");
    free(vert_shader_binary.binary);
    free(frag_shader_binary.binary);
}
