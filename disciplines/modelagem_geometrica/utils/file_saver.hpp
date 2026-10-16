#pragma once

#include "imgui.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
    #include <direct.h>
    #define GETCWD _getcwd
    #include <io.h>
    #define ACCESS _access

    #include <windows.h>
#else
    #include <unistd.h>
    #define GETCWD getcwd

    #include <dirent.h>
    #include <sys/stat.h>
#endif

class FileSaver {
private:
    struct FileItem {
        std::string name;
        bool isDirectory;
    };

    std::vector<FileItem> items;

    std::string selectedItem;
    std::string lastSelectedItem;

    std::string currentDirectory;
    std::vector<std::string> pathHistory;

    bool isDirectorySelected = false;
    bool isOpen = false;

    char fileNameBuffer[256]{};

    std::string selectedPath;
    bool hasSelected = false;

private:
    static bool IsValidDirectoryEntry(const std::string& name) {
        return name != "." && name != "..";
    }

    static bool IsObjFile(const std::string& filename) {
        size_t dotPos = filename.rfind('.');

        if (dotPos == std::string::npos)
            return false;

        std::string ext = filename.substr(dotPos);

        std::transform(
            ext.begin(),
            ext.end(),
            ext.begin(),
            ::tolower
        );

        return ext == ".obj";
    }

    void UpdateFileList() {
        items.clear();

#ifdef _WIN32

        std::string searchPath =
            currentDirectory + "\\*";

        WIN32_FIND_DATAA findData;

        HANDLE hFind =
            FindFirstFileA(
                searchPath.c_str(),
                &findData
            );

        if (hFind != INVALID_HANDLE_VALUE) {

            do {

                std::string name =
                    findData.cFileName;

                if (!IsValidDirectoryEntry(name))
                    continue;

                FileItem item;

                item.name = name;

                item.isDirectory =
                    (findData.dwFileAttributes &
                     FILE_ATTRIBUTE_DIRECTORY) != 0;

                if (item.isDirectory || IsObjFile(name))
                    items.push_back(item);

            } while (
                FindNextFileA(hFind, &findData) != 0
            );

            FindClose(hFind);
        }

#else

        DIR* dir =
            opendir(currentDirectory.c_str());

        if (dir != nullptr) {

            struct dirent* entry;

            while ((entry = readdir(dir)) != nullptr) {

                std::string name =
                    entry->d_name;

                if (!IsValidDirectoryEntry(name))
                    continue;

                FileItem item;

                item.name = name;

                struct stat statbuf;

                std::string fullPath =
                    currentDirectory + "/" + name;

                if (stat(fullPath.c_str(), &statbuf) != 0)
                    continue;

                item.isDirectory =
                    S_ISDIR(statbuf.st_mode);

                if (item.isDirectory || IsObjFile(name))
                    items.push_back(item);
            }

            closedir(dir);
        }

#endif

        std::sort(
            items.begin(),
            items.end(),
            [](const FileItem& a,
               const FileItem& b) {

                if (a.isDirectory != b.isDirectory)
                    return a.isDirectory > b.isDirectory;

                std::string aLower = a.name;
                std::string bLower = b.name;

                std::transform(
                    aLower.begin(),
                    aLower.end(),
                    aLower.begin(),
                    ::tolower
                );

                std::transform(
                    bLower.begin(),
                    bLower.end(),
                    bLower.begin(),
                    ::tolower
                );

                return aLower < bLower;
            }
        );
    }

    void NavigateTo(const std::string& directory) {
        pathHistory.push_back(currentDirectory);

        currentDirectory = directory;

        selectedItem.clear();
        lastSelectedItem.clear();

        isDirectorySelected = false;

        UpdateFileList();
    }

    void GoBack() {
        if (pathHistory.empty())
            return;

        currentDirectory =
            pathHistory.back();

        pathHistory.pop_back();

        selectedItem.clear();
        lastSelectedItem.clear();

        isDirectorySelected = false;

        UpdateFileList();
    }

    std::string BuildPath(const std::string& file) const {

#ifdef _WIN32
        return currentDirectory + "\\" + file;
#else
        return currentDirectory + "/" + file;
#endif
    }

    void ConfirmSelection() {

        if (std::strlen(fileNameBuffer) == 0)
            return;

        std::string finalPath =
            BuildPath(fileNameBuffer);

        if (!IsObjFile(finalPath))
            finalPath += ".obj";

        selectedPath = finalPath;

        hasSelected = true;
        isOpen = false;
    }

public:
    FileSaver() {

        char buffer[1024];

        if (GETCWD(buffer, sizeof(buffer)) != nullptr)
            currentDirectory = buffer;
        else
            currentDirectory = ".";

        UpdateFileList();
    }

    void Open() {

        hasSelected = false;

        selectedPath.clear();

        selectedItem.clear();
        lastSelectedItem.clear();

        isDirectorySelected = false;

        std::memset(
            fileNameBuffer,
            0,
            sizeof(fileNameBuffer)
        );

        UpdateFileList();

        isOpen = true;
    }

    bool HasSelected() const {
        return hasSelected;
    }

    std::string GetSelectedPath() {

        hasSelected = false;

        return selectedPath;
    }

    bool IsOpen() const {
        return isOpen;
    }

    void Draw() {

        if (!isOpen)
            return;

        ImGui::Begin(
            "Salvar Arquivo .OBJ",
            &isOpen,
            ImGuiWindowFlags_AlwaysAutoResize
        );

        ImGui::Text("Diretório atual:");

        ImGui::SameLine();

        ImGui::TextColored(
            ImVec4(0.2f, 0.7f, 0.2f, 1.0f),
            "%s",
            currentDirectory.c_str()
        );

        ImGui::Separator();

        if (ImGui::Button("Voltar"))
            GoBack();

        ImGui::SameLine();

        if (ImGui::Button("Atualizar"))
            UpdateFileList();

        ImGui::SameLine();

        ImGui::TextDisabled(
            "(Arquivos .obj)"
        );

        ImGui::Separator();

        ImGui::Text("Conteúdo:");

        ImGui::BeginChild(
            "FileList",
            ImVec2(500, 400),
            true,
            ImGuiWindowFlags_HorizontalScrollbar
        );

        if (items.empty()) {

            ImGui::TextDisabled(
                "Nenhum arquivo/pasta encontrado"
            );
        }

        for (const auto& item : items) {

            if (item.isDirectory) {

                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImVec4(
                        1.0f,
                        0.8f,
                        0.2f,
                        1.0f
                    )
                );

            } else {

                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImVec4(
                        0.3f,
                        1.0f,
                        0.6f,
                        1.0f
                    )
                );
            }

            if (ImGui::Selectable(
                    item.name.c_str(),
                    selectedItem == item.name
                )) {

                selectedItem = item.name;

                isDirectorySelected =
                    item.isDirectory;

                if (selectedItem ==
                    lastSelectedItem) {

                    if (item.isDirectory) {

                        NavigateTo(
                            BuildPath(item.name)
                        );

                    } else {

                        std::strncpy(
                            fileNameBuffer,
                            item.name.c_str(),
                            sizeof(fileNameBuffer) - 1
                        );
                    }
                }

                lastSelectedItem =
                    selectedItem;

                if (!item.isDirectory) {

                    std::strncpy(
                        fileNameBuffer,
                        item.name.c_str(),
                        sizeof(fileNameBuffer) - 1
                    );
                }
            }

            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) {

                ImGui::BeginTooltip();

                ImGui::Text(
                    "%s",
                    BuildPath(item.name).c_str()
                );

                if (item.isDirectory) {

                    ImGui::TextColored(
                        ImVec4(
                            0.5f,
                            0.5f,
                            0.5f,
                            1.0f
                        ),
                        "Clique duas vezes para entrar"
                    );
                }

                ImGui::EndTooltip();
            }
        }

        ImGui::EndChild();

        ImGui::Separator();

        ImGui::InputText(
            "Nome do arquivo",
            fileNameBuffer,
            sizeof(fileNameBuffer)
        );

        if (!selectedItem.empty()) {

            if (isDirectorySelected) {

                ImGui::TextColored(
                    ImVec4(
                        0.3f,
                        0.5f,
                        1.0f,
                        1.0f
                    ),
                    "Pasta selecionada: %s",
                    selectedItem.c_str()
                );

                if (ImGui::Button("Abrir Pasta")) {

                    NavigateTo(
                        BuildPath(selectedItem)
                    );
                }

            } else {

                ImGui::TextColored(
                    ImVec4(
                        0.8f,
                        0.8f,
                        0.2f,
                        1.0f
                    ),
                    "Arquivo selecionado: %s",
                    selectedItem.c_str()
                );
            }
        }

        if (ImGui::Button("Salvar")) {
            ConfirmSelection();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancelar")) {
            isOpen = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("Subir um nível")) {

#ifdef _WIN32

            size_t pos =
                currentDirectory.find_last_of("\\");

#else

            size_t pos =
                currentDirectory.find_last_of("/");

#endif

            if (pos != std::string::npos) {

                std::string parent =
                    currentDirectory.substr(
                        0,
                        pos
                    );

                if (!parent.empty())
                    NavigateTo(parent);
            }
        }

        ImGui::End();
    }
};