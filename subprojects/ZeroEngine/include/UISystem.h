#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>

struct UIConfig {
    float uiScale = 1.0f;
    glm::vec4 primaryColor = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
    glm::vec4 secondaryColor = glm::vec4(0.15f, 0.15f, 0.15f, 0.9f);
    glm::vec4 textColor = glm::vec4(1.0f);
};

enum class Anchor { TopLeft, TopRight, BottomLeft, BottomRight, Center };

struct UIElement {
    glm::vec2 position;
    glm::vec2 size;
    Anchor anchor = Anchor::TopLeft;
    bool visible = true;
    std::string text;
    glm::vec4 color = glm::vec4(1);
};

class UISystem {
    std::vector<UIElement> elements;
    
public:
    UIConfig config;
    
    void addText(const std::string& text, glm::vec2 pos, Anchor anchor = Anchor::TopLeft);
    void addButton(const std::string& text, glm::vec2 pos, glm::vec2 size, std::function<void()> callback);
    void addPanel(glm::vec2 pos, glm::vec2 size, glm::vec4 color);
    void clear();
    void render(VkCommandBuffer cmd);
    bool handleInput(glm::vec2 mousePos, bool clicked);
};
