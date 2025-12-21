using System;
using System.Collections.Generic;
using System.IO;
using OpenTK.Audio.OpenAL;
using EngineCore.ECS;

namespace EngineCore.Audio
{
    /// <summary>
    /// Audio clip data - contains actual audio buffer
    /// </summary>
    public class AudioClip : IDisposable
    {
        internal int BufferId { get; set; }
        public string Name { get; set; } = string.Empty;
        public float Length { get; set; } // Duration in seconds
        public int SampleRate { get; set; }
        public int Channels { get; set; }

        public void Dispose()
        {
            if (BufferId != 0)
            {
                AL.DeleteBuffer(BufferId);
                BufferId = 0;
            }
        }
    }

    /// <summary>
    /// Audio system manager - handles OpenAL context
    /// </summary>
    public static class AudioSystem
    {
        private static ALDevice _device;
        private static ALContext _context;
        private static bool _initialized = false;

        /// <summary>
        /// Initialize the audio system
        /// </summary>
        public static void Initialize()
        {
            if (_initialized)
                return;

            try
            {
                // Open default audio device
                _device = ALC.OpenDevice(string.Empty);
                
                if (_device == ALDevice.Null)
                {
                    Console.WriteLine("ERROR: Failed to open audio device");
                    return;
                }

                // Create audio context
                _context = ALC.CreateContext(_device, (int[]?)null);
                if (_context == ALContext.Null)
                {
                    Console.WriteLine("ERROR: Failed to create audio context");
                    return;
                }

                // Make context current
                ALC.MakeContextCurrent(_context);

                _initialized = true;
                Console.WriteLine("Audio system initialized successfully");
                Console.WriteLine($"Audio Device: {ALC.GetString(_device, AlcGetString.DeviceSpecifier)}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR initializing audio system: {ex.Message}");
            }
        }

        /// <summary>
        /// Shutdown the audio system
        /// </summary>
        public static void Shutdown()
        {
            if (!_initialized)
                return;

            ALC.MakeContextCurrent(ALContext.Null);
            if (_context != ALContext.Null)
            {
                ALC.DestroyContext(_context);
                _context = ALContext.Null;
            }
            if (_device != ALDevice.Null)
            {
                ALC.CloseDevice(_device);
                _device = ALDevice.Null;
            }

            _initialized = false;
            Console.WriteLine("Audio system shut down");
        }

        /// <summary>
        /// Set the listener position (usually the camera/player position)
        /// </summary>
        public static void SetListenerPosition(Vector3 position)
        {
            if (!_initialized) return;
            AL.Listener(ALListener3f.Position, position.x, position.y, position.z);
        }

        /// <summary>
        /// Set the listener velocity (for doppler effect)
        /// </summary>
        public static void SetListenerVelocity(Vector3 velocity)
        {
            if (!_initialized) return;
            AL.Listener(ALListener3f.Velocity, velocity.x, velocity.y, velocity.z);
        }

        /// <summary>
        /// Set the listener orientation
        /// </summary>
        public static void SetListenerOrientation(Vector3 forward, Vector3 up)
        {
            if (!_initialized) return;
            float[] orientation = new float[] {
                forward.x, forward.y, forward.z,
                up.x, up.y, up.z
            };
            AL.Listener(ALListenerfv.Orientation, orientation);
        }

        /// <summary>
        /// Load an audio clip from a WAV file
        /// </summary>
        public static AudioClip? LoadWav(string filePath)
        {
            if (!_initialized)
            {
                Console.WriteLine("ERROR: Audio system not initialized");
                return null;
            }

            if (!File.Exists(filePath))
            {
                Console.WriteLine($"ERROR: Audio file not found: {filePath}");
                return null;
            }

            try
            {
                using var stream = File.OpenRead(filePath);
                using var reader = new BinaryReader(stream);

                // Read WAV header
                string signature = new string(reader.ReadChars(4));
                if (signature != "RIFF")
                    throw new NotSupportedException("Not a valid WAV file");

                reader.ReadInt32(); // File size
                string format = new string(reader.ReadChars(4));
                if (format != "WAVE")
                    throw new NotSupportedException("Not a valid WAV file");

                // Find fmt chunk
                while (stream.Position < stream.Length)
                {
                    string chunkId = new string(reader.ReadChars(4));
                    int chunkSize = reader.ReadInt32();

                    if (chunkId == "fmt ")
                    {
                        reader.ReadInt16(); // Audio format
                        int channels = reader.ReadInt16();
                        int sampleRate = reader.ReadInt32();
                        reader.ReadInt32(); // Byte rate
                        reader.ReadInt16(); // Block align
                        int bitsPerSample = reader.ReadInt16();

                        // Skip any extra format bytes
                        reader.ReadBytes(chunkSize - 16);

                        // Find data chunk
                        while (stream.Position < stream.Length)
                        {
                            string dataChunkId = new string(reader.ReadChars(4));
                            int dataSize = reader.ReadInt32();

                            if (dataChunkId == "data")
                            {
                                byte[] audioData = reader.ReadBytes(dataSize);

                                // Determine format
                                ALFormat alFormat;
                                if (channels == 1 && bitsPerSample == 8)
                                    alFormat = ALFormat.Mono8;
                                else if (channels == 1 && bitsPerSample == 16)
                                    alFormat = ALFormat.Mono16;
                                else if (channels == 2 && bitsPerSample == 8)
                                    alFormat = ALFormat.Stereo8;
                                else if (channels == 2 && bitsPerSample == 16)
                                    alFormat = ALFormat.Stereo16;
                                else
                                    throw new NotSupportedException($"Unsupported WAV format: {channels} channels, {bitsPerSample} bits");

                                // Create OpenAL buffer
                                int buffer = AL.GenBuffer();
                                AL.BufferData(buffer, alFormat, audioData, sampleRate);

                                // Check for errors
                                ALError error = AL.GetError();
                                if (error != ALError.NoError)
                                {
                                    Console.WriteLine($"OpenAL Error: {error}");
                                    AL.DeleteBuffer(buffer);
                                    return null;
                                }

                                float length = (float)audioData.Length / (sampleRate * channels * (bitsPerSample / 8));

                                var clip = new AudioClip
                                {
                                    BufferId = buffer,
                                    Name = Path.GetFileName(filePath),
                                    Length = length,
                                    SampleRate = sampleRate,
                                    Channels = channels
                                };

                                Console.WriteLine($"Loaded audio: {clip.Name} ({length:F2}s, {sampleRate}Hz, {channels}ch)");
                                return clip;
                            }
                            else
                            {
                                // Skip unknown chunk
                                reader.ReadBytes(dataSize);
                            }
                        }
                    }
                    else
                    {
                        // Skip unknown chunk
                        reader.ReadBytes(chunkSize);
                    }
                }

                throw new InvalidDataException("No data chunk found in WAV file");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR loading audio file {filePath}: {ex.Message}");
                return null;
            }
        }

        /// <summary>
        /// Check if audio system is initialized
        /// </summary>
        public static bool IsInitialized => _initialized;
    }
}
