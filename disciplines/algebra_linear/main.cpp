#include "renderer.hpp"
#include "input.h"
#include "utils.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

#include <iomanip>
#include <vector.hpp>

// ============================================================
// CLASSE BASE - ATIVIDADE (ABSTRATA)
// ============================================================
class Activity {
public:
    virtual ~Activity() = default;
    
    virtual const char* getTitle() const = 0;
    virtual void render() = 0;
};

// ============================================================
// CLASSE ATIVIDADE MATRIZ - HERDA DE ATIVIDADE
// ============================================================
class MatrixActivity : public Activity {
public:
    virtual ~MatrixActivity() = default;
    
    // Metodos especificos para matriz
    virtual void onMatrixModified() = 0;
    virtual std::vector<std::vector<double>>& getMatrix() = 0;
    virtual std::vector<double>& getVectorB() = 0;
    virtual int getSize() const = 0;
    virtual void setSize(int newSize) = 0;
    
    // Metodos para resolucao
    virtual bool solveGauss(std::vector<double>& x) const = 0;
    virtual void solveSystem() = 0;
};

// ============================================================
// CLASSE ATIVIDADE 1 - PIVOTEAMENTO TOTAL
// ============================================================
class TotalPivotActivity : public MatrixActivity {
private:
    int matrixSize = 3;
    std::vector<std::vector<double>> matrix;
    std::vector<double> vectorB;
    
    std::vector<double> solution;
    bool hasSolution = false;
    bool solutionComputed = false;
    double maxError = 0.0;

public:
    TotalPivotActivity() {
        initExampleSystem();
    }

    const char* getTitle() const override {
        return "Atividade 1 - Pivotamento Total";
    }

    std::vector<std::vector<double>>& getMatrix() override {
        return matrix;
    }

    std::vector<double>& getVectorB() override {
        return vectorB;
    }

    int getSize() const override {
        return matrixSize;
    }

    void setSize(int newSize) override {
        if (newSize >= 2 && newSize <= 20) {
            matrixSize = newSize;
            LinearAlgebraUtils::identityMatrix(matrix, matrixSize);
            vectorB.assign(matrixSize, 0.0);
            clearSolution();
        }
    }

    void onMatrixModified() override {
        clearSolution();
    }

    void render() override {
        ImGui::BeginChild("ScrollTotal1", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "%s", getTitle());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        renderControls();
        renderMatrixAndVector();
        renderButtons();
        renderResult();
        
        ImGui::EndChild();
    }

    bool solveGauss(std::vector<double>& x) const override {
        return LinearAlgebraUtils::solveTotalPivot(matrix, vectorB, x);
    }

    void solveSystem() override {
        hasSolution = LinearAlgebraUtils::solveTotalPivot(matrix, vectorB, solution);
        solutionComputed = true;
        
        if (hasSolution) {
            maxError = LinearAlgebraUtils::computeError(matrix, vectorB, solution);
        }
    }

private:
    void initExampleSystem() {
        matrixSize = 3;
        matrix = {
            {2.0, 1.0, -1.0},
            {-3.0, -1.0, 2.0},
            {-2.0, 1.0, 2.0}
        };
        vectorB = {8.0, -11.0, -3.0};
    }

    void clearSolution() {
        solutionComputed = false;
        hasSolution = false;
        solution.clear();
        maxError = 0.0;
    }

    void renderControls() {
        ImGui::Text("Tamanho da Matriz:");
        ImGui::SameLine();
        
        if (ImGui::Button("-##size1")) {
            if (matrixSize > 2) {
                setSize(matrixSize - 1);
            }
        }
        ImGui::SameLine();
        ImGui::Text("%d", matrixSize);
        ImGui::SameLine();
        if (ImGui::Button("+##size1")) {
            if (matrixSize < 20) {
                setSize(matrixSize + 1);
            }
        }
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        
        if (ImGui::Button("Gerar Sistema Aleatorio")) {
            generateRandomSystem();
        }
        ImGui::SameLine();
        if (ImGui::Button("Resetar Exemplo")) {
            initExampleSystem();
            clearSolution();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void generateRandomSystem() {
        LinearAlgebraUtils::generateRandomSystem(matrix, vectorB, matrixSize);
        clearSolution();
    }

    void renderMatrixAndVector() {
        float fieldWidth = 70.0f;
        
        if (matrixSize > 10) {
            fieldWidth = 50.0f;
        } else if (matrixSize > 6) {
            fieldWidth = 60.0f;
        }
        
        ImGui::Text("Matriz A (Coeficientes):");
        
        for (int i = 0; i < matrixSize; ++i) {
            ImGui::Text("  Linha %d:", i + 1);
            ImGui::SameLine();
            
            for (int j = 0; j < matrixSize; ++j) {
                ImGui::PushID(i * matrixSize + j + 1000);
                ImGui::SetNextItemWidth(fieldWidth);
                std::string label = "##A" + std::to_string(i) + std::to_string(j);
                if (ImGui::InputDouble(label.c_str(), &matrix[i][j], 0.0, 0.0, "%.2f")) {
                    onMatrixModified();
                }
                ImGui::PopID();
                
                if (j < matrixSize - 1) {
                    ImGui::SameLine();
                }
            }
        }
        
        ImGui::Spacing();
        
        ImGui::Text("Vetor b (Termos Independentes):");
        ImGui::SameLine();
        for (int i = 0; i < matrixSize; ++i) {
            ImGui::PushID(i + 2000);
            ImGui::SetNextItemWidth(fieldWidth);
            std::string label = "b[" + std::to_string(i) + "]";
            if (ImGui::InputDouble(label.c_str(), &vectorB[i], 0.0, 0.0, "%.2f")) {
                onMatrixModified();
            }
            ImGui::PopID();
            
            if (i < matrixSize - 1) {
                ImGui::SameLine();
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void renderButtons() {
        if (ImGui::Button("Resolver Sistema")) {
            solveSystem();
        }
        
        if (solutionComputed) {
            ImGui::SameLine();
            if (ImGui::Button("Limpar Resultado")) {
                clearSolution();
            }
        }
        ImGui::Spacing();
    }

    void renderResult() {
        if (!solutionComputed) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                "Clique em 'Resolver Sistema' para calcular a solucao.");
            return;
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "RESULTADO");
        ImGui::Spacing();
        
        if (hasSolution) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), 
                "Sistema resolvido com sucesso!");
            ImGui::Spacing();
            
            ImGui::Text("Vetor Solucao X:");
            
            if (ImGui::BeginTable("ResultTable1", 2, 
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame)) {
                
                ImGui::TableSetupColumn("Variavel", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Valor", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (int i = 0; i < matrixSize; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("x%d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "%.8f", solution[i]);
                }
                ImGui::EndTable();
            }
            
            ImGui::Spacing();
            ImGui::Text("Verificacao: A*x ~= b");
            
            if (maxError < 1e-6) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), 
                    "Solucao verificada! Erro maximo: %.2e", maxError);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), 
                    "Atencao: Erro residual maximo = %.6f", maxError);
            }
            
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), 
                "O sistema nao possui solucao unica!");
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), 
                "Motivo: Matriz singular ou indeterminada.");
        }
    }
};

// ============================================================
// CLASSE ATIVIDADE 2 - PIVOTEAMENTO PARCIAL
// ============================================================
class PartialPivotActivity : public MatrixActivity {
private:
    int matrixSize = 3;
    std::vector<std::vector<double>> matrix;
    std::vector<double> vectorB;
    
    std::vector<double> solution;
    bool hasSolution = false;
    bool solutionComputed = false;
    double maxError = 0.0;

public:
    PartialPivotActivity() {
        initExampleSystem();
    }

    const char* getTitle() const override {
        return "Atividade 2 - Pivotamento Parcial";
    }

    std::vector<std::vector<double>>& getMatrix() override {
        return matrix;
    }

    std::vector<double>& getVectorB() override {
        return vectorB;
    }

    int getSize() const override {
        return matrixSize;
    }

    void setSize(int newSize) override {
        if (newSize >= 2 && newSize <= 20) {
            matrixSize = newSize;
            LinearAlgebraUtils::identityMatrix(matrix, matrixSize);
            vectorB.assign(matrixSize, 0.0);
            clearSolution();
        }
    }

    void onMatrixModified() override {
        clearSolution();
    }

    void render() override {
        ImGui::BeginChild("ScrollTotal2", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), "%s", getTitle());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        renderControls();
        renderMatrixAndVector();
        renderButtons();
        renderResult();
        
        ImGui::EndChild();
    }

    bool solveGauss(std::vector<double>& x) const override {
        return LinearAlgebraUtils::solvePartialPivot(matrix, vectorB, x);
    }

    void solveSystem() override {
        hasSolution = LinearAlgebraUtils::solvePartialPivot(matrix, vectorB, solution);
        solutionComputed = true;
        
        if (hasSolution) {
            maxError = LinearAlgebraUtils::computeError(matrix, vectorB, solution);
        }
    }

private:
    void initExampleSystem() {
        matrixSize = 3;
        matrix = {
            {2.0, 1.0, -1.0},
            {-3.0, -1.0, 2.0},
            {-2.0, 1.0, 2.0}
        };
        vectorB = {8.0, -11.0, -3.0};
    }

    void clearSolution() {
        solutionComputed = false;
        hasSolution = false;
        solution.clear();
        maxError = 0.0;
    }

    void renderControls() {
        ImGui::Text("Tamanho da Matriz:");
        ImGui::SameLine();
        
        if (ImGui::Button("-##size2")) {
            if (matrixSize > 2) {
                setSize(matrixSize - 1);
            }
        }
        ImGui::SameLine();
        ImGui::Text("%d", matrixSize);
        ImGui::SameLine();
        if (ImGui::Button("+##size2")) {
            if (matrixSize < 20) {
                setSize(matrixSize + 1);
            }
        }
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        
        if (ImGui::Button("Gerar Sistema Aleatorio")) {
            generateRandomSystem();
        }
        ImGui::SameLine();
        if (ImGui::Button("Resetar Exemplo")) {
            initExampleSystem();
            clearSolution();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void generateRandomSystem() {
        LinearAlgebraUtils::generateRandomSystem(matrix, vectorB, matrixSize);
        clearSolution();
    }

    void renderMatrixAndVector() {
        float fieldWidth = 70.0f;
        
        if (matrixSize > 10) {
            fieldWidth = 50.0f;
        } else if (matrixSize > 6) {
            fieldWidth = 60.0f;
        }
        
        ImGui::Text("Matriz A (Coeficientes):");
        
        for (int i = 0; i < matrixSize; ++i) {
            ImGui::Text("  Linha %d:", i + 1);
            ImGui::SameLine();
            
            for (int j = 0; j < matrixSize; ++j) {
                ImGui::PushID(i * matrixSize + j + 3000);
                ImGui::SetNextItemWidth(fieldWidth);
                std::string label = "##A" + std::to_string(i) + std::to_string(j);
                if (ImGui::InputDouble(label.c_str(), &matrix[i][j], 0.0, 0.0, "%.2f")) {
                    onMatrixModified();
                }
                ImGui::PopID();
                
                if (j < matrixSize - 1) {
                    ImGui::SameLine();
                }
            }
        }
        
        ImGui::Spacing();
        
        ImGui::Text("Vetor b (Termos Independentes):");
        ImGui::SameLine();
        for (int i = 0; i < matrixSize; ++i) {
            ImGui::PushID(i + 4000);
            ImGui::SetNextItemWidth(fieldWidth);
            std::string label = "b[" + std::to_string(i) + "]";
            if (ImGui::InputDouble(label.c_str(), &vectorB[i], 0.0, 0.0, "%.2f")) {
                onMatrixModified();
            }
            ImGui::PopID();
            
            if (i < matrixSize - 1) {
                ImGui::SameLine();
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void renderButtons() {
        if (ImGui::Button("Resolver Sistema")) {
            solveSystem();
        }
        
        if (solutionComputed) {
            ImGui::SameLine();
            if (ImGui::Button("Limpar Resultado")) {
                clearSolution();
            }
        }
        ImGui::Spacing();
    }

    void renderResult() {
        if (!solutionComputed) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                "Clique em 'Resolver Sistema' para calcular a solucao.");
            return;
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "RESULTADO");
        ImGui::Spacing();
        
        if (hasSolution) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), 
                "Sistema resolvido com sucesso!");
            ImGui::Spacing();
            
            ImGui::Text("Vetor Solucao X:");
            
            if (ImGui::BeginTable("ResultTable2", 2, 
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame)) {
                
                ImGui::TableSetupColumn("Variavel", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Valor", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (int i = 0; i < matrixSize; ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("x%d", i + 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "%.8f", solution[i]);
                }
                ImGui::EndTable();
            }
            
            ImGui::Spacing();
            ImGui::Text("Verificacao: A*x ~= b");
            
            if (maxError < 1e-6) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), 
                    "Solucao verificada! Erro maximo: %.2e", maxError);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), 
                    "Atencao: Erro residual maximo = %.6f", maxError);
            }
            
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), 
                "O sistema nao possui solucao unica!");
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), 
                "Motivo: Matriz singular ou indeterminada.");
        }
    }
};

// ============================================================
// CLASSE PRINCIPAL - GERENCIA AS ATIVIDADES
// ============================================================
class AlgebraLinear : public Renderer {
public:
    AlgebraLinear() {
        activities.push_back(std::make_unique<TotalPivotActivity>());
        activities.push_back(std::make_unique<PartialPivotActivity>());
    }

protected:
    void onInit(int w, int h, const std::string&) override {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        
        activityNames.clear();
        for (const auto& activity : activities) {
            activityNames.push_back(activity->getTitle());
        }
    }
    
    void onWindowResize(int, int) override {}
    void onUpdate(float) override {}

    void onUI() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin("Calculadora de Sistemas Lineares", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse
        );

        ImGui::BeginChild("ScrollGeral", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        ImGui::Text("Selecione a Atividade:");
        
        ImGui::SetNextItemWidth(500.0f);
        ImGui::Combo("##SeletorAtividade", &selectedActivity, 
            activityNames.data(), static_cast<int>(activityNames.size()));
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (selectedActivity >= 0 && selectedActivity < activities.size()) {
            activities[selectedActivity]->render();
        }
        
        ImGui::EndChild();
        ImGui::End();
    }

private:
    std::vector<std::unique_ptr<Activity>> activities;
    std::vector<const char*> activityNames;
    int selectedActivity = 0;
};

// ============================================================
//  ENTRY POINT
// ============================================================
int main() {
    AlgebraLinear app;
    app.run(1000, 750, "MDCC - Algebra Linear: Eliminacao de Gauss");
}