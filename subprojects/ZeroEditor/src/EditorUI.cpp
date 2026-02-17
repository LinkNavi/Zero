#include "Editor.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

// ============================================================
// Style & Fonts
// ============================================================

void Editor::setupStyle() {
  ImGuiStyle &s = ImGui::GetStyle();

  s.WindowRounding = 0.0f; // No rounding for docked look
  s.ChildRounding = 0.0f;
  s.FrameRounding = 3.0f;
  s.PopupRounding = 4.0f;
  s.ScrollbarRounding = 4.0f;
  s.GrabRounding = 2.0f;
  s.TabRounding = 3.0f;

  s.WindowPadding = ImVec2(8, 8);
  s.FramePadding = ImVec2(6, 4);
  s.ItemSpacing = ImVec2(6, 4);
  s.ItemInnerSpacing = ImVec2(4, 4);
  s.IndentSpacing = 16.0f;
  s.ScrollbarSize = 11.0f;
  s.GrabMinSize = 7.0f;
  s.WindowBorderSize = 1.0f;
  s.ChildBorderSize = 0.0f;
  s.FrameBorderSize = 0.0f;
  s.TabBorderSize = 0.0f;
  s.TabBarBorderSize = 1.0f;
  s.SeparatorTextBorderSize = 2.0f;
  s.WindowTitleAlign = ImVec2(0.0f, 0.5f);

  ImVec4 *c = s.Colors;

  // Background palette
  ImVec4 bgDark = ImVec4(0.106f, 0.110f, 0.129f, 1.0f);  // darkest
  ImVec4 bgMid = ImVec4(0.137f, 0.141f, 0.169f, 1.0f);   // panels
  ImVec4 bgLight = ImVec4(0.165f, 0.173f, 0.204f, 1.0f); // frames/inputs
  ImVec4 bgPopup = ImVec4(0.157f, 0.161f, 0.196f, 1.0f);

  // Border / separator
  ImVec4 border = ImVec4(0.216f, 0.224f, 0.271f, 1.0f);
  ImVec4 separator = ImVec4(0.216f, 0.224f, 0.271f, 0.6f);

  // Text
  ImVec4 text = ImVec4(0.890f, 0.898f, 0.925f, 1.0f);
  ImVec4 textDim = ImVec4(0.478f, 0.494f, 0.565f, 1.0f);

  // Accent (soft blue)
  ImVec4 accent = ImVec4(0.286f, 0.494f, 0.882f, 1.0f);
  ImVec4 accentHov = ImVec4(0.357f, 0.561f, 0.945f, 1.0f);
  ImVec4 accentAct = ImVec4(0.220f, 0.408f, 0.773f, 1.0f);
  ImVec4 accentDim = ImVec4(0.286f, 0.494f, 0.882f, 0.35f);

  // Interactive
  ImVec4 frame = bgLight;
  ImVec4 frameHov = ImVec4(0.200f, 0.208f, 0.247f, 1.0f);
  ImVec4 header = ImVec4(0.180f, 0.188f, 0.227f, 1.0f);
  ImVec4 headerHov = ImVec4(0.220f, 0.231f, 0.278f, 1.0f);

  // Tabs
  ImVec4 tab = bgDark;
  ImVec4 tabSel = bgMid;
  ImVec4 tabHov = ImVec4(0.200f, 0.210f, 0.255f, 1.0f);

  // Title
  ImVec4 titleBg = bgDark;
  ImVec4 titleAct = ImVec4(0.125f, 0.129f, 0.157f, 1.0f);

  c[ImGuiCol_Text] = text;
  c[ImGuiCol_TextDisabled] = textDim;
  c[ImGuiCol_WindowBg] = bgMid;
  c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_PopupBg] = bgPopup;
  c[ImGuiCol_Border] = border;
  c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_FrameBg] = frame;
  c[ImGuiCol_FrameBgHovered] = frameHov;
  c[ImGuiCol_FrameBgActive] = accentDim;
  c[ImGuiCol_TitleBg] = titleBg;
  c[ImGuiCol_TitleBgActive] = titleAct;
  c[ImGuiCol_TitleBgCollapsed] = titleBg;
  c[ImGuiCol_MenuBarBg] = bgDark;
  c[ImGuiCol_ScrollbarBg] = ImVec4(0.098f, 0.102f, 0.122f, 0.5f);
  c[ImGuiCol_ScrollbarGrab] = ImVec4(0.278f, 0.290f, 0.345f, 0.6f);
  c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.340f, 0.353f, 0.412f, 0.7f);
  c[ImGuiCol_ScrollbarGrabActive] = accent;
  c[ImGuiCol_CheckMark] = accent;
  c[ImGuiCol_SliderGrab] = accentDim;
  c[ImGuiCol_SliderGrabActive] = accent;
  c[ImGuiCol_Button] = frame;
  c[ImGuiCol_ButtonHovered] = headerHov;
  c[ImGuiCol_ButtonActive] = accentAct;
  c[ImGuiCol_Header] = header;
  c[ImGuiCol_HeaderHovered] = headerHov;
  c[ImGuiCol_HeaderActive] = accentDim;
  c[ImGuiCol_Separator] = separator;
  c[ImGuiCol_SeparatorHovered] = accent;
  c[ImGuiCol_SeparatorActive] = accent;
  c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_ResizeGripHovered] = accentDim;
  c[ImGuiCol_ResizeGripActive] = accent;
  c[ImGuiCol_Tab] = tab;
  c[ImGuiCol_TabHovered] = tabHov;
  c[ImGuiCol_TabSelected] = tabSel;
  c[ImGuiCol_TabSelectedOverline] = accent;
  c[ImGuiCol_TabDimmed] = tab;
  c[ImGuiCol_TabDimmedSelected] = tabSel;
  c[ImGuiCol_PlotLines] = accent;
  c[ImGuiCol_PlotLinesHovered] = accentHov;
  c[ImGuiCol_PlotHistogram] = accent;
  c[ImGuiCol_PlotHistogramHovered] = accentHov;
  c[ImGuiCol_TableHeaderBg] = header;
  c[ImGuiCol_TableBorderStrong] = border;
  c[ImGuiCol_TableBorderLight] = separator;
  c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
  c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.012f);
  c[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
  c[ImGuiCol_DragDropTarget] = accent;
  c[ImGuiCol_NavCursor] = accent;
  c[ImGuiCol_NavWindowingHighlight] = ImVec4(1, 1, 1, 0.6f);
  c[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0.2f);
  c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.45f);
}

void Editor::setupFonts() {
  ImGuiIO &io = ImGui::GetIO();

  float scale = renderer.getContentScale();

  ImFontConfig cfg;
  cfg.OversampleH = 2;
  cfg.OversampleV = 1;
  cfg.PixelSnapH = true;

  fontRegular = io.Fonts->AddFontDefault(&cfg);
  io.FontGlobalScale = scale;
}

// ============================================================
// Fixed Layout Constants
// ============================================================

static constexpr float MENUBAR_H = 22.0f;
static constexpr float TOOLBAR_H = 36.0f;
static constexpr float STATUSBAR_H = 22.0f;
static constexpr float LEFT_W = 220.0f;       // hierarchy
static constexpr float RIGHT_W = 280.0f;      // inspector
static constexpr float BOTTOM_H_FRAC = 0.25f; // console+assets height fraction

static constexpr ImGuiWindowFlags PANEL_FLAGS =
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus;

static constexpr ImGuiWindowFlags BAR_FLAGS =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

// ============================================================
// Main UI Layout
// ============================================================

void Editor::drawUI(VkCommandBuffer cmd) {
  ImGuiIO &io = ImGui::GetIO();
  ImGuiViewport *vp = ImGui::GetMainViewport();

  float W = vp->WorkSize.x;
  float H = vp->WorkSize.y;
  float ox = vp->WorkPos.x;
  float oy = vp->WorkPos.y;

  // Layout rects
  float topY = oy;
  float menuBot = topY + MENUBAR_H;
  float toolBot = menuBot + TOOLBAR_H;
  float statTop = oy + H - STATUSBAR_H;
  float bodyH = statTop - toolBot;
  float bottomH = bodyH * BOTTOM_H_FRAC;
  float midH = bodyH - bottomH;
  float centerW = W - LEFT_W - RIGHT_W;

  // ---- Menu Bar ----
  ImGui::SetNextWindowPos(ImVec2(ox, topY));
  ImGui::SetNextWindowSize(ImVec2(W, MENUBAR_H));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 2));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("##MenuBar", nullptr, BAR_FLAGS | ImGuiWindowFlags_MenuBar);
  drawMenuBar();
  ImGui::End();
  ImGui::PopStyleVar(2);

  // ---- Toolbar ----
  drawToolbar(ox, menuBot, W, TOOLBAR_H);

  // ---- Left: Hierarchy ----
  if (panels.hierarchy) {
    ImGui::SetNextWindowPos(ImVec2(ox, toolBot));
    ImGui::SetNextWindowSize(ImVec2(LEFT_W, midH));
    ImGui::Begin("Hierarchy", &panels.hierarchy, PANEL_FLAGS);
    drawHierarchyContent();
    ImGui::End();
  }

  // ---- Center: Viewport ----
  if (panels.viewport) {
    float vpX = panels.hierarchy ? ox + LEFT_W : ox;
    float vpW = centerW + (panels.hierarchy ? 0 : LEFT_W) +
                (panels.inspector ? 0 : RIGHT_W);
    ImGui::SetNextWindowPos(ImVec2(vpX, toolBot));
    ImGui::SetNextWindowSize(ImVec2(vpW, midH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", &panels.viewport, PANEL_FLAGS);
    drawViewportContent();
    ImGui::End();
    ImGui::PopStyleVar();
  }

  // ---- Right: Inspector ----
  if (panels.inspector) {
    ImGui::SetNextWindowPos(ImVec2(ox + W - RIGHT_W, toolBot));
    ImGui::SetNextWindowSize(ImVec2(RIGHT_W, midH));
    ImGui::Begin("Inspector", &panels.inspector, PANEL_FLAGS);
    drawInspectorContent();
    ImGui::End();
  }

  // ---- Bottom Left: Console ----
  if (panels.console) {
    float consoleW = (W - RIGHT_W) * 0.6f;
    ImGui::SetNextWindowPos(ImVec2(ox, toolBot + midH));
    ImGui::SetNextWindowSize(ImVec2(consoleW, bottomH));
    ImGui::Begin("Console", &panels.console, PANEL_FLAGS);
    drawConsoleContent();
    ImGui::End();
  }

  // ---- Bottom Right: Assets ----
  if (panels.assets) {
    float consoleW = (W - RIGHT_W) * 0.6f;
    float assetsX = ox + consoleW;
    float assetsW = W - RIGHT_W - consoleW;
    if (!panels.console) {
      assetsX = ox;
      assetsW = W - RIGHT_W;
    }
    ImGui::SetNextWindowPos(ImVec2(assetsX, toolBot + midH));
    ImGui::SetNextWindowSize(ImVec2(assetsW, bottomH));
    ImGui::Begin("Assets", &panels.assets, PANEL_FLAGS);
    drawAssetBrowserContent();
    ImGui::End();
  }

  // ---- Status Bar ----
  drawStatusBar(ox, statTop, W, STATUSBAR_H);

  // ---- Popups (float freely) ----
  if (panels.sceneSettings)
    drawSceneSettings();
  if (showFileDialog)
    drawFileDialog();

  // ---- Keyboard shortcuts ----
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
    newScene();
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
    saveScene();
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O))
    openScene();
  if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D))
    duplicateEntity();
  if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !io.WantTextInput)
    deleteSelectedEntity();
  if (ImGui::IsKeyPressed(ImGuiKey_F5))
    playScene();
  if (ImGui::IsKeyPressed(ImGuiKey_F6))
    pauseScene();
  if (ImGui::IsKeyPressed(ImGuiKey_F7))
    stopScene();
}

// ============================================================
// Menu Bar (content only, window managed by drawUI)
// ============================================================

void Editor::drawMenuBar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N"))
        newScene();
      if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
        openScene();
      ImGui::Separator();
      if (ImGui::MenuItem("Save", "Ctrl+S"))
        saveScene();
      if (ImGui::MenuItem("Save As..."))
        saveSceneAs();
      ImGui::Separator();
      if (ImGui::MenuItem("Exit"))
        glfwSetWindowShouldClose(renderer.getWindow(), GLFW_TRUE);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Duplicate", "Ctrl+D", false,
                          selectedEntity != INVALID_ENTITY))
        duplicateEntity();
      if (ImGui::MenuItem("Delete", "Del", false,
                          selectedEntity != INVALID_ENTITY))
        deleteSelectedEntity();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Entity")) {
      if (ImGui::MenuItem("Create Empty")) {
        selectedEntity = engine.createEntity("Empty");
        unsavedChanges = true;
      }
      if (ImGui::MenuItem("Create Cube")) {
        EntityID e = engine.createEntity("Cube");
        engine.setEntityModel(e, ResourcePath::models("cube.obj"));
        selectedEntity = e;
        unsavedChanges = true;
      }
      if (ImGui::MenuItem("Create Camera")) {
        EntityID e = engine.createEntity("Camera");
        engine.setEntityPosition(e, glm::vec3(0, 2, 5));
        engine.setEntityAsCamera(e, false);
        selectedEntity = e;
        unsavedChanges = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Hierarchy", nullptr, &panels.hierarchy);
      ImGui::MenuItem("Inspector", nullptr, &panels.inspector);
      ImGui::MenuItem("Viewport", nullptr, &panels.viewport);
      ImGui::MenuItem("Console", nullptr, &panels.console);
      ImGui::MenuItem("Assets", nullptr, &panels.assets);
      ImGui::MenuItem("Scene Settings", nullptr, &panels.sceneSettings);
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
}

// ============================================================
// Toolbar
// ============================================================

void Editor::drawToolbar(float x, float y, float w, float h) {
  ImGui::SetNextWindowPos(ImVec2(x, y));
  ImGui::SetNextWindowSize(ImVec2(w, h));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.118f, 0.122f, 0.149f, 1.0f));

  ImGui::Begin("##Toolbar", nullptr, BAR_FLAGS);

  float btnW = 64.0f;
  float btnH = 24.0f;
  float availW = ImGui::GetContentRegionAvail().x;
  float totalPlayW = btnW * 2 + 6;
  ImGui::SetCursorPosX((availW - totalPlayW) * 0.5f);

  // Play/Pause
  bool isPlaying = (playState == EditorPlayState::Play);
  if (isPlaying)
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.173f, 0.506f, 0.278f, 1.0f));
  else
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.200f, 0.208f, 0.247f, 1.0f));

  if (ImGui::Button(isPlaying ? "Pause" : "Play", ImVec2(btnW, btnH))) {
    if (isPlaying)
      pauseScene();
    else
      playScene();
  }
  ImGui::PopStyleColor();
  ImGui::SameLine(0, 6);

  // Stop
  bool canStop = (playState != EditorPlayState::Edit);
  if (!canStop)
    ImGui::BeginDisabled();
  ImGui::PushStyleColor(ImGuiCol_Button,
                        canStop ? ImVec4(0.592f, 0.184f, 0.184f, 1.0f)
                                : ImVec4(0.200f, 0.208f, 0.247f, 1.0f));
  if (ImGui::Button("Stop", ImVec2(btnW, btnH)))
    stopScene();
  ImGui::PopStyleColor();
  if (!canStop)
    ImGui::EndDisabled();

  // Gizmo mode — right side
  ImGui::SameLine(availW - 170);
  auto gizmoBtn = [&](const char *label, GizmoMode mode, float width) {
    bool sel = (gizmoMode == mode);
    if (sel)
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.286f, 0.494f, 0.882f, 0.4f));
    if (ImGui::Button(label, ImVec2(width, btnH)))
      gizmoMode = mode;
    if (sel)
      ImGui::PopStyleColor();
  };
  gizmoBtn("Move", GizmoMode::Translate, 48);
  ImGui::SameLine(0, 2);
  gizmoBtn("Rot", GizmoMode::Rotate, 40);
  ImGui::SameLine(0, 2);
  gizmoBtn("Scl", GizmoMode::Scale, 40);

  // State indicator — far right
  ImGui::SameLine(availW - 14 - ImGui::CalcTextSize(getPlayStateText()).x);
  ImGui::TextColored(getPlayStateColor(), "%s", getPlayStateText());

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

// ============================================================
// Hierarchy Content
// ============================================================

void Editor::drawHierarchyContent() {
  static char searchBuf[128] = "";
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##Search", "Search...", searchBuf, sizeof(searchBuf));
  ImGui::PopStyleVar();
  ImGui::Spacing();
  
  auto entities = engine.getEntities();
  std::string filter = searchBuf;
  
  // Helper to recursively draw entity and its children
  std::function<void(EntityID)> drawEntityTree = [&](EntityID parentId) {
    for (const auto &e : entities) {
      // Only draw entities whose parent matches
      if (e.parent != parentId)
        continue;
      
      // Apply search filter
      if (!filter.empty()) {
        std::string nl = e.name, fl = filter;
        std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
        std::transform(fl.begin(), fl.end(), fl.begin(), ::tolower);
        if (nl.find(fl) == std::string::npos)
          continue;
      }
      
      bool isSelected = (e.id == selectedEntity);
      const char *icon = e.isCamera ? "[C]" : (e.hasModel ? "[M]" : "   ");
      
      // Check if this entity has children
      bool hasChildren = false;
      for (const auto &child : entities) {
        if (child.parent == e.id) {
          hasChildren = true;
          break;
        }
      }
      
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
      if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
      if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;
      
      bool nodeOpen = false;
      
      // Show rename input or normal node
      if (renamingEntity == e.id) {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##rename", renameBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
          engine.setEntityName(e.id, renameBuffer);
          renamingEntity = INVALID_ENTITY;
          unsavedChanges = true;
        }
        if (ImGui::IsItemDeactivated()) {
          renamingEntity = INVALID_ENTITY;
        }
      } else {
        if (hasChildren) {
          nodeOpen = ImGui::TreeNodeEx((void *)(intptr_t)e.id, flags, "%s %s", icon, e.name.c_str());
        } else {
          ImGui::TreeNodeEx((void *)(intptr_t)e.id, flags, "%s %s", icon, e.name.c_str());
        }
        
        // Select on click
        if (ImGui::IsItemClicked()) {
          selectedEntity = e.id;
        }
        
        // Double-click to rename
        if (isSelected && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          renamingEntity = e.id;
          strncpy(renameBuffer, e.name.c_str(), sizeof(renameBuffer) - 1);
          renameBuffer[sizeof(renameBuffer) - 1] = '\0';
        }
        
        // Drag source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
          ImGui::SetDragDropPayload("ENTITY_DND", &e.id, sizeof(EntityID));
          ImGui::Text("Move %s", e.name.c_str());
          ImGui::EndDragDropSource();
        }
        
        // Drop target
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_DND")) {
            EntityID draggedId = *(EntityID *)payload->Data;
            if (draggedId != e.id) {
              // Check for circular dependency
              bool isCircular = false;
              EntityID checkParent = e.id;
              while (checkParent != 0) {
                if (checkParent == draggedId) {
                  isCircular = true;
                  break;
                }
                checkParent = engine.getEntityParent(checkParent);
              }
              
              if (!isCircular) {
                engine.setEntityParent(draggedId, e.id);
                unsavedChanges = true;
                log("Parented entity to " + e.name);
              } else {
                log("Cannot create circular parent relationship", LogEntry::Warning);
              }
            }
          }
          ImGui::EndDragDropTarget();
        }
      }
      
      // Right-click menu
      if (ImGui::BeginPopupContextItem()) {
        selectedEntity = e.id;
        
        if (e.parent != 0 && ImGui::MenuItem("Unparent")) {
          engine.removeEntityParent(e.id);
          unsavedChanges = true;
          log("Unparented entity");
        }
        
        if (ImGui::MenuItem("Rename")) {
          renamingEntity = e.id;
          strncpy(renameBuffer, e.name.c_str(), sizeof(renameBuffer) - 1);
          renameBuffer[sizeof(renameBuffer) - 1] = '\0';
        }
        
        if (ImGui::MenuItem("Duplicate"))
          duplicateEntity();
        if (ImGui::MenuItem("Delete"))
          deleteSelectedEntity();
        ImGui::Separator();
        if (ImGui::MenuItem("Focus Camera"))
          engine.setEditorCameraTarget(e.position);
        ImGui::EndPopup();
      }
      
      // Recursively draw children if node is open
      if (hasChildren && nodeOpen) {
        drawEntityTree(e.id);
        ImGui::TreePop();
      }
    }
  };
  
  // Draw root entities (parent == 0)
  drawEntityTree(0);
  
  // Background drop target for unparenting
  ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
  ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
  ImVec2 windowPos = ImGui::GetWindowPos();
  ImGui::SetCursorScreenPos(ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y));
  ImGui::InvisibleButton("##bg", ImVec2(contentMax.x - contentMin.x, contentMax.y - contentMin.y));
  
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_DND")) {
      EntityID draggedId = *(EntityID *)payload->Data;
      engine.removeEntityParent(draggedId);
      unsavedChanges = true;
      log("Unparented entity (dropped on background)");
    }
    ImGui::EndDragDropTarget();
  }
  
  // Create entity context menu
  if (ImGui::BeginPopupContextWindow("##HCtx", ImGuiPopupFlags_NoOpenOverItems)) {
    if (ImGui::MenuItem("Create Empty")) {
      selectedEntity = engine.createEntity("Empty");
      unsavedChanges = true;
    }
    if (ImGui::MenuItem("Create Cube")) {
      EntityID e = engine.createEntity("Cube");
      engine.setEntityModel(e, ResourcePath::models("cube.obj"));
      selectedEntity = e;
      unsavedChanges = true;
    }
    ImGui::EndPopup();
  }
}// ============================================================
// Inspector Content
// ============================================================

static bool drawVec3(const char *label, glm::vec3 &v) {
  bool changed = false;
  ImGui::PushID(label);

  ImGui::Columns(2, nullptr, false);
  ImGui::SetColumnWidth(0, 70.0f);
  ImGui::TextUnformatted(label);
  ImGui::NextColumn();

  float w = (ImGui::GetContentRegionAvail().x - 8) / 3.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));

  // X
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.50f, 0.15f, 0.15f, 0.50f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImVec4(0.60f, 0.20f, 0.20f, 0.60f));
  ImGui::SetNextItemWidth(w);
  if (ImGui::DragFloat("##X", &v.x, 0.05f, 0, 0, "%.2f"))
    changed = true;
  ImGui::PopStyleColor(2);
  ImGui::SameLine();

  // Y
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.50f, 0.15f, 0.50f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImVec4(0.20f, 0.60f, 0.20f, 0.60f));
  ImGui::SetNextItemWidth(w);
  if (ImGui::DragFloat("##Y", &v.y, 0.05f, 0, 0, "%.2f"))
    changed = true;
  ImGui::PopStyleColor(2);
  ImGui::SameLine();

  // Z
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.50f, 0.50f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
                        ImVec4(0.20f, 0.20f, 0.60f, 0.60f));
  ImGui::SetNextItemWidth(w);
  if (ImGui::DragFloat("##Z", &v.z, 0.05f, 0, 0, "%.2f"))
    changed = true;
  ImGui::PopStyleColor(2);

  ImGui::PopStyleVar();
  ImGui::Columns(1);
  ImGui::PopID();
  return changed;
}

void Editor::drawInspectorContent() {
  if (selectedEntity == INVALID_ENTITY) {
    ImGui::TextColored(ImVec4(0.45f, 0.46f, 0.52f, 1.0f), "No entity selected");
    return;
  }

  EntityInfo info = engine.getEntityInfo(selectedEntity);

  // Name
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.97f, 1.0f));
  ImGui::Text("%s", info.name.c_str());
  ImGui::PopStyleColor();
  ImGui::Separator();
  ImGui::Spacing();

  // Transform
  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    glm::vec3 pos = info.position, rot = info.rotation, scl = info.scale;
    if (drawVec3("Position", pos)) {
      engine.setEntityPosition(selectedEntity, pos);
      unsavedChanges = true;
    }
    if (drawVec3("Rotation", rot)) {
      engine.setEntityRotation(selectedEntity, rot);
      unsavedChanges = true;
    }
    if (drawVec3("Scale", scl)) {
      engine.setEntityScale(selectedEntity, scl);
      unsavedChanges = true;
    }
  }

  // Model
  if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (info.hasModel) {
      ImGui::TextWrapped("Path: %s", info.modelPath.c_str());
      if (ImGui::Button("Remove Model", ImVec2(-1, 0))) {
        engine.removeEntityModel(selectedEntity);
        unsavedChanges = true;
      }
    } else {
      ImGui::TextColored(ImVec4(0.45f, 0.46f, 0.52f, 1.0f), "No model");
      static char mp[512] = "";
      ImGui::SetNextItemWidth(-60);
      ImGui::InputTextWithHint("##MP", "path...", mp, sizeof(mp));
      ImGui::SameLine();
      if (ImGui::Button("Load", ImVec2(-1, 0)) && strlen(mp) > 0) {
        if (engine.setEntityModel(selectedEntity, mp)) {
          unsavedChanges = true;
          log("Loaded: " + std::string(mp));
        } else {
          log("Failed: " + std::string(mp), LogEntry::Error);
        }
      }
    }
  }

  // Camera
  if (info.isCamera) {
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
      bool active = info.isActiveCamera;
      if (ImGui::Checkbox("Active", &active)) {
        if (active)
          engine.setActiveCamera(selectedEntity);
      }
    }
  } else {
    ImGui::Spacing();
    if (ImGui::Button("+ Camera", ImVec2(-1, 0))) {
      engine.setEntityAsCamera(selectedEntity, false);
      unsavedChanges = true;
    }
  }

  // Drag drop target
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload("ASSET_MODEL")) {
      const char *path = (const char *)p->Data;
      if (engine.setEntityModel(selectedEntity, path)) {
        unsavedChanges = true;
        log("Dropped: " + std::string(path));
      }
    }
    ImGui::EndDragDropTarget();
  }
}

// ============================================================
// Viewport Content
// ============================================================

void Editor::drawViewportContent() {
  viewportFocused = ImGui::IsWindowFocused();
  viewportHovered = ImGui::IsWindowHovered();

  ImVec2 size = ImGui::GetContentRegionAvail();
  uint32_t newW = std::max((uint32_t)size.x, 64u);
  uint32_t newH = std::max((uint32_t)size.y, 64u);

  if (newW != viewportWidth || newH != viewportHeight) {
    viewportWidth = newW;
    viewportHeight = newH;
    engine.resize(viewportWidth, viewportHeight);
  }

  if (viewportDescSet) {
    ImGui::Image((ImTextureID)viewportDescSet, size);
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *p = ImGui::AcceptDragDropPayload("ASSET_MODEL")) {
        const char *path = (const char *)p->Data;
        std::string name = std::filesystem::path(path).stem().string();
        EntityID e = engine.createEntity(name);
        engine.setEntityModel(e, path);
        selectedEntity = e;
        unsavedChanges = true;
      }
      ImGui::EndDragDropTarget();
    }
  }

  // FPS overlay
  ImVec2 wp = ImGui::GetWindowPos();
  float fh = ImGui::GetFrameHeight();
  char fps_str[48];
  snprintf(fps_str, sizeof(fps_str), "%d FPS  %ux%u", fps, viewportWidth,
           viewportHeight);
  ImVec2 ts = ImGui::CalcTextSize(fps_str);
  ImVec2 p(wp.x + 6, wp.y + fh + 4);
  ImGui::GetWindowDrawList()->AddRectFilled(
      ImVec2(p.x - 3, p.y - 1), ImVec2(p.x + ts.x + 4, p.y + ts.y + 2),
      IM_COL32(18, 18, 24, 200), 3.0f);
  ImGui::GetWindowDrawList()->AddText(p, IM_COL32(170, 175, 195, 255), fps_str);
}

// ============================================================
// Console Content
// ============================================================

void Editor::drawConsoleContent() {
  // Filter bar
  auto filterBtn = [&](const char *label, int val, ImVec4 col) {
    if (consoleFilter == val)
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col.x, col.y, col.z, 0.3f));
    else
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    if (ImGui::SmallButton(label))
      consoleFilter = val;
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
  };

  filterBtn("All", -1, ImVec4(0.7f, 0.72f, 0.78f, 1.0f));
  filterBtn("Info", 0, ImVec4(0.6f, 0.65f, 0.75f, 1.0f));
  filterBtn("Warn", 1, ImVec4(0.95f, 0.82f, 0.30f, 1.0f));
  filterBtn("Error", 2, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
  if (ImGui::SmallButton("Clear"))
    consoleLogs.clear();
  ImGui::Separator();

  ImGui::BeginChild("##log", ImVec2(0, 0), ImGuiChildFlags_None,
                    ImGuiWindowFlags_HorizontalScrollbar);
  for (const auto &e : consoleLogs) {
    if (consoleFilter >= 0 && (int)e.level != consoleFilter)
      continue;
    ImVec4 col;
    const char *pfx;
    switch (e.level) {
    case LogEntry::Warning:
      col = ImVec4(0.95f, 0.82f, 0.30f, 1.0f);
      pfx = "[W] ";
      break;
    case LogEntry::Error:
      col = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);
      pfx = "[E] ";
      break;
    default:
      col = ImVec4(0.62f, 0.64f, 0.72f, 1.0f);
      pfx = "[I] ";
      break;
    }
    ImGui::TextColored(col, "%s%s", pfx, e.message.c_str());
  }
  if (consoleAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
}

// ============================================================
// Asset Browser Content
// ============================================================

void Editor::drawAssetBrowserContent() {
  static std::string curDir = ".";
  static std::vector<std::filesystem::directory_entry> entries;
  static bool needsRefresh = true;

  if (needsRefresh) {
    entries.clear();
    try {
      if (std::filesystem::exists(curDir)) {
        for (auto &e : std::filesystem::directory_iterator(curDir))
          entries.push_back(e);
        std::sort(entries.begin(), entries.end(), [](auto &a, auto &b) {
          if (a.is_directory() != b.is_directory())
            return a.is_directory();
          return a.path().filename().string() < b.path().filename().string();
        });
      }
    } catch (...) {
    }
    needsRefresh = false;
  }

  ImGui::TextColored(ImVec4(0.5f, 0.52f, 0.6f, 1.0f), "%s", curDir.c_str());
  ImGui::SameLine();
  if (ImGui::SmallButton("Up")) {
    auto parent = std::filesystem::path(curDir).parent_path();
    if (!parent.empty()) {
      curDir = parent.string();
      needsRefresh = true;
    }
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Refresh"))
    needsRefresh = true;
  ImGui::Separator();

  for (auto &entry : entries) {
    std::string name = entry.path().filename().string();
    if (name.empty() || name[0] == '.')
      continue;
    bool isDir = entry.is_directory();
    std::string ext = entry.path().extension().string();

    ImVec4 color(0.65f, 0.67f, 0.73f, 1.0f);
    if (isDir)
      color = ImVec4(0.45f, 0.65f, 0.95f, 1.0f);
    else if (ext == ".zscene")
      color = ImVec4(0.40f, 0.85f, 0.50f, 1.0f);
    else if (ext == ".obj" || ext == ".fbx" || ext == ".glb" || ext == ".gltf")
      color = ImVec4(0.90f, 0.70f, 0.30f, 1.0f);
    else if (ext == ".png" || ext == ".jpg")
      color = ImVec4(0.75f, 0.50f, 0.80f, 1.0f);

    ImGui::TextColored(color, "%s %s", isDir ? ">" : " ", name.c_str());

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      if (isDir) {
        curDir = entry.path().string();
        needsRefresh = true;
      } else if (ext == ".zscene") {
        if (engine.loadScene(entry.path().string())) {
          currentScenePath = entry.path().string();
          selectedEntity = INVALID_ENTITY;
          log("Loaded: " + currentScenePath);
        } else
          log("Load failed!", LogEntry::Error);
      }
    }

    if (!isDir &&
        (ext == ".obj" || ext == ".fbx" || ext == ".glb" || ext == ".gltf")) {
      if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        std::string fp = entry.path().string();
        ImGui::SetDragDropPayload("ASSET_MODEL", fp.c_str(), fp.size() + 1);
        ImGui::Text("%s", name.c_str());
        ImGui::EndDragDropSource();
      }
    }
  }
}

// ============================================================
// File Dialog
// ============================================================

void Editor::drawFileDialog() {
  ImGui::SetNextWindowSize(ImVec2(480, 360), ImGuiCond_FirstUseEver);
  const char *title = fileDialogSave ? "Save Scene###FD" : "Open Scene###FD";

  if (ImGui::Begin(title, &showFileDialog)) {
    static std::string dir = "scenes/";
    static std::vector<std::filesystem::directory_entry> de;
    static bool ref = true;
    static char fn[256] = "untitled.zscene";

    if (ref) {
      de.clear();
      try {
        std::filesystem::create_directories(dir);
        for (auto &e : std::filesystem::directory_iterator(dir))
          de.push_back(e);
        std::sort(de.begin(), de.end(), [](auto &a, auto &b) {
          if (a.is_directory() != b.is_directory())
            return a.is_directory();
          return a.path().filename().string() < b.path().filename().string();
        });
      } catch (...) {
      }
      ref = false;
    }

    ImGui::Text("%s", dir.c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Up##fd")) {
      auto p = std::filesystem::path(dir).parent_path();
      if (!p.empty()) {
        dir = p.string() + "/";
        ref = true;
      }
    }
    ImGui::Separator();

    ImGui::BeginChild("##flist", ImVec2(0, -50), ImGuiChildFlags_Borders);
    for (auto &e : de) {
      std::string n = e.path().filename().string();
      if (n.empty() || n[0] == '.')
        continue;
      bool d = e.is_directory();
      std::string x = e.path().extension().string();
      if (!d && x != ".zscene")
        continue;

      if (ImGui::Selectable((std::string(d ? "> " : "  ") + n).c_str(), false,
                            ImGuiSelectableFlags_AllowDoubleClick)) {
        if (d) {
          dir = e.path().string() + "/";
          ref = true;
        } else {
          strncpy(fn, n.c_str(), sizeof(fn) - 1);
          if (ImGui::IsMouseDoubleClicked(0) && !fileDialogSave) {
            std::string fp = dir + fn;
            if (engine.loadScene(fp)) {
              currentScenePath = fp;
              selectedEntity = INVALID_ENTITY;
              unsavedChanges = false;
              log("Loaded: " + fp);
            } else
              log("Failed: " + fp, LogEntry::Error);
            showFileDialog = false;
          }
        }
      }
    }
    ImGui::EndChild();

    ImGui::SetNextItemWidth(-120);
    ImGui::InputText("##fn", fn, sizeof(fn));
    ImGui::SameLine();
    if (ImGui::Button(fileDialogSave ? "Save" : "Open", ImVec2(52, 0))) {
      std::string fp = dir + fn;
      if (fp.find(".zscene") == std::string::npos)
        fp += ".zscene";
      if (fileDialogSave) {
        if (engine.saveScene(fp)) {
          currentScenePath = fp;
          unsavedChanges = false;
          log("Saved: " + fp);
        } else
          log("Save failed: " + fp, LogEntry::Error);
      } else {
        if (engine.loadScene(fp)) {
          currentScenePath = fp;
          selectedEntity = INVALID_ENTITY;
          unsavedChanges = false;
          log("Loaded: " + fp);
        } else
          log("Load failed: " + fp, LogEntry::Error);
      }
      showFileDialog = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(52, 0)))
      showFileDialog = false;
  }
  ImGui::End();
}

// ============================================================
// Scene Settings (popup)
// ============================================================

void Editor::drawSceneSettings() {
  ImGui::SetNextWindowSize(ImVec2(320, 340), ImGuiCond_FirstUseEver);
  ImGui::Begin("Scene Settings", &panels.sceneSettings);

  if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
    static bool shadows = true, skybox = false, pp = false;
    if (ImGui::Checkbox("Shadows", &shadows))
      engine.setShadowsEnabled(shadows);
    if (ImGui::Checkbox("Skybox", &skybox))
      engine.setSkyboxEnabled(skybox);
    if (ImGui::Checkbox("Post Processing", &pp))
      engine.setPostProcessEnabled(pp);
  }
  if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
    static float exposure = 1.0f, gamma = 2.2f;
    if (ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f))
      engine.setExposure(exposure);
    if (ImGui::SliderFloat("Gamma", &gamma, 1.0f, 3.0f))
      engine.setGamma(gamma);
  }
  if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    static glm::vec3 dir(-0.5f, -1.0f, -0.3f), col(1, 0.98f, 0.9f);
    static float amb = 0.15f;
    bool ch = false;
    if (drawVec3("Direction", dir))
      ch = true;
    if (ImGui::ColorEdit3("Color", &col.x))
      ch = true;
    if (ImGui::SliderFloat("Ambient", &amb, 0, 1))
      ch = true;
    if (ch)
      engine.setDirectionalLight(dir, col, amb);
  }

  ImGui::End();
}

// ============================================================
// Status Bar
// ============================================================

void Editor::drawStatusBar(float x, float y, float w, float h) {
  ImGui::SetNextWindowPos(ImVec2(x, y));
  ImGui::SetNextWindowSize(ImVec2(w, h));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 3));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg,
                        ImVec4(0.098f, 0.102f, 0.122f, 1.0f));

  ImGui::Begin("##Status", nullptr, BAR_FLAGS);

  if (!currentScenePath.empty())
    ImGui::Text("%s%s", currentScenePath.c_str(), unsavedChanges ? " *" : "");
  else
    ImGui::TextColored(ImVec4(0.45f, 0.47f, 0.55f, 1.0f), "Untitled%s",
                       unsavedChanges ? " *" : "");

  auto ents = engine.getEntities();
  char rt[96];
  snprintf(rt, sizeof(rt), "%zu entities | %d FPS", ents.size(), fps);
  float tw = ImGui::CalcTextSize(rt).x;
  ImGui::SameLine(ImGui::GetWindowWidth() - tw - 12);
  ImGui::TextColored(ImVec4(0.50f, 0.52f, 0.60f, 1.0f), "%s", rt);

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

// Stub for drawMenuWindow (now unused, drawUI manages menu directly)
void Editor::drawMenuWindow() {}
