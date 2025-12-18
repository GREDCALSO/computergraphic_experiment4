#include<glad/glad.h>
#include<GLFW/glfw3.h>

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // 初始化GLFW
    if (!glfwInit()) {
        return -1;
    }

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // 设置当前上下文
    glfwMakeContextCurrent(window);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 清屏
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window);

        // 交换缓冲区
        glfwSwapBuffers(window);

        // 处理输入
        glfwPollEvents();
    }

    // 清理
    glfwTerminate();
    return 0;
}