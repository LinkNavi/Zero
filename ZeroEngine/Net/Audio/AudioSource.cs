
using System;
using EngineCore.ECS;

namespace EngineCore.Audio
{
    /// <summary>
    /// Component for playing audio clips
    /// Note: Requires an audio library like OpenAL or similar
    /// </summary>
    public class AudioSource : Component
    {
        public AudioClip clip;
        public bool playOnAwake = true;
        public bool loop = false;
        public float volume = 1.0f;
        public float pitch = 1.0f;
        
        private bool _isPlaying = false;

        public void Play()
        {
            _isPlaying = true;
            // TODO: Implement with audio library
            Console.WriteLine($"Playing audio: {clip?.name ?? "null"}");
        }

        public void Stop()
        {
            _isPlaying = false;
        }

        public void Pause()
        {
            // TODO: Implement
        }

        public bool IsPlaying => _isPlaying;
    }
}
