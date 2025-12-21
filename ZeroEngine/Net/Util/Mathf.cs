using System;
using EngineCore.ECS;

namespace EngineCore
{
    /// <summary>
    /// Math utility class - Unity-style math helpers
    /// </summary>
    public static class Mathf
    {
        public const float PI = MathF.PI;
        public const float Deg2Rad = PI / 180f;
        public const float Rad2Deg = 180f / PI;
        public const float Epsilon = 1e-6f;

        // ==================== BASIC MATH ====================

        public static float Abs(float value) => MathF.Abs(value);
        public static float Sign(float value) => MathF.Sign(value);
        public static float Sqrt(float value) => MathF.Sqrt(value);
        public static float Pow(float value, float power) => MathF.Pow(value, power);
        public static float Floor(float value) => MathF.Floor(value);
        public static float Ceil(float value) => MathF.Ceiling(value);
        public static float Round(float value) => MathF.Round(value);
        public static int FloorToInt(float value) => (int)MathF.Floor(value);
        public static int CeilToInt(float value) => (int)MathF.Ceiling(value);
        public static int RoundToInt(float value) => (int)MathF.Round(value);

        public static float Min(float a, float b) => a < b ? a : b;
        public static float Max(float a, float b) => a > b ? a : b;
        public static float Min(params float[] values)
        {
            if (values.Length == 0) return 0f;
            float min = values[0];
            for (int i = 1; i < values.Length; i++)
                if (values[i] < min) min = values[i];
            return min;
        }
        public static float Max(params float[] values)
        {
            if (values.Length == 0) return 0f;
            float max = values[0];
            for (int i = 1; i < values.Length; i++)
                if (values[i] > max) max = values[i];
            return max;
        }

        public static int Min(int a, int b) => a < b ? a : b;
        public static int Max(int a, int b) => a > b ? a : b;

        /// <summary>
        /// Clamp a value between min and max
        /// </summary>
        public static float Clamp(float value, float min, float max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        /// <summary>
        /// Clamp a value between 0 and 1
        /// </summary>
        public static float Clamp01(float value)
        {
            return Clamp(value, 0f, 1f);
        }

        /// <summary>
        /// Linear interpolation between a and b by t
        /// </summary>
        public static float Lerp(float a, float b, float t)
        {
            t = Clamp01(t);
            return a + (b - a) * t;
        }

        /// <summary>
        /// Linear interpolation without clamping t
        /// </summary>
        public static float LerpUnclamped(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        /// <summary>
        /// Inverse lerp - find t value for a value between a and b
        /// </summary>
        public static float InverseLerp(float a, float b, float value)
        {
            if (Abs(a - b) < Epsilon)
                return 0f;
            return Clamp01((value - a) / (b - a));
        }

        /// <summary>
        /// Smooth interpolation (ease in/out)
        /// </summary>
        public static float SmoothStep(float a, float b, float t)
        {
            t = Clamp01(t);
            t = t * t * (3f - 2f * t);
            return a + (b - a) * t;
        }

        // ==================== TRIGONOMETRY ====================

        public static float Sin(float radians) => MathF.Sin(radians);
        public static float Cos(float radians) => MathF.Cos(radians);
        public static float Tan(float radians) => MathF.Tan(radians);
        public static float Asin(float value) => MathF.Asin(value);
        public static float Acos(float value) => MathF.Acos(value);
        public static float Atan(float value) => MathF.Atan(value);
        public static float Atan2(float y, float x) => MathF.Atan2(y, x);

        // ==================== MOVEMENT & ANIMATION ====================

        /// <summary>
        /// Move current towards target by maxDelta
        /// </summary>
        public static float MoveTowards(float current, float target, float maxDelta)
        {
            if (Abs(target - current) <= maxDelta)
                return target;
            return current + Sign(target - current) * maxDelta;
        }

        /// <summary>
        /// Smoothly interpolate between current and target
        /// </summary>
        public static float SmoothDamp(float current, float target, ref float currentVelocity, 
                                      float smoothTime, float maxSpeed, float deltaTime)
        {
            smoothTime = Max(0.0001f, smoothTime);
            float omega = 2f / smoothTime;
            float x = omega * deltaTime;
            float exp = 1f / (1f + x + 0.48f * x * x + 0.235f * x * x * x);
            float change = current - target;
            float originalTo = target;
            
            float maxChange = maxSpeed * smoothTime;
            change = Clamp(change, -maxChange, maxChange);
            target = current - change;
            
            float temp = (currentVelocity + omega * change) * deltaTime;
            currentVelocity = (currentVelocity - omega * temp) * exp;
            float output = target + (change + temp) * exp;
            
            if (originalTo - current > 0f == output > originalTo)
            {
                output = originalTo;
                currentVelocity = (output - originalTo) / deltaTime;
            }
            
            return output;
        }

        /// <summary>
        /// Repeat value between 0 and length
        /// </summary>
        public static float Repeat(float t, float length)
        {
            return Clamp(t - Floor(t / length) * length, 0f, length);
        }

        /// <summary>
        /// Ping-pong value between 0 and length
        /// </summary>
        public static float PingPong(float t, float length)
        {
            t = Repeat(t, length * 2f);
            return length - Abs(t - length);
        }

        // ==================== ANGLES ====================

        /// <summary>
        /// Returns the shortest difference between two angles in degrees
        /// </summary>
        public static float DeltaAngle(float current, float target)
        {
            float delta = Repeat(target - current, 360f);
            if (delta > 180f)
                delta -= 360f;
            return delta;
        }

        /// <summary>
        /// Move angle towards target by maxDelta (handles wrapping)
        /// </summary>
        public static float MoveTowardsAngle(float current, float target, float maxDelta)
        {
            float delta = DeltaAngle(current, target);
            if (-maxDelta < delta && delta < maxDelta)
                return target;
            target = current + delta;
            return MoveTowards(current, target, maxDelta);
        }

        /// <summary>
        /// Lerp between two angles (handles wrapping)
        /// </summary>
        public static float LerpAngle(float a, float b, float t)
        {
            float delta = Repeat(b - a, 360f);
            if (delta > 180f)
                delta -= 360f;
            return a + delta * Clamp01(t);
        }

        // ==================== UTILITY ====================

        /// <summary>
        /// Check if two floats are approximately equal
        /// </summary>
        public static bool Approximately(float a, float b)
        {
            return Abs(a - b) < Epsilon;
        }

        /// <summary>
        /// Remap a value from one range to another
        /// </summary>
        public static float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
        {
            float t = InverseLerp(fromMin, fromMax, value);
            return LerpUnclamped(toMin, toMax, t);
        }
    }

    /// <summary>
    /// Extended Vector3 utilities
    /// </summary>
    public static class Vector3Extensions
    {
        /// <summary>
        /// Linear interpolation between two vectors
        /// </summary>
        public static Vector3 Lerp(Vector3 a, Vector3 b, float t)
        {
            t = Mathf.Clamp01(t);
            return new Vector3(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t
            );
        }

        /// <summary>
        /// Lerp without clamping t
        /// </summary>
        public static Vector3 LerpUnclamped(Vector3 a, Vector3 b, float t)
        {
            return new Vector3(
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t
            );
        }

        /// <summary>
        /// Move towards target by maxDistanceDelta
        /// </summary>
        public static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta)
        {
            Vector3 diff = new Vector3(
                target.x - current.x,
                target.y - current.y,
                target.z - current.z
            );
            float magnitude = MathF.Sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            
            if (magnitude <= maxDistanceDelta || magnitude < Mathf.Epsilon)
                return target;
            
            return new Vector3(
                current.x + diff.x / magnitude * maxDistanceDelta,
                current.y + diff.y / magnitude * maxDistanceDelta,
                current.z + diff.z / magnitude * maxDistanceDelta
            );
        }

        /// <summary>
        /// Dot product
        /// </summary>
        public static float Dot(Vector3 a, Vector3 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        /// <summary>
        /// Cross product
        /// </summary>
        public static Vector3 Cross(Vector3 a, Vector3 b)
        {
            return new Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        /// <summary>
        /// Distance between two points
        /// </summary>
        public static float Distance(Vector3 a, Vector3 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            float dz = a.z - b.z;
            return MathF.Sqrt(dx * dx + dy * dy + dz * dz);
        }

        /// <summary>
        /// Project a vector onto another
        /// </summary>
        public static Vector3 Project(Vector3 vector, Vector3 onNormal)
        {
            float sqrMag = Dot(onNormal, onNormal);
            if (sqrMag < Mathf.Epsilon)
                return Vector3.Zero;
            
            float dot = Dot(vector, onNormal);
            return new Vector3(
                onNormal.x * dot / sqrMag,
                onNormal.y * dot / sqrMag,
                onNormal.z * dot / sqrMag
            );
        }
    }

    /// <summary>
    /// Extended Quaternion utilities
    /// </summary>
    public static class QuaternionExtensions
    {
        /// <summary>
        /// Create a quaternion from Euler angles (in degrees)
        /// </summary>
        public static Quaternion Euler(float x, float y, float z)
        {
            x *= Mathf.Deg2Rad * 0.5f;
            y *= Mathf.Deg2Rad * 0.5f;
            z *= Mathf.Deg2Rad * 0.5f;
            
            float cx = MathF.Cos(x);
            float sx = MathF.Sin(x);
            float cy = MathF.Cos(y);
            float sy = MathF.Sin(y);
            float cz = MathF.Cos(z);
            float sz = MathF.Sin(z);
            
            return new Quaternion(
                sx * cy * cz - cx * sy * sz,
                cx * sy * cz + sx * cy * sz,
                cx * cy * sz - sx * sy * cz,
                cx * cy * cz + sx * sy * sz
            );
        }

        /// <summary>
        /// Create a quaternion from Euler angles vector (in degrees)
        /// </summary>
        public static Quaternion Euler(Vector3 euler)
        {
            return Euler(euler.x, euler.y, euler.z);
        }

        /// <summary>
        /// Spherical linear interpolation between quaternions
        /// </summary>
        public static Quaternion Slerp(Quaternion a, Quaternion b, float t)
        {
            t = Mathf.Clamp01(t);
            
            float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
            
            // If negative dot, negate one quaternion to take shorter path
            if (dot < 0.0f)
            {
                b = new Quaternion(-b.x, -b.y, -b.z, -b.w);
                dot = -dot;
            }
            
            // If quaternions are very close, use linear interpolation
            if (dot > 0.9995f)
            {
                return new Quaternion(
                    a.x + (b.x - a.x) * t,
                    a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t,
                    a.w + (b.w - a.w) * t
                );
            }
            
            float theta = MathF.Acos(dot);
            float sinTheta = MathF.Sin(theta);
            float wa = MathF.Sin((1.0f - t) * theta) / sinTheta;
            float wb = MathF.Sin(t * theta) / sinTheta;
            
            return new Quaternion(
                a.x * wa + b.x * wb,
                a.y * wa + b.y * wb,
                a.z * wa + b.z * wb,
                a.w * wa + b.w * wb
            );
        }

        /// <summary>
        /// Look rotation - create quaternion that looks along forward with up as reference
        /// </summary>
        public static Quaternion LookRotation(Vector3 forward, Vector3 up)
        {
            // Normalize forward
            float fMag = MathF.Sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
            if (fMag < Mathf.Epsilon)
                return Quaternion.Identity;
            
            forward = new Vector3(forward.x / fMag, forward.y / fMag, forward.z / fMag);
            
            // Calculate right vector
            Vector3 right = Vector3Extensions.Cross(up, forward);
            float rMag = MathF.Sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
            if (rMag < Mathf.Epsilon)
                return Quaternion.Identity;
            right = new Vector3(right.x / rMag, right.y / rMag, right.z / rMag);
            
            // Recalculate up
            up = Vector3Extensions.Cross(forward, right);
            
            // Build quaternion from matrix
            float m00 = right.x, m01 = right.y, m02 = right.z;
            float m10 = up.x, m11 = up.y, m12 = up.z;
            float m20 = forward.x, m21 = forward.y, m22 = forward.z;
            
            float trace = m00 + m11 + m22;
            
            if (trace > 0)
            {
                float s = 0.5f / MathF.Sqrt(trace + 1.0f);
                return new Quaternion(
                    (m12 - m21) * s,
                    (m20 - m02) * s,
                    (m01 - m10) * s,
                    0.25f / s
                );
            }
            else if (m00 > m11 && m00 > m22)
            {
                float s = 2.0f * MathF.Sqrt(1.0f + m00 - m11 - m22);
                return new Quaternion(
                    0.25f * s,
                    (m10 + m01) / s,
                    (m20 + m02) / s,
                    (m12 - m21) / s
                );
            }
            else if (m11 > m22)
            {
                float s = 2.0f * MathF.Sqrt(1.0f + m11 - m00 - m22);
                return new Quaternion(
                    (m10 + m01) / s,
                    0.25f * s,
                    (m21 + m12) / s,
                    (m20 - m02) / s
                );
            }
            else
            {
                float s = 2.0f * MathF.Sqrt(1.0f + m22 - m00 - m11);
                return new Quaternion(
                    (m20 + m02) / s,
                    (m21 + m12) / s,
                    0.25f * s,
                    (m01 - m10) / s
                );
            }
        }
    }
}
