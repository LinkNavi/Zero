namespace EngineCore
{
    /// <summary>
    /// Time utility class for tracking frame timing
    /// Similar to Unity's Time class
    /// </summary>
    public static class Time
    {
        /// <summary>
        /// Time in seconds since the last frame (use this for frame-rate independent movement)
        /// </summary>
        public static float DeltaTime { get; internal set; }
        
        /// <summary>
        /// Total time in seconds since the game started
        /// </summary>
        public static float TotalTime { get; internal set; }
        
        /// <summary>
        /// Time scale for slow-motion or fast-forward effects (default: 1.0)
        /// 0.5 = half speed, 2.0 = double speed, 0 = paused
        /// </summary>
        public static float TimeScale { get; set; } = 1.0f;
        
        /// <summary>
        /// Scaled delta time (DeltaTime * TimeScale)
        /// </summary>
        public static float ScaledDeltaTime => DeltaTime * TimeScale;
        
        /// <summary>
        /// Unscaled time since the last frame (ignores TimeScale)
        /// </summary>
        public static float UnscaledDeltaTime { get; internal set; }
        
        /// <summary>
        /// Frame count since the game started
        /// </summary>
        public static int FrameCount { get; internal set; }
        
        /// <summary>
        /// Current frames per second
        /// </summary>
        public static float FPS { get; internal set; }
        
        /// <summary>
        /// Time at the beginning of this frame
        /// </summary>
        public static float TimeAsDouble { get; internal set; }
        
        // Internal state for FPS calculation
        private static float _fpsTimer = 0f;
        private static int _fpsFrameCount = 0;
        
        /// <summary>
        /// Update time values (called by the engine each frame)
        /// </summary>
        internal static void Update(float deltaTime)
        {
            UnscaledDeltaTime = deltaTime;
            DeltaTime = deltaTime * TimeScale;
            TotalTime += DeltaTime;
            TimeAsDouble += deltaTime;
            FrameCount++;
            
            // Calculate FPS (update every 0.5 seconds)
            _fpsTimer += deltaTime;
            _fpsFrameCount++;
            
            if (_fpsTimer >= 0.5f)
            {
                FPS = _fpsFrameCount / _fpsTimer;
                _fpsTimer = 0f;
                _fpsFrameCount = 0;
            }
        }
        
        /// <summary>
        /// Reset time to initial state
        /// </summary>
        internal static void Reset()
        {
            DeltaTime = 0f;
            TotalTime = 0f;
            TimeScale = 1.0f;
            UnscaledDeltaTime = 0f;
            FrameCount = 0;
            FPS = 0f;
            TimeAsDouble = 0f;
            _fpsTimer = 0f;
            _fpsFrameCount = 0;
        }
    }
}
