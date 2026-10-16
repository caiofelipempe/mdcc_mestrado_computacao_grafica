#include "imgui.h"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
    #include <direct.h>  // Windows para _getcwd
    #define GETCWD _getcwd
    #include <io.h>
    #define ACCESS _access
    
    // Windows: estruturas para listar diretórios
    #include <windows.h>
#else
    #include <unistd.h>   // Linux/Mac para getcwd
    #define GETCWD getcwd
    #include <dirent.h>
    #include <sys/stat.h>
#endif

class FileSelector {
private:
    struct FileItem {
        std::string name;
        bool isDirectory;
    };
    
    std::vector<FileItem> items;  // Lista de arquivos e pastas
    std::string selectedItem;
    std::string lastSelectedItem;
    std::string fileContent;
    bool isOpen;
    std::string currentDirectory;
    std::vector<std::string> pathHistory;  // Para navegação
    bool isDirectorySelected;
    
    // Verifica se o arquivo tem extensão .obj (case insensitive)
    bool IsObjFile(const std::string& filename) {
        // Procura a última ocorrência de '.'
        size_t dotPos = filename.rfind('.');
        if (dotPos == std::string::npos) return false;
        
        // Extrai e converte extensão para minúsculo
        std::string ext = filename.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        return ext == ".obj";
    }
    
    // Verifica se é uma entrada de diretório válida (não é . ou ..)
    bool IsValidDirectoryEntry(const std::string& name) {
        return name != "." && name != "..";
    }
    
    void UpdateFileList() {
        items.clear();
        
#ifdef _WIN32
        // Método Windows usando FindFirstFile/FindNextFile
        std::string searchPath = currentDirectory + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string name = findData.cFileName;
                if (IsValidDirectoryEntry(name)) {
                    FileItem item;
                    item.name = name;
                    item.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    
                    // Se for arquivo, verificar se é .obj
                    if (!item.isDirectory && IsObjFile(name)) {
                        items.push_back(item);
                    }
                    // Se for pasta, adicionar sempre
                    else if (item.isDirectory) {
                        items.push_back(item);
                    }
                }
            } while (FindNextFileA(hFind, &findData) != 0);
            FindClose(hFind);
        }
#else
        // Método Linux/Mac usando dirent.h
        DIR* dir = opendir(currentDirectory.c_str());
        if (dir != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string name = entry->d_name;
                if (IsValidDirectoryEntry(name)) {
                    FileItem item;
                    item.name = name;
                    
                    // Verifica se é diretório
                    struct stat statbuf;
                    std::string fullPath = currentDirectory + "/" + name;
                    if (stat(fullPath.c_str(), &statbuf) == 0) {
                        item.isDirectory = S_ISDIR(statbuf.st_mode);
                        
                        // Se for arquivo, verificar se é .obj
                        if (!item.isDirectory && IsObjFile(name)) {
                            items.push_back(item);
                        }
                        // Se for pasta, adicionar sempre
                        else if (item.isDirectory) {
                            items.push_back(item);
                        }
                    }
                }
            }
            closedir(dir);
        }
#endif
        
        // Ordenar: pastas primeiro, depois arquivos
        std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
            if (a.isDirectory != b.isDirectory) {
                return a.isDirectory > b.isDirectory; // Pastas primeiro
            }
            // Ordem alfabética case insensitive
            std::string aLower = a.name;
            std::string bLower = b.name;
            std::transform(aLower.begin(), aLower.end(), aLower.begin(), ::tolower);
            std::transform(bLower.begin(), bLower.end(), bLower.begin(), ::tolower);
            return aLower < bLower;
        });
    }
    
    void NavigateTo(const std::string& directory) {
        // Salva no histórico
        pathHistory.push_back(currentDirectory);
        
        currentDirectory = directory;
        UpdateFileList();
        selectedItem.clear();
        isDirectorySelected = false;
    }
    
    void GoBack() {
        if (!pathHistory.empty()) {
            currentDirectory = pathHistory.back();
            pathHistory.pop_back();
            UpdateFileList();
            selectedItem.clear();
            isDirectorySelected = false;
        }
    }
    
public:
    FileSelector() {
        // Obtém o diretório atual
        char buffer[1024];
        if (GETCWD(buffer, sizeof(buffer)) != nullptr) {
            currentDirectory = buffer;
        } else {
            currentDirectory = ".";
        }
        
        UpdateFileList();
        isOpen = false;
        selectedItem = "";
        isDirectorySelected = false;
    }
    
    void Draw() {
        if (!isOpen) return;
        
        ImGui::Begin("Selecionar Arquivo .OBJ", &isOpen, ImGuiWindowFlags_AlwaysAutoResize);
        
        // Barra de navegação
        ImGui::Text("Diretório atual:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.2f, 1.0f), "%s", currentDirectory.c_str());
        
        ImGui::Separator();
        
        // Botões de navegação
        if (ImGui::Button("Voltar")) {
            GoBack();
        }
        ImGui::SameLine();
        if (ImGui::Button("Atualizar")) {
            UpdateFileList();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Apenas arquivos .obj)");
        
        ImGui::Separator();
        
        // Lista de arquivos e pastas
        ImGui::Text("Conteúdo:");
        ImGui::BeginChild("FileList", ImVec2(500, 400), true, ImGuiWindowFlags_HorizontalScrollbar);
        
        if (items.empty()) {
            ImGui::TextDisabled("Nenhum arquivo .obj encontrado neste diretório");
        }
        
        for (size_t i = 0; i < items.size(); i++) {
            const auto& item = items[i];
            
            if (item.isDirectory) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f)); // Azul para pastas
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.6f, 1.0f)); // Amarelo para arquivos .obj
            }
            if (ImGui::Selectable((item.name).c_str(), selectedItem == item.name)) {
                selectedItem = item.name;
                isDirectorySelected = item.isDirectory;

                if (lastSelectedItem == selectedItem) {
                    if(item.isDirectory) {
                        std::string newPath;
#ifdef _WIN32
                        newPath = currentDirectory + "\\" + item.name;
#else
                        newPath = currentDirectory + "/" + item.name;
#endif
                        NavigateTo(newPath);
                    } else {
                        ReadFile();
                    }
                }
                
                lastSelectedItem = selectedItem;
            }
            ImGui::PopStyleColor();
            
            // Tooltip para mostrar caminho completo
            if (ImGui::IsItemHovered()) {
                std::string fullPath;
#ifdef _WIN32
                fullPath = currentDirectory + "\\" + item.name;
#else
                fullPath = currentDirectory + "/" + item.name;
#endif
                ImGui::BeginTooltip();
                ImGui::Text("%s", fullPath.c_str());
                if (item.isDirectory) {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Clique em 'Abrir' para entrar na pasta");
                }
                ImGui::EndTooltip();
            }
        }
        
        ImGui::EndChild();
        
        ImGui::Separator();
        
        // Informação do item selecionado
        if (!selectedItem.empty()) {
            if (isDirectorySelected) {
                ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 1.0f), "Pasta selecionada: %s", selectedItem.c_str());
                ImGui::TextDisabled("Clique em 'Abrir Pasta' para entrar");
                
                if (ImGui::Button("Abrir Pasta")) {
                    std::string newPath;
#ifdef _WIN32
                    newPath = currentDirectory + "\\" + selectedItem;
#else
                    newPath = currentDirectory + "/" + selectedItem;
#endif
                    NavigateTo(newPath);
                }
            } else {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Arquivo selecionado: %s", selectedItem.c_str());
                
                if (ImGui::Button("Ler Arquivo .OBJ")) {
                    ReadFile();
                }
                ImGui::SameLine();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancelar")) {
                isOpen = false;
            }
        } else {
            if (ImGui::Button("Cancelar")) {
                isOpen = false;
            }
        }
        
        // Botão para subir um nível
        ImGui::SameLine();
        if (ImGui::Button("Subir um nível")) {
            std::string parentPath;
#ifdef _WIN32
            size_t pos = currentDirectory.find_last_of("\\");
            if (pos != std::string::npos) {
                parentPath = currentDirectory.substr(0, pos);
            } else {
                parentPath = currentDirectory;
            }
#else
            size_t pos = currentDirectory.find_last_of("/");
            if (pos != std::string::npos) {
                parentPath = currentDirectory.substr(0, pos);
            } else {
                parentPath = currentDirectory;
            }
#endif
            if (parentPath != currentDirectory) {
                NavigateTo(parentPath);
            }
        }
        
        ImGui::End();
    }
    
    void ReadFile() {
        if (selectedItem.empty() || isDirectorySelected) return;
        
        std::string fullPath;
        
#ifdef _WIN32
        fullPath = currentDirectory + "\\" + selectedItem;
#else
        fullPath = currentDirectory + "/" + selectedItem;
#endif
        
        std::ifstream file(fullPath);
        
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            fileContent = buffer.str();
            file.close();
            
            // Estatísticas básicas do OBJ
            int vertexCount = 0;
            int faceCount = 0;
            std::istringstream iss(fileContent);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.size() >= 2) {
                    if (line[0] == 'v' && line[1] == ' ') vertexCount++;
                    else if (line[0] == 'f' && line[1] == ' ') faceCount++;
                }
            }
        } else {
            fileContent = "Erro: Não foi possível ler o arquivo .OBJ!";
        }
        
        isOpen = false;
    }
    
    void Open() {
        selectedItem.clear();
        isOpen = true;
        UpdateFileList();  // Atualiza a lista ao abrir
    }
    
    const std::string& GetContent() const {
        return fileContent;
    }
    
    void ClearContent() {
        fileContent = "";
    }
    
    bool IsOpen() const {
        return isOpen;
    }
    
    // Retorna o conteúdo como vetor de linhas (útil para processamento OBJ)
    std::vector<std::string> GetLines() const {
        std::vector<std::string> lines;
        std::istringstream iss(fileContent);
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        return lines;
    }
};