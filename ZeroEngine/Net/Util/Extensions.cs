
using System;
using EngineCore.ECS;

namespace EngineCore.Utility
{
    /// <summary>
    /// Useful extension methods
    /// </summary>
    public static class Extensions
    {
        public static float Distance(this Vector3 a, Vector3 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            float dz = a.z - b.z;
            return MathF.Sqrt(dx * dx + dy * dy + dz * dz);
        }

        public static Vector3 Normalized(this Vector3 v)
        {
            float length = MathF.Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (length > 0.00001f)
                return new Vector3(v.x / length, v.y / length, v.z / length);
            return Vector3.Zero;
        }

        public static float Magnitude(this Vector3 v)
        {
            return MathF.Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        }
    }
}
