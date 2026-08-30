#include "renderer.hpp"
#include "input.h"
#include "vector.hpp"
#include "point.hpp"
#include "mesh.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <random>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

using namespace geometry;

class Trabalho01 : public Renderer {
public:
    Trabalho01() = default;

protected:
    void onInit(int w, int h, const std::string&) override {
        onWindowResize(w, h);
        generateRandomPolyhedron();
    }

    void onWindowResize(int w, int h) override {
        width  = w;
        height = h;
    }

    void onUpdate(float dt) override {
        updateInput();
        updateCamera();
        updateButtonClick();
    }

    void onUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin(
            "Main",
            nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );

        float fullHeight = ImGui::GetContentRegionAvail().y;

        ImGui::BeginChild("Panel", ImVec2(leftPanelWidth, fullHeight), true);
        panelUI();
        ImGui::EndChild();

        drawVerticalSplitter(leftPanelWidth, 150.0f, 200.0f);

        ImGui::SameLine();
        ImGui::BeginChild(
            "Canvas",
            ImVec2(0, fullHeight),
            true,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse
        );
        drawCanvas();
        ImGui::EndChild();

        ImGui::End();
    }

private:
    int width = 0;
    int height = 0;
    float dt;

    double mouseX{0.0};
    double mouseY{0.0};
    double mouseDx{0.0};
    double mouseDy{0.0};

    float leftPanelWidth = 300.0f;

    enum ButtonClick {
        none = 0,
        randomPolyhedron,
        resetCamera,
    } buttonClick = ButtonClick::none;

    Mesh3f mesh;
    bool shouldUpdateCamera = true;

    GLuint fbo = 0;
    GLuint fboTexture = 0;
    GLuint rbo = 0;
    ImVec2 canvasSize = {0, 0};
    ImVec2 canvasOrigin = {0, 0};
    bool holdingOnCanvasMouseBtn0 = false;
    bool holdingOnCanvasMouseBtn1 = false;

    struct LookAt {
        float boom = 15;
        float angleX = 0;
        float angleY = 0;
        GLdouble centerX = 0;
        GLdouble centerY = 0;
        GLdouble centerZ = 0;
        GLdouble forwardX = 0;
        GLdouble forwardY = 0;
        GLdouble forwardZ = 0;
        GLdouble upX = 0;
        GLdouble upY = 0;
        GLdouble upZ = 0;
        GLdouble rightX = 0;
        GLdouble rightY = 0;
        GLdouble rightZ = 0;
    } camera;

private:
    void generateRandomPolyhedron() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-2.0, 2.0);

        // Criar nova mesh
        mesh = Mesh3f();

        // Gerar 10 pontos aleatórios
        std::vector<Point3f> points;
        for (int i = 0; i < 10; ++i) {
            points.push_back(Point3f({
                (float)dis(gen),
                (float)dis(gen),
                (float)dis(gen)
            }));
        }

        // Adicionar vértices à mesh
        std::vector<int> indices;
        for (const auto& p : points) {
            indices.push_back((int)mesh.addVertex(p));
        }

        // Calcular o centro usando Vector3f
        Vec3f centerVec{0.0f, 0.0f, 0.0f};
        for (const auto& p : points) {
            centerVec = centerVec + Vec3f{p[0], p[1], p[2]};
        }
        centerVec = centerVec / (float)points.size();

        // Ordenar pontos por ângulo ao redor do centro
        std::sort(indices.begin(), indices.end(),
            [&](int a, int b) {
                const auto& pa = mesh.getVertices()[a];
                const auto& pb = mesh.getVertices()[b];
                Vec3f va = Vec3f{pa[0], pa[1], pa[2]} - centerVec;
                Vec3f vb = Vec3f{pb[0], pb[1], pb[2]} - centerVec;
                float angleA = std::atan2(va[2], va[0]);
                float angleB = std::atan2(vb[2], vb[0]);
                return angleA < angleB;
            });

        // Adicionar centro como vértice
        int centerIdx = (int)mesh.addVertex(Point3f{centerVec[0], centerVec[1], centerVec[2]});

        // Criar faces triangulares
        int n = indices.size();
        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            mesh.addFace(indices[i], indices[j], centerIdx);
        }

        // Resetar câmera
        camera.centerX = 0;
        camera.centerY = 0;
        camera.centerZ = 0;
        camera.angleX = 0;
        camera.angleY = 0;
        camera.boom = 10;
        shouldUpdateCamera = true;
    }

    void updateButtonClick() {
        switch (buttonClick) {
        case ButtonClick::randomPolyhedron:
            generateRandomPolyhedron();
            break;
        
        case ButtonClick::resetCamera:
            camera.centerX = 0;
            camera.centerY = 0;
            camera.centerZ = 0;
            camera.angleX = 0;
            camera.angleY = 0;
            camera.boom = 10;
            shouldUpdateCamera = true;
            break;
        
        default:
            break;
        }

        buttonClick = ButtonClick::none;
    }

    void updateInput() {
        auto& input = this->input();
        this->dt = dt;

        mouseDx = input.mouseX - mouseX;
        mouseDy = input.mouseY - mouseY;
        mouseX = input.mouseX;
        mouseY = input.mouseY;

        if (isOnCanvas(input.mouseX, input.mouseY)) {
            float moveSpeed = 5.0f * dt * (camera.boom * 0.1f + 1.0f);

            if (input.keys[GLFW_KEY_W]) {
                camera.centerX -= camera.forwardX * moveSpeed;
                camera.centerY -= camera.forwardY * moveSpeed;
                camera.centerZ -= camera.forwardZ * moveSpeed;
                shouldUpdateCamera = true;
            }
            if (input.keys[GLFW_KEY_S]) {
                camera.centerX += camera.forwardX * moveSpeed;
                camera.centerY += camera.forwardY * moveSpeed;
                camera.centerZ += camera.forwardZ * moveSpeed;
                shouldUpdateCamera = true;
            }
            if (input.keys[GLFW_KEY_A]) {
                camera.centerX -= camera.rightX * moveSpeed;
                camera.centerY -= camera.rightY * moveSpeed;
                camera.centerZ -= camera.rightZ * moveSpeed;
                shouldUpdateCamera = true;
            }
            if (input.keys[GLFW_KEY_D]) {
                camera.centerX += camera.rightX * moveSpeed;
                camera.centerY += camera.rightY * moveSpeed;
                camera.centerZ += camera.rightZ * moveSpeed;
                shouldUpdateCamera = true;
            }

            if (input.scrollOffset != 0) {
                shouldUpdateCamera = true;
                camera.boom -= input.scrollOffset * (camera.boom * 0.1f + 0.2f);
                if (camera.boom < 0.1f) camera.boom = 0.1f;
            }

            holdingOnCanvasMouseBtn0 = input.mouseButtons[0];
            holdingOnCanvasMouseBtn1 = input.mouseButtons[1];
        } else {
            if (!input.mouseButtons[0]) holdingOnCanvasMouseBtn0 = false;
            if (!input.mouseButtons[1]) holdingOnCanvasMouseBtn1 = false;
        }

        if (holdingOnCanvasMouseBtn0 && (mouseDx != 0 || mouseDy != 0)) {
            shouldUpdateCamera = true;
            camera.angleX += mouseDx * (-0.005f);
            camera.angleY = std::clamp(camera.angleY + (float)mouseDy * 0.005f, (float)-M_PI/2.1f, (float)M_PI/2.1f);
        }

        if (holdingOnCanvasMouseBtn1 && (mouseDx != 0 || mouseDy != 0)) {
            shouldUpdateCamera = true;
            float panSpeed = camera.boom * 0.001f;
            camera.centerX += (mouseDx * -panSpeed * camera.rightX + mouseDy * panSpeed * camera.upX);
            camera.centerY += (mouseDx * -panSpeed * camera.rightY + mouseDy * panSpeed * camera.upY);
            camera.centerZ += (mouseDx * -panSpeed * camera.rightZ + mouseDy * panSpeed * camera.upZ);
        }
    }

    void updateCamera() {
        if (shouldUpdateCamera) {
            float cosY = std::cos(camera.angleY);
            float sinY = std::sin(camera.angleY);
            float cosX = std::cos(camera.angleX);
            float sinX = std::sin(camera.angleX);

            camera.forwardX = cosY * sinX;
            camera.forwardY = sinY;
            camera.forwardZ = cosY * cosX;

            camera.upX = -sinY * sinX;
            camera.upY =  cosY;
            camera.upZ = -sinY * cosX;

            camera.rightX = cosX;
            camera.rightY = 0.0f;
            camera.rightZ = -sinX;
        }

        shouldUpdateCamera = false;
    }

    void drawVerticalSplitter(float& leftWidth, float minLeft, float minRight) {
        ImGui::SameLine();

        ImGui::InvisibleButton("##splitter", ImVec2(6.0f, -1.0f));

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (ImGui::IsItemActive()) {
            float delta = ImGui::GetIO().MouseDelta.x;
            leftWidth += delta;
        }

        float total = ImGui::GetContentRegionAvail().x + leftWidth;
        leftWidth = ImClamp(leftWidth, minLeft, total - minRight);
    }

    void panelUI() {
        if (ImGui::Button("Gerar Poliedro Aleatório")) {
            buttonClick = ButtonClick::randomPolyhedron;
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Controle de Câmera", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::DragFloat("Distância (Boom)", &camera.boom, 0.1f, 0.1f, 1000.0f))
                shouldUpdateCamera = true;
            
            if (ImGui::SliderAngle("Ângulo X", &camera.angleX))
                shouldUpdateCamera = true;
            if (ImGui::SliderAngle("Ângulo Y", &camera.angleY, -89.0f, 89.0f))
                shouldUpdateCamera = true;

            ImGui::Separator();
            ImGui::Text("Centro do Alvo:");
            
            float cX = (float)camera.centerX;
            float cY = (float)camera.centerY;
            float cZ = (float)camera.centerZ;

            if (ImGui::DragFloat("X", &cX, 0.1f)) { camera.centerX = cX; shouldUpdateCamera = true; }
            if (ImGui::DragFloat("Y", &cY, 0.1f)) { camera.centerY = cY; shouldUpdateCamera = true; }
            if (ImGui::DragFloat("Z", &cZ, 0.1f)) { camera.centerZ = cZ; shouldUpdateCamera = true; }

            if (ImGui::Button("Resetar Câmera")) {
                buttonClick = ButtonClick::resetCamera;
            }
        }

        ImGui::Separator();
        ImGui::Text("Número de vértices: %zu", mesh.vertexCount());
        ImGui::Text("Número de faces: %zu", mesh.faceCount());
    }

    void setupFBO(int w, int h) {
        if (fbo) {
            glDeleteFramebuffers(1, &fbo);
            glDeleteTextures(1, &fboTexture);
            glDeleteRenderbuffers(1, &rbo);
        }

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Erro: FBO incompleto!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderScene(int w, int h) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, w, h);
        
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)w / (double)h, 0.1, 1000.0);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        gluLookAt(
            camera.centerX + camera.forwardX * camera.boom,
            camera.centerY + camera.forwardY * camera.boom,
            camera.centerZ + camera.forwardZ * camera.boom,
            camera.centerX,
            camera.centerY,
            camera.centerZ,
            camera.upX,
            camera.upY,
            camera.upZ
        );

        // Renderizar faces
        const auto& vertices = mesh.getVertices();
        const auto& faces = mesh.getFaces();

        if (faces.size() > 0 && vertices.size() > 0) {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            
            GLfloat light_pos[] = {5.0f, 10.0f, 5.0f, 1.0f};
            GLfloat light_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
            GLfloat light_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
            GLfloat light_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
            
            glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
            glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
            glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
            
            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glColor3f(0.3f, 0.6f, 0.9f);
            
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            
            glBegin(GL_TRIANGLES);
            
            for (const auto& face : faces) {
                int i0 = face[0];
                int i1 = face[1];
                int i2 = face[2];
                
                if (i0 < (int)vertices.size() && i1 < (int)vertices.size() && i2 < (int)vertices.size()) {
                    const Point3f& v0 = vertices[i0];
                    const Point3f& v1 = vertices[i1];
                    const Point3f& v2 = vertices[i2];
                    
                    // Calcular normal usando Vec3f
                    Vec3f vec0{v0[0], v0[1], v0[2]};
                    Vec3f vec1{v1[0], v1[1], v1[2]};
                    Vec3f vec2{v2[0], v2[1], v2[2]};
                    
                    Vec3f edge1 = vec1 - vec0;
                    Vec3f edge2 = vec2 - vec0;
                    Vec3f normal = edge1.cross(edge2);
                    normal = normal.normalized();
                    
                    glNormal3f(normal[0], normal[1], normal[2]);
                    glVertex3f(v0[0], v0[1], v0[2]);
                    glVertex3f(v1[0], v1[1], v1[2]);
                    glVertex3f(v2[0], v2[1], v2[2]);
                }
            }
            
            glEnd();
            
            glDisable(GL_POLYGON_OFFSET_FILL);
            
            // Renderizar arestas
            glDisable(GL_LIGHTING);
            glLineWidth(1.5f);
            glColor3f(0.0f, 0.0f, 0.0f);
            
            glBegin(GL_LINES);
            
            for (const auto& face : faces) {
                int i0 = face[0];
                int i1 = face[1];
                int i2 = face[2];
                
                if (i0 < (int)vertices.size() && i1 < (int)vertices.size() && i2 < (int)vertices.size()) {
                    const Point3f& v0 = vertices[i0];
                    const Point3f& v1 = vertices[i1];
                    const Point3f& v2 = vertices[i2];
                    
                    glVertex3f(v0[0], v0[1], v0[2]);
                    glVertex3f(v1[0], v1[1], v1[2]);
                    
                    glVertex3f(v1[0], v1[1], v1[2]);
                    glVertex3f(v2[0], v2[1], v2[2]);
                    
                    glVertex3f(v2[0], v2[1], v2[2]);
                    glVertex3f(v0[0], v0[1], v0[2]);
                }
            }
            
            glEnd();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glPopAttrib();
    }

    void drawCanvas() {
        ImVec2 currentOrigin = ImGui::GetCursorScreenPos();
        this->canvasOrigin = currentOrigin;

        ImVec2 currentSize = ImGui::GetContentRegionAvail();

        if (currentSize.x < 1.0f || currentSize.y < 1.0f) return;

        if (currentSize.x != canvasSize.x || currentSize.y != canvasSize.y) {
            canvasSize = currentSize;
            setupFBO((int)canvasSize.x, (int)canvasSize.y);
        }

        renderScene((int)canvasSize.x, (int)canvasSize.y);

        ImGui::Image(
            (ImTextureID)(intptr_t)fboTexture,
            canvasSize,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
    }

    bool isOnCanvas(double x, double y) {
        return x > canvasOrigin.x && x < canvasOrigin.x + canvasSize.x &&
               y > canvasOrigin.y && y < canvasOrigin.y + canvasSize.y;
    }
};

int main() {
    Trabalho01 app;
    app.run(800, 600, "Poliedro Aleatório - Geometria Computacional");
}