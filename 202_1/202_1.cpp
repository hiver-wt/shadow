#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<camera.h>

#include<imgui/imgui.h>
#include<imgui/imgui_impl_glfw.h>
#include<imgui/imgui_impl_opengl3.h>

#include"model.h"

#include<iostream>
#include<cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

unsigned int SCR_WIDTH = 1024;
unsigned int SCR_HEIGHT = 1024;
const char* glsl_version = "#version 460";

// camera
Camera camera(glm::vec3(0.0f, 30.0f, 30.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    glfwInit(); //��ʼ��GLFW

    //������Ҫ�ʹ�Ҫ�汾
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);//ʹ�õ��Ǻ���ģʽ(Core-profile)��ֻ��ʹ��OpenGL���ܵ�һ���Ӽ�

    //�������ڶ���
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();//�����߳�
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);//ע�ᴰ�ڱ任��������
    glfwSetCursorPosCallback(window, mouse_callback);//����ƶ�
    glfwSetScrollCallback(window, scroll_callback);//�����ֹ���

    // ���ع��
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    /*�ڵ����κ�OpenGL�ĺ���֮ǰ������Ҫ��ʼ��GLAD��*/
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    //------------------------------
    //����imgui������
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    //����imgui���
    ImGui::StyleColorsDark();

    //����ƽ̨����Ⱦ��
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    Shader shadow("./202/202_1/shader/shadow.vert", "./202/202_1/shader/shadow.frag");
    Shader shadowMapping("./202/202_1/shader/shadowDepth.vert", "./202/202_1/shader/shadowDepth.frag");

    //Model ourModel("./model/elysia/elysia.obj");
    Model ourModel("./model/mary/Marry.obj");
    Model floor("./model/floor/floor.obj");

    unsigned int FBO;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    const unsigned int SHADOW_WIDTH = 1024;
    const unsigned int SHADOW_HEIGHT = 1024;
    unsigned int shadowMap;
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMap, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shadow.use();
    shadow.setInt("depthMap", 1);
    glm::vec3 lightPos(0, 80.0, 80.0);

    //������Ȳ���
    glEnable(GL_DEPTH_TEST);

    //imgui����ֵ
    ImVec4 clear_color = ImVec4(0.1, 0.1, 0.1, 1.0);
    float biasOffset = 0.005;
    float lightWidth = 10.0;
    while (!glfwWindowShouldClose(window))
    {
        //ÿ֡ʱ���߼�
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //����
        processInput(window);
        //����imgui���
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("ImGui");
        ImGui::ColorEdit3("clear color", (float*)&clear_color);
        ImGui::SliderFloat("Bias Offset", &biasOffset, 0.005, 0.03);
        ImGui::SliderFloat("light Width", &lightWidth, 10.0, 100.0);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        //��Ⱦָ��
        /*������glClear�����������ɫ����֮��������ɫ���嶼�ᱻ���ΪglClearColor�������õ���ɫ��*/
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //1-Pass
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shadowMapping.use();
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0), glm::vec3(0, 1, 0));

        //  ͶӰ���󣬼ٶ�ʹ����ƽ�й�����ʹ��������ͶӰ��
        float near_plane = 1e-2, far_plane = 400;
        glm::mat4 lightProjection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, near_plane, far_plane);
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        shadowMapping.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // floor
        glm::mat4 model = glm::mat4(1.0);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -30.0));
        model = glm::scale(model, glm::vec3(4.0));
        shadowMapping.setMat4("model", model);
        floor.draw(shadowMapping);

        // ourModel
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0));
        model = glm::scale(model, glm::vec3(10));
        shadowMapping.setMat4("model", model);
        ourModel.draw(shadowMapping);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //---------------------------------------------
        //2-Pass
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shadow.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shadow.setMat4("projection", projection);
        shadow.setMat4("view", view);
        shadow.setVec3("viewPos", camera.Position);
        shadow.setVec3("lightPos", lightPos);
        shadow.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shadow.setFloat("biasOffset", biasOffset);
        shadow.setFloat("lightWidth", lightWidth);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        // floor
        model = glm::mat4(1.0);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -30.0));
        model = glm::scale(model, glm::vec3(4));
        shadow.setMat4("model", model);
        floor.draw(shadow);

        // ourModel
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0));
        model = glm::scale(model, glm::vec3(10));
        shadow.setMat4("model", model);
        ourModel.draw(shadow);

        //��Ⱦgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        //��������
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // ����ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();//��ȷ�ͷ�/ɾ��֮ǰ�ķ����������Դ
    return 0;
}

//�ӿڵ������ص���������
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

//�����ر�
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // ��ת����Ϊ y ������µ���

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}