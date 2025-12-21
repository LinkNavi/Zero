# Game Engine - TODO Before Scripting & Runtime

## 🔴 Critical Core Systems

### Input System
- [ ] **Keyboard Input**
  - [ ] Implement `Input.GetKey(KeyCode)` - Check if key is held down
  - [ ] Implement `Input.GetKeyDown(KeyCode)` - Check if key was just pressed
  - [ ] Implement `Input.GetKeyUp(KeyCode)` - Check if key was just released
  - [ ] Create `KeyCode` enum for all keyboard keys
  - [ ] Integrate with OpenTK's keyboard state

- [ ] **Mouse Input**
  - [ ] Implement `Input.GetMouseButton(int button)` - Check mouse button state
  - [ ] Implement `Input.GetMouseButtonDown(int button)` - Mouse button press
  - [ ] Implement `Input.GetMouseButtonUp(int button)` - Mouse button release
  - [ ] Track `Input.MousePosition` (screen coordinates)
  - [ ] Track `Input.MouseDelta` (movement since last frame)
  - [ ] Implement mouse scroll wheel support
  - [ ] World position from mouse (raycasting from camera)

- [ ] **Gamepad/Controller Support** (Optional but recommended)
  - [ ] Detect connected controllers
  - [ ] Read analog stick input
  - [ ] Read button states
  - [ ] Support vibration/rumble

### Physics System
- [ ] **Collision Detection**
  - [ ] Implement `OnCollisionEnter()` callback
  - [ ] Implement `OnCollisionStay()` callback
  - [ ] Implement `OnCollisionExit()` callback
  - [ ] Implement `OnTriggerEnter()` for trigger colliders
  - [ ] Implement `OnTriggerStay()` for trigger colliders
  - [ ] Implement `OnTriggerExit()` for trigger colliders
  - [ ] Collision resolution (push objects apart)
  - [ ] Collision filtering by layers

- [ ] **Additional Collider Types**
  - [ ] SphereCollider
  - [ ] CapsuleCollider
  - [ ] MeshCollider (for complex geometry)
  - [ ] 2D colliders (BoxCollider2D, CircleCollider2D)

- [ ] **Physics Features**
  - [ ] Raycasting (`Physics.Raycast()`)
  - [ ] Sphere casting
  - [ ] Layer-based collision matrix
  - [ ] Physics materials (friction, bounciness)
  - [ ] Constraints (freeze position/rotation axes)

### Material & Shader System
- [ ] **Enhanced Material System**
  - [ ] Support for multiple textures (diffuse, normal, specular)
  - [ ] Material presets (Standard, Unlit, Transparent)
  - [ ] Material property blocks (for instancing)
  - [ ] Shader variants/keywords

- [ ] **Built-in Shaders**
  - [ ] Lit shader with lighting support
  - [ ] Unlit shader (current basic shader)
  - [ ] Transparent shader with alpha blending
  - [ ] Particle shader
  - [ ] UI shader (for screen-space rendering)
  - [ ] Skybox shader

- [ ] **Lighting System**
  - [ ] Pass light data to shaders (position, color, intensity)
  - [ ] Support for multiple lights
  - [ ] Shadow mapping (directional lights)
  - [ ] Point light shadows (cube maps)
  - [ ] Ambient lighting
  - [ ] Light culling/optimization

## 🟡 Important Rendering Features

### Camera Enhancements
- [ ] **Multiple Camera Support**
  - [ ] Render to texture (RenderTexture)
  - [ ] Camera stacking (overlay cameras)
  - [ ] Split-screen support
  - [ ] Picture-in-picture

- [ ] **Camera Effects**
  - [ ] Post-processing stack
  - [ ] Bloom effect
  - [ ] Color grading
  - [ ] Depth of field
  - [ ] Anti-aliasing (FXAA, MSAA)

### UI System Completion
- [ ] **UI Rendering**
  - [ ] Implement actual UI rendering pipeline
  - [ ] UI batching for performance
  - [ ] Screen-space shader for UI
  - [ ] Support for Canvas render modes

- [ ] **UI Components**
  - [ ] Implement UIText rendering (integrate font library)
  - [ ] UISlider component
  - [ ] UIToggle/Checkbox component
  - [ ] UIScrollView component
  - [ ] UIDropdown component
  - [ ] UIInputField (text input)

- [ ] **UI Layout System**
  - [ ] LayoutGroup components (Horizontal, Vertical, Grid)
  - [ ] ContentSizeFitter (auto-resize UI elements)
  - [ ] AspectRatioFitter
  - [ ] Auto-layout for responsive UI

- [ ] **UI Events**
  - [ ] Implement proper UI event system
  - [ ] Raycasting for UI (detect mouse over/clicks)
  - [ ] Event system integration (EventSystem component)

### Sprite/2D Rendering
- [ ] **2D Rendering Pipeline**
  - [ ] Integrate SpriteRenderer into main rendering system
  - [ ] Sprite batching for performance
  - [ ] Sorting layers and order
  - [ ] 2D camera (orthographic projection)

- [ ] **Sprite Features**
  - [ ] Sprite atlases (texture packing)
  - [ ] Sprite animation system
  - [ ] 9-slice sprites (for UI)
  - [ ] Tilemap support (optional)

### Particle System Rendering
- [ ] **Particle Rendering**
  - [ ] Integrate ParticleSystem into rendering pipeline
  - [ ] Billboard particles (always face camera)
  - [ ] Particle shaders with texture support
  - [ ] GPU particle system (compute shaders)

## 🟢 Nice-to-Have Systems

### Audio System
- [ ] **Audio Implementation**
  - [ ] Integrate OpenAL or similar audio library
  - [ ] Implement AudioSource.Play/Stop/Pause
  - [ ] 3D spatial audio (positional sound)
  - [ ] Audio mixer (volume control, effects)
  - [ ] Audio loading (WAV, MP3, OGG support)

### Animation System
- [ ] **Animation Framework**
  - [ ] Animation clips (keyframe animation)
  - [ ] Animator component
  - [ ] Animation state machine
  - [ ] Blend trees
  - [ ] Root motion
  - [ ] Skeletal animation (for character models)

### Scene Management
- [ ] **Scene System**
  - [ ] Save/Load scenes to file (JSON/XML)
  - [ ] Scene serialization
  - [ ] Additive scene loading
  - [ ] Scene transitions
  - [ ] DontDestroyOnLoad functionality

### Asset Management
- [ ] **Asset Loading**
  - [ ] Resource folder system
  - [ ] Asset bundles
  - [ ] Async asset loading
  - [ ] Asset caching
  - [ ] Hot-reloading assets in editor

### Optimization & Performance
- [ ] **Performance Features**
  - [ ] Object pooling system
  - [ ] Frustum culling (don't render off-screen objects)
  - [ ] Occlusion culling
  - [ ] LOD (Level of Detail) system
  - [ ] Mesh batching/instancing
  - [ ] Profiler/Performance monitoring

## 🔵 Quality of Life

### Debug Tools
- [ ] **Debug Utilities**
  - [ ] `Debug.DrawLine()` - Draw debug lines in world space
  - [ ] `Debug.DrawRay()` - Draw debug rays
  - [ ] Gizmos system (visual debug helpers)
  - [ ] Debug console (in-game command line)
  - [ ] FPS counter overlay
  - [ ] Performance graphs

### Math Utilities
- [ ] **Enhanced Math Library**
  - [ ] `Mathf` utility class (Lerp, Clamp, etc.)
  - [ ] Quaternion.Euler() - Create rotation from angles
  - [ ] Quaternion.Slerp() - Smooth rotation interpolation
  - [ ] Vector3.Lerp() - Linear interpolation
  - [ ] Proper Transform.Rotate() and Transform.LookAt()
  - [ ] Color utilities (HSV conversion, etc.)

### Coroutines
- [ ] **Coroutine System**
  - [ ] IEnumerator-based coroutines
  - [ ] `yield return null` (wait one frame)
  - [ ] `yield return new WaitForSeconds(time)`
  - [ ] `yield return new WaitUntil(condition)`
  - [ ] StartCoroutine/StopCoroutine in MonoBehaviour

### Prefab System
- [ ] **Prefabs**
  - [ ] Save GameObject as prefab
  - [ ] Instantiate from prefab
  - [ ] Prefab variants
  - [ ] Apply/revert prefab changes

---

## 📋 Recommended Implementation Order

### Phase 1: Core Input & Physics (Week 1)
1. Input system (keyboard + mouse)
2. Basic collision detection & callbacks
3. Raycasting

### Phase 2: Rendering & Visuals (Week 2)
4. Lighting system & lit shaders
5. Sprite rendering integration
6. UI rendering pipeline
7. Particle rendering

### Phase 3: Essential Components (Week 3)
8. Audio system integration
9. Enhanced math utilities (Lerp, Quaternion helpers)
10. Debug drawing tools
11. Scene serialization

### Phase 4: Polish & Optimization (Week 4)
12. Animation system basics
13. Frustum culling
14. Object pooling
15. Coroutines

---

## ✅ Current Status

### Already Implemented ✓
- ✅ ECS/GameObject system
- ✅ Transform hierarchy
- ✅ Component system
- ✅ MonoBehaviour lifecycle
- ✅ Basic rendering (meshes, materials)
- ✅ Camera system
- ✅ Time class
- ✅ Texture loading
- ✅ Basic components (MeshRenderer, Light, etc.)
- ✅ UI component structure (Canvas, RectTransform, etc.)
- ✅ Basic physics (Rigidbody, BoxCollider)
- ✅ Particle system structure
- ✅ Trail renderer structure

### Next Immediate Steps 🎯
1. **Input System** - This is critical for any interactive game
2. **Collision Callbacks** - Make physics actually usable
3. **Lighting System** - Make your scenes look good
4. **UI Rendering** - Get UI actually displaying on screen

---

## 📝 Notes

- **Minimum Viable Engine**: Focus on Input, Collision Callbacks, and basic Lighting first
- **Can Start Scripting After**: Input + Collision + Lighting implementation
- **Runtime/Editor**: Needs Scene Serialization + Asset Management at minimum
- **Performance**: Can optimize later, focus on functionality first

**Estimated Time to "Scripting Ready"**: 1-2 weeks of focused work on Phase 1-2

**Estimated Time to "Runtime Ready"**: 3-4 weeks including Phase 3

Good luck! 🚀
