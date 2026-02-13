#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

struct AudioConfig {
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    int maxChannels = 32;
};

struct Sound {
    std::vector<float> samples;
    int sampleRate = 44100;
    int channels = 2;
    float duration = 0.0f;
};

class AudioSystem {
    std::unordered_map<std::string, Sound> sounds;
    
public:
    AudioConfig config;
    
    bool init();
    bool loadSound(const std::string& name, const std::string& filepath);
    void playSound(const std::string& name, float volume = 1.0f, bool loop = false);
    void playMusic(const std::string& name, float volume = 1.0f);
    void stopMusic();
    void setListenerPosition(glm::vec3 pos);
    void play3DSound(const std::string& name, glm::vec3 position, float volume = 1.0f);
    void update();
    void cleanup();
};
