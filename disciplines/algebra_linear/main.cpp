#include "renderer.hpp"
#include "input.h"
#include "vector.hpp"
#include "utils.hpp"

using namespace geometry;

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

#include <numbers>
#include <random>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include "renderer.hpp"
#include <imgui.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

class AlgebraLinear : public Renderer {
public:

protected:
    void onInit(int w, int h, const std::string&) override {}
    void onWindowResize(int, int) override {}
    void onUpdate(float) override {}

    void onUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin("Calculadora de Determinante", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse
        );

        ImGui::Text("Cálculo de Determinante de Matriz N x N");
        ImGui::Separator();

        if (ImGui::SliderInt("Ordem da Matriz (N)", &matrixSize, 1, 10)) {
            resizeMatrix(matrixSize);
        }

        if (ImGui::Button("Zerar Matriz")) setZeros();
        ImGui::SameLine();
        if (ImGui::Button("Matriz Identidade")) setIdentity();
        ImGui::SameLine();
        if (ImGui::Button("Valores Aleatórios")) setRandom();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Entradas da Matriz:");
        ImGui::Spacing();

        if (ImGui::BeginTable("MatrixTable", matrixSize, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame)) {
            for (int i = 0; i < matrixSize; ++i) {
                ImGui::TableNextRow();
                for (int j = 0; j < matrixSize; ++j) {
                    ImGui::TableSetColumnIndex(j);
                    ImGui::PushID(i * matrixSize + j);

                    // Largura compacta para cada célula da matriz
                    ImGui::SetNextItemWidth(70.0f);
                    ImGui::InputDouble("##cell", &matrix[i][j], 0.0, 0.0, "%.2f");

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 3. Exibição do Resultado
        double det = calculateDeterminant();
        ImGui::Text("Determinante: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%.4f", det);

        if (std::abs(det) < 1e-9) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "(A matriz é singular / não invertível)");
        }

        ImGui::End();
    }

private:
    int matrixSize = 3;
    std::vector<std::vector<double>> matrix = {
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}
    };

    void resizeMatrix(int newSize) {
        matrix.assign(newSize, std::vector<double>(newSize, 0.0));
        for (int i = 0; i < newSize; ++i) {
            matrix[i][i] = 1.0;
        }
    }

    void setZeros() {
        for (auto& row : matrix) {
            std::fill(row.begin(), row.end(), 0.0);
        }
    }

    void setIdentity() {
        setZeros();
        for (int i = 0; i < matrixSize; ++i) {
            matrix[i][i] = 1.0;
        }
    }

    void setRandom() {
        for (int i = 0; i < matrixSize; ++i) {
            for (int j = 0; j < matrixSize; ++j) {
                matrix[i][j] = (std::rand() % 21) - 10.0;
            }
        }
    }

    double calculateDeterminant() const {
        int n = matrixSize;
        auto mat = matrix;
        double det = 1.0;

        for (int i = 0; i < n; ++i) {
            int pivot = i;
            for (int j = i + 1; j < n; ++j) {
                if (std::abs(mat[j][i]) > std::abs(mat[pivot][i])) {
                    pivot = j;
                }
            }

            if (std::abs(mat[pivot][i]) < 1e-9) {
                return 0.0;
            }

            if (i != pivot) {
                std::swap(mat[i], mat[pivot]);
                det = -det;
            }

            det *= mat[i][i];

            for (int j = i + 1; j < n; ++j) {
                double factor = mat[j][i] / mat[i][i];
                for (int k = i + 1; k < n; ++k) {
                    mat[j][k] -= factor * mat[i][k];
                }
            }
        }

        return det;
    }
};

// ─────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────
int main() {
    AlgebraLinear app;
    app.run(800, 600, "MDCC - Álgebra Linear");
}