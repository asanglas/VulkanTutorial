#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef uint32_t u32;

const char* WIN_TITLE = "Vulkan";
const u32 WIN_WIDTH = 800;
const u32 WIN_HEIGHT = 600;

typedef struct App App;
struct App {
    GLFWwindow* window;
};

void init_window(App* pApp);
void init_vulkan(void);
void main_loop(App* pApp);
void cleanup(App* pApp);

int main(void)
{
    App app = {0};

    init_window(&app);
    init_vulkan();
    main_loop(&app);
    cleanup(&app);

    return 0;
}

void init_window(App* pApp)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pApp->window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
}

void init_vulkan(void) {}
void main_loop(App* pApp)
{

    while (!glfwWindowShouldClose(pApp->window)) {
        glfwPollEvents();
    }
}
void cleanup(App* pApp)
{
    glfwDestroyWindow(pApp->window);
    glfwTerminate();
}
