 using System;
using System.Collections.Generic;
using System.IO;
using OpenTK.Audio.OpenAL;
using EngineCore.Audio;
using EngineCore.ECS;

/// <summary>
    /// Audio mixer for managing multiple audio sources
    /// </summary>
    public static class AudioMixer
    {
        private static float _masterVolume = 1.0f;
        private static float _musicVolume = 1.0f;
        private static float _sfxVolume = 1.0f;

        public static float MasterVolume
        {
            get => _masterVolume;
            set
            {
                _masterVolume = MathHelper.Clamp(value, 0f, 1f);
                AL.Listener(ALListenerf.Gain, _masterVolume);
            }
        }

        public static float MusicVolume
        {
            get => _musicVolume;
            set => _musicVolume = MathHelper.Clamp(value, 0f, 1f);
        }

        public static float SFXVolume
        {
            get => _sfxVolume;
            set => _sfxVolume = MathHelper.Clamp(value, 0f, 1f);
        }
    }

