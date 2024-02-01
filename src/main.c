#include <stdio.h>
#include <stdlib.h>

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

    // get required extensions from glfw (window stuff related, surface)
    u32 glfw_extension_count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions,
        .enabledLayerCount = 0,
    };

    if (vkCreateInstance(&create_info, NULL, &pApp->vk_instance) != VK_SUCCESS) {
        printf("Failed to create Intance\n");
        exit(1);
    }

    printf("Vulkan instance created succesfully.\n");
}
