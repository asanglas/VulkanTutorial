#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// typedefs
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

// constants
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

// structs
typedef struct App App;
struct App {
    GLFWwindow* window;
    VkInstance vk_instance;
};

// declarations
void init_window(App* pApp);
void init_vulkan(App* pApp);
void main_loop(App* pApp);
void cleanup(App* pApp);

void create_instance(App* pApp);

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

void init_vulkan(App* pApp) { create_instance(pApp); }
void main_loop(App* pApp)
{

    while (!glfwWindowShouldClose(pApp->window)) {
        glfwPollEvents();
    }
}
void cleanup(App* pApp)
{
    printf("Cleaning...\n");
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

    // check instance extensions
    u32 extension_count = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);
    VkExtensionProperties* extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * extension_count);
    vkEnumerateInstanceExtensionProperties(NULL, &extension_count, extensions);
    for (u32 i = 0; i < extension_count; i += 1) {
        printf("\tExtension: %s\n", extensions[i].extensionName);
    }

    // get required extensions from glfw (window stuff related, surface)
    u32 glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // check that the required extensions are in the available
    u32 found_extensions = 0;
    for (u32 i = 0; i < glfw_extension_count; i += 1) {
        for (u32 j = 0; j < extension_count; j += 1) {
            if (strcmp(extensions[j].extensionName, glfw_extensions[i]) == 0) {
                printf("%s vs %s\n", extensions[j].extensionName, glfw_extensions[i]);
                found_extensions += 1;
            }
        }
    }
    if (found_extensions == glfw_extension_count) {
        printf("All required extensions found!\n");
    }
    free(extensions); // free from heap

    // check for layer support
    u32 available_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
    VkLayerProperties* available_layers = (VkLayerProperties*)malloc(sizeof(VkLayerProperties) * available_layer_count);
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
    free(available_layers); // free from heap

    // create info
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions,
    };
    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layers_count;
        create_info.ppEnabledLayerNames = validation_layers;
    } else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&create_info, NULL, &pApp->vk_instance) != VK_SUCCESS) {
        printf("Failed to create Instance\n");
        exit(1);
    }

    printf("Vulkan instance created succesfully.\n");
}
