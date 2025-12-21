using System;
using OpenTK.Mathematics;

namespace EngineCore.Utility
{
    /// <summary>
    /// Random utility class
    /// </summary>
    public static class RandomUtil
    {
        private static Random _random = new Random();

        public static float Range(float min, float max)
        {
            return min + (float)_random.NextDouble() * (max - min);
        }

        public static int Range(int min, int max)
        {
            return _random.Next(min, max);
        }

        public static Color4 RandomColor()
        {
            return new Color4(
                (float)_random.NextDouble(),
                (float)_random.NextDouble(),
                (float)_random.NextDouble(),
                1.0f
            );
        }
    }
}
