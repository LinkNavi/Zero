#pragma once
#include "Engine.h"
#include "ModelLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

struct AnimatorComponent : Component {
    Model* model = nullptr;
    int animationIndex = -1;
    float currentTime = 0.0f;
    float speed = 1.0f;
    bool playing = false;
    bool loop = true;
    
    // Blending
    int blendFromIndex = -1;
    float blendFromTime = 0.0f;
    float blendFactor = 0.0f;
    float blendDuration = 0.0f;
    bool blending = false;
    
    // Root motion
    bool useRootMotion = false;
    glm::vec3 rootMotionDelta = glm::vec3(0);
    glm::vec3 lastRootPosition = glm::vec3(0);
    
    // Output
    std::vector<glm::mat4> boneMatrices;
    std::vector<glm::mat4> finalTransforms;
    
    void init(Model* mdl) {
        model = mdl;
        if (!model) return;
        size_t boneCount = model->bones.size();
        if (boneCount == 0) boneCount = 128;
        boneMatrices.resize(boneCount, glm::mat4(1.0f));
        finalTransforms.resize(boneCount, glm::mat4(1.0f));
    }
    
    void play(int index, bool looping = true) {
        if (!model || index < 0 || index >= (int)model->animations.size()) return;
        animationIndex = index;
        currentTime = 0.0f;
        playing = true;
        loop = looping;
        blending = false;
    }
    
    void play(const std::string& name, bool looping = true) {
        if (!model) return;
        for (size_t i = 0; i < model->animations.size(); i++) {
            if (model->animations[i].name == name) {
                play((int)i, looping);
                return;
            }
        }
    }
    
    void crossfade(int toIndex, float duration = 0.3f) {
        if (!model || toIndex < 0 || toIndex >= (int)model->animations.size()) return;
        if (toIndex == animationIndex) return;
        
        blendFromIndex = animationIndex;
        blendFromTime = currentTime;
        animationIndex = toIndex;
        currentTime = 0.0f;
        blendFactor = 0.0f;
        blendDuration = duration;
        blending = true;
        playing = true;
    }
    
    void crossfade(const std::string& name, float duration = 0.3f) {
        if (!model) return;
        for (size_t i = 0; i < model->animations.size(); i++) {
            if (model->animations[i].name == name) {
                crossfade((int)i, duration);
                return;
            }
        }
    }
    
    void stop() { playing = false; currentTime = 0.0f; blending = false; }
    void pause() { playing = false; }
    void resume() { playing = true; }
    
    float getDuration() const {
        if (!model || animationIndex < 0 || animationIndex >= (int)model->animations.size()) return 0.0f;
        const Animation& anim = model->animations[animationIndex];
        return anim.duration / anim.ticksPerSecond;
    }
    
    float getProgress() const {
        float dur = getDuration();
        return dur > 0.0f ? currentTime / dur : 0.0f;
    }
    
    int getAnimationIndex(const std::string& name) const {
        if (!model) return -1;
        for (size_t i = 0; i < model->animations.size(); i++) {
            if (model->animations[i].name == name) return (int)i;
        }
        return -1;
    }
};

// ============== ANIMATION STATE MACHINE ==============

enum class AnimationTransitionCondition {
    Immediate,
    OnComplete,
    OnThreshold
};

struct AnimationTransition {
    std::string toState;
    AnimationTransitionCondition condition = AnimationTransitionCondition::Immediate;
    float blendDuration = 0.2f;
    std::function<bool()> predicate = nullptr;
};

struct AnimationState {
    std::string name;
    int animationIndex = -1;
    float speed = 1.0f;
    bool loop = true;
    std::vector<AnimationTransition> transitions;
    
    void addTransition(const std::string& to, float blendTime = 0.2f, 
                       std::function<bool()> condition = nullptr,
                       AnimationTransitionCondition type = AnimationTransitionCondition::Immediate) {
        AnimationTransition t;
        t.toState = to;
        t.blendDuration = blendTime;
        t.predicate = condition;
        t.condition = type;
        transitions.push_back(t);
    }
};

class AnimationStateMachine {
public:
    std::unordered_map<std::string, AnimationState> states;
    std::string currentState;
    std::string defaultState;
    AnimatorComponent* animator = nullptr;
    
    // Parameters for conditions
    std::unordered_map<std::string, float> floatParams;
    std::unordered_map<std::string, bool> boolParams;
    std::unordered_map<std::string, int> intParams;
    
    void init(AnimatorComponent* anim) {
        animator = anim;
    }
    
    void addState(const std::string& name, int animIndex, bool loop = true, float speed = 1.0f) {
        AnimationState state;
        state.name = name;
        state.animationIndex = animIndex;
        state.loop = loop;
        state.speed = speed;
        states[name] = state;
        
        if (defaultState.empty()) {
            defaultState = name;
        }
    }
    
    void addState(const std::string& name, const std::string& animName, bool loop = true, float speed = 1.0f) {
        if (!animator || !animator->model) return;
        int idx = animator->getAnimationIndex(animName);
        if (idx >= 0) addState(name, idx, loop, speed);
    }
    
    void addTransition(const std::string& from, const std::string& to, 
                       float blendTime = 0.2f, std::function<bool()> condition = nullptr) {
        if (states.find(from) != states.end()) {
            states[from].addTransition(to, blendTime, condition);
        }
    }
    
    void setFloat(const std::string& name, float value) { floatParams[name] = value; }
    void setBool(const std::string& name, bool value) { boolParams[name] = value; }
    void setInt(const std::string& name, int value) { intParams[name] = value; }
    
    float getFloat(const std::string& name) const {
        auto it = floatParams.find(name);
        return it != floatParams.end() ? it->second : 0.0f;
    }
    bool getBool(const std::string& name) const {
        auto it = boolParams.find(name);
        return it != boolParams.end() ? it->second : false;
    }
    int getInt(const std::string& name) const {
        auto it = intParams.find(name);
        return it != intParams.end() ? it->second : 0;
    }
    
    void start() {
        if (!defaultState.empty()) {
            transitionTo(defaultState, 0.0f);
        }
    }
    
    void transitionTo(const std::string& stateName, float blendTime = 0.2f) {
        if (states.find(stateName) == states.end()) return;
        if (stateName == currentState) return;
        
        auto& state = states[stateName];
        currentState = stateName;
        
        if (blendTime > 0.0f && animator->animationIndex >= 0) {
            animator->crossfade(state.animationIndex, blendTime);
        } else {
            animator->play(state.animationIndex, state.loop);
        }
        animator->speed = state.speed;
    }
    
    void update(float dt) {
        if (!animator || currentState.empty()) return;
        
        auto it = states.find(currentState);
        if (it == states.end()) return;
        
        auto& state = it->second;
        
        // Check transitions
        for (auto& transition : state.transitions) {
            bool shouldTransition = false;
            
            switch (transition.condition) {
                case AnimationTransitionCondition::Immediate:
                    if (transition.predicate && transition.predicate()) {
                        shouldTransition = true;
                    }
                    break;
                    
                case AnimationTransitionCondition::OnComplete:
                    if (!state.loop && animator->getProgress() >= 0.95f) {
                        if (!transition.predicate || transition.predicate()) {
                            shouldTransition = true;
                        }
                    }
                    break;
                    
                case AnimationTransitionCondition::OnThreshold:
                    if (transition.predicate && transition.predicate()) {
                        shouldTransition = true;
                    }
                    break;
            }
            
            if (shouldTransition) {
                transitionTo(transition.toState, transition.blendDuration);
                break;
            }
        }
    }
};

class AnimationSystem : public System {
public:
    ECS* ecs = nullptr;
    
    void init(ECS* e) { ecs = e; }
    
    void update(float dt) override {
        for (EntityID entity : entities) {
            auto* anim = ecs->getComponent<AnimatorComponent>(entity);
            if (!anim || !anim->playing || !anim->model) continue;
            if (anim->animationIndex < 0 || anim->model->animations.empty()) continue;
            
            updateAnimator(*anim, dt);
        }
    }
    
private:
    void updateAnimator(AnimatorComponent& anim, float dt) {
        const Animation& animation = anim.model->animations[anim.animationIndex];
        float duration = animation.duration / animation.ticksPerSecond;
        
        anim.currentTime += dt * anim.speed;
        
        // Update blend
        if (anim.blending) {
            anim.blendFactor += dt / anim.blendDuration;
            if (anim.blendFactor >= 1.0f) {
                anim.blendFactor = 1.0f;
                anim.blending = false;
            }
            
            // Also advance the "from" animation time
            if (anim.blendFromIndex >= 0) {
                const Animation& fromAnim = anim.model->animations[anim.blendFromIndex];
                float fromDuration = fromAnim.duration / fromAnim.ticksPerSecond;
                anim.blendFromTime += dt * anim.speed;
                if (anim.blendFromTime >= fromDuration) {
                    anim.blendFromTime = fmod(anim.blendFromTime, fromDuration);
                }
            }
        }
        
        if (anim.currentTime >= duration) {
            if (anim.loop) {
                anim.currentTime = fmod(anim.currentTime, duration);
            } else {
                anim.currentTime = duration;
                anim.playing = false;
            }
        }
    }
};
