#pragma once
#include <imgui.h>
#include "Engine.h"
#include "transform.h"
#include "tags.h"
#include "Time.h"
#include "Camera.h"
#include <vector>

class DebugUI {
public:
    bool showPerformance = true;
    bool showEntityList = true;
    bool showEntityInspector = true;
    bool showCameraInfo = true;
    bool showDemo = false;
    
    EntityID selectedEntity = 0;
    
    void render(ECS* ecs, Camera* camera, uint32_t entityCount, uint32_t drawCalls) {
        renderMainMenuBar();
        
        if (showPerformance) renderPerformanceWindow(entityCount, drawCalls);
        if (showEntityList) renderEntityListWindow(ecs);
        if (showEntityInspector) renderEntityInspectorWindow(ecs);
        if (showCameraInfo) renderCameraWindow(camera);
        if (showDemo) ImGui::ShowDemoWindow(&showDemo);
    }
    
private:
    void renderMainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Performance", nullptr, &showPerformance);
                ImGui::MenuItem("Entity List", nullptr, &showEntityList);
                ImGui::MenuItem("Inspector", nullptr, &showEntityInspector);
                ImGui::MenuItem("Camera", nullptr, &showCameraInfo);
                ImGui::Separator();
                ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
    
    void renderPerformanceWindow(uint32_t entityCount, uint32_t drawCalls) {
        ImGui::Begin("Performance", &showPerformance);
        
        ImGui::Text("FPS: %.1f (%.2f ms)", Time::getFPS(), Time::getDeltaTime() * 1000.0f);
        ImGui::Text("Entities: %u", entityCount);
        ImGui::Text("Draw Calls: %u", drawCalls);
        
        ImGui::Separator();
        
        static float fpsHistory[100] = {};
        static int historyIndex = 0;
        fpsHistory[historyIndex] = Time::getFPS();
        historyIndex = (historyIndex + 1) % 100;
        
        ImGui::PlotLines("FPS", fpsHistory, 100, historyIndex, nullptr, 0.0f, 120.0f, ImVec2(0, 80));
        
        ImGui::End();
    }
    
    void renderEntityListWindow(ECS* ecs) {
        ImGui::Begin("Entity List", &showEntityList);
        
        ImGui::Text("Click to select entity");
        ImGui::Separator();
        
        if (ImGui::BeginChild("EntityScroll")) {
            for (size_t i = 0; i < 10000; i++) {
                auto* tag = ecs->getComponent<Tag>(i);
                auto* transform = ecs->getComponent<Transform>(i);
                
                if (!tag && !transform) continue;
                
                std::string label = tag ? tag->name : "Entity";
                label += " [" + std::to_string(i) + "]";
                
                if (ImGui::Selectable(label.c_str(), selectedEntity == i)) {
                    selectedEntity = i;
                }
            }
        }
        ImGui::EndChild();
        
        ImGui::End();
    }
    
    void renderEntityInspectorWindow(ECS* ecs) {
        ImGui::Begin("Inspector", &showEntityInspector);
        
        if (selectedEntity == 0) {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }
        
        ImGui::Text("Entity ID: %u", selectedEntity);
        ImGui::Separator();
        
        auto* tag = ecs->getComponent<Tag>(selectedEntity);
        if (tag) {
            if (ImGui::CollapsingHeader("Tag", ImGuiTreeNodeFlags_DefaultOpen)) {
                char buffer[256];
                strncpy(buffer, tag->name.c_str(), sizeof(buffer));
                if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
                    tag->name = buffer;
                }
            }
        }
        
        auto* transform = ecs->getComponent<Transform>(selectedEntity);
        if (transform) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Position", &transform->position.x, 0.1f);
                
                glm::vec3 euler = transform->getEulerAngles();
                if (ImGui::DragFloat3("Rotation", &euler.x, 1.0f)) {
                    transform->setEulerAngles(euler);
                }
                
                ImGui::DragFloat3("Scale", &transform->scale.x, 0.01f, 0.01f, 10.0f);
            }
        }
        
        auto* layer = ecs->getComponent<Layer>(selectedEntity);
        if (layer) {
            if (ImGui::CollapsingHeader("Layer")) {
                int layerNum = 0;
                for (int i = 0; i < 32; i++) {
                    if (layer->hasLayer(i)) {
                        layerNum = i;
                        break;
                    }
                }
                
                if (ImGui::SliderInt("Layer", &layerNum, 0, 31)) {
                    layer->setLayer(layerNum);
                }
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }
        
        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (!tag && ImGui::Selectable("Tag")) {
                ecs->addComponent(selectedEntity, Tag("Entity"));
            }
            if (!transform && ImGui::Selectable("Transform")) {
                ecs->addComponent(selectedEntity, Transform());
            }
            if (!layer && ImGui::Selectable("Layer")) {
                ecs->addComponent(selectedEntity, Layer());
            }
            ImGui::EndPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Delete Entity")) {
            ecs->destroyEntity(selectedEntity);
            selectedEntity = 0;
        }
        
        ImGui::End();
    }
    
    void renderCameraWindow(Camera* camera) {
        ImGui::Begin("Camera", &showCameraInfo);
        
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", 
                   camera->position.x, camera->position.y, camera->position.z);
        ImGui::Text("Target: (%.2f, %.2f, %.2f)",
                   camera->target.x, camera->target.y, camera->target.z);
        
        ImGui::Separator();
        
        ImGui::SliderFloat("FOV", &camera->fov, 30.0f, 120.0f);
        ImGui::DragFloat("Near Plane", &camera->nearPlane, 0.01f, 0.001f, 10.0f);
        ImGui::DragFloat("Far Plane", &camera->farPlane, 1.0f, 10.0f, 1000.0f);
        
        ImGui::End();
    }
};
