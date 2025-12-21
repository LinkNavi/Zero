 using System;
using System.Collections.Generic;
using System.IO;
using OpenTK.Audio.OpenAL;
using EngineCore.Audio;
using EngineCore.ECS;

/// <summary>
    /// Component for playing audio clips in 3D space
    /// </summary>
    public class AudioSource : MonoBehaviour
    {
        public AudioClip Clip { get; set; }
        public bool PlayOnAwake { get; set; } = false;
        public bool Loop { get; set; } = false;
        public float Volume { get; set; } = 1.0f;
        public float Pitch { get; set; } = 1.0f;
        public float MinDistance { get; set; } = 1.0f;
        public float MaxDistance { get; set; } = 500.0f;
        public bool Spatial { get; set; } = true; // 3D spatial audio

        private int _sourceId;
        private bool _isPlaying;

        protected override void Awake()
        {
            if (!AudioSystem.IsInitialized)
            {
                Console.WriteLine("WARNING: AudioSource created but AudioSystem not initialized");
                return;
            }

            // Create OpenAL source
            _sourceId = AL.GenSource();
            UpdateSourceProperties();
        }

        protected override void Start()
        {
            if (PlayOnAwake && Clip != null)
            {
                Play();
            }
        }

        protected override void Update()
        {
            if (!AudioSystem.IsInitialized || _sourceId == 0)
                return;

            // Update 3D position
            if (Spatial)
            {
                var pos = transform.position;
                AL.Source(_sourceId, ALSource3f.Position, pos.x, pos.y, pos.z);
            }

            // Update playing state
            AL.GetSource(_sourceId, ALGetSourcei.SourceState, out int state);
            _isPlaying = (state == (int)ALSourceState.Playing);
        }

        protected override void OnDestroy()
        {
            if (_sourceId != 0)
            {
                Stop();
                AL.DeleteSource(_sourceId);
                _sourceId = 0;
            }
        }

        private void UpdateSourceProperties()
        {
            if (_sourceId == 0) return;

            AL.Source(_sourceId, ALSourcef.Gain, Volume);
            AL.Source(_sourceId, ALSourcef.Pitch, Pitch);
            AL.Source(_sourceId, ALSourceb.Looping, Loop);
            AL.Source(_sourceId, ALSourcef.ReferenceDistance, MinDistance);
            AL.Source(_sourceId, ALSourcef.MaxDistance, MaxDistance);

            if (!Spatial)
            {
                // Non-spatial audio (2D)
                AL.Source(_sourceId, ALSourceb.SourceRelative, true);
                AL.Source(_sourceId, ALSource3f.Position, 0, 0, 0);
            }
            else
            {
                AL.Source(_sourceId, ALSourceb.SourceRelative, false);
                var pos = transform.position;
                AL.Source(_sourceId, ALSource3f.Position, pos.x, pos.y, pos.z);
            }
        }

        /// <summary>
        /// Play the audio clip
        /// </summary>
        public void Play()
        {
            if (!AudioSystem.IsInitialized || _sourceId == 0 || Clip == null)
                return;

            Stop(); // Stop current playback

            AL.Source(_sourceId, ALSourcei.Buffer, Clip.BufferId);
            UpdateSourceProperties();
            AL.SourcePlay(_sourceId);
            _isPlaying = true;
        }

        /// <summary>
        /// Stop playback
        /// </summary>
        public void Stop()
        {
            if (_sourceId == 0) return;

            AL.SourceStop(_sourceId);
            _isPlaying = false;
        }

        /// <summary>
        /// Pause playback
        /// </summary>
        public void Pause()
        {
            if (_sourceId == 0) return;

            AL.SourcePause(_sourceId);
            _isPlaying = false;
        }

        /// <summary>
        /// Resume playback
        /// </summary>
        public void UnPause()
        {
            if (_sourceId == 0) return;

            AL.SourcePlay(_sourceId);
            _isPlaying = true;
        }

        /// <summary>
        /// Check if audio is currently playing
        /// </summary>
        public bool IsPlaying => _isPlaying;

        /// <summary>
        /// Play a one-shot audio clip at a position
        /// </summary>
        public static void PlayClipAtPoint(AudioClip clip, Vector3 position, float volume = 1.0f)
        {
            if (!AudioSystem.IsInitialized || clip == null)
                return;

            // Create temporary source
            int source = AL.GenSource();
            AL.Source(source, ALSourcei.Buffer, clip.BufferId);
            AL.Source(source, ALSource3f.Position, position.x, position.y, position.z);
            AL.Source(source, ALSourcef.Gain, volume);
            AL.Source(source, ALSourceb.SourceRelative, false);
            AL.SourcePlay(source);

            // Note: In a real implementation, you'd track this source and delete it after playback
            // For now, this is a simplified version
        }
    }
