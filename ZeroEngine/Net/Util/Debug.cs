using System;
using System.Collections.Generic;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Mathematics;
using EngineCore.ECS;
using EngineCore.Rendering;

namespace EngineCore
{
    /// <summary>
    /// Debug utility class for drawing lines, rays, and logging
    /// </summary>
    public static class Debug
    {
        private static List<DebugLine> _lines = new List<DebugLine>();
        private static Shader _debugShader;
        private static int _vao, _vbo;
        private static bool _initialized = false;

        private struct DebugLine
        {
            public Vector3 Start;
            public Vector3 End;
            public Color4 Color;
            public float Duration;
            public float TimeRemaining;
        }

        /// <summary>
        /// Initialize debug rendering
        /// </summary>
        public static void Initialize()
        {
            if (_initialized) return;

            // Create debug shader
            string vertexShader = @"
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;

out vec4 Color;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    Color = aColor;
    gl_Position = projection * view * vec4(aPosition, 1.0);
}
";

            string fragmentShader = @"
#version 330 core
in vec4 Color;
out vec4 FragColor;

void main()
{
    FragColor = Color;
}
";

            _debugShader = new Shader(vertexShader, fragmentShader);

            // Create VAO and VBO for line rendering
            _vao = GL.GenVertexArray();
            _vbo = GL.GenBuffer();

            GL.BindVertexArray(_vao);
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);

            // Position attribute
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, 7 * sizeof(float), 0);

            // Color attribute
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 4, VertexAttribPointerType.Float, false, 7 * sizeof(float), 3 * sizeof(float));

            GL.BindVertexArray(0);

            _initialized = true;
            Console.WriteLine("Debug rendering initialized");
        }

        /// <summary>
        /// Update debug system (removes expired lines)
        /// </summary>
        public static void Update(float deltaTime)
        {
            for (int i = _lines.Count - 1; i >= 0; i--)
            {
                var line = _lines[i];
                line.TimeRemaining -= deltaTime;

                if (line.TimeRemaining <= 0 && line.Duration > 0)
                {
                    _lines.RemoveAt(i);
                }
                else
                {
                    _lines[i] = line;
                }
            }
        }

        /// <summary>
        /// Draw a line in 3D space
        /// </summary>
        public static void DrawLine(Vector3 start, Vector3 end, Color4 color, float duration = 0f)
        {
            _lines.Add(new DebugLine
            {
                Start = start,
                End = end,
                Color = color,
                Duration = duration,
                TimeRemaining = duration
            });
        }

        /// <summary>
        /// Draw a line in 3D space (convenience overload)
        /// </summary>
        public static void DrawLine(Vector3 start, Vector3 end)
        {
            DrawLine(start, end, Color4.White, 0f);
        }

        /// <summary>
        /// Draw a ray from start in direction
        /// </summary>
        public static void DrawRay(Vector3 start, Vector3 direction, Color4 color, float duration = 0f)
        {
            DrawLine(start, new Vector3(
                start.x + direction.x,
                start.y + direction.y,
                start.z + direction.z
            ), color, duration);
        }

        /// <summary>
        /// Draw a ray from start in direction (convenience overload)
        /// </summary>
        public static void DrawRay(Vector3 start, Vector3 direction)
        {
            DrawRay(start, direction, Color4.White, 0f);
        }

        /// <summary>
        /// Draw a wireframe box
        /// </summary>
        public static void DrawWireBox(Vector3 center, Vector3 size, Color4 color, float duration = 0f)
        {
            Vector3 halfSize = new Vector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);

            // Bottom face
            Vector3 v0 = new Vector3(center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z);
            Vector3 v1 = new Vector3(center.x + halfSize.x, center.y - halfSize.y, center.z - halfSize.z);
            Vector3 v2 = new Vector3(center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z);
            Vector3 v3 = new Vector3(center.x - halfSize.x, center.y - halfSize.y, center.z + halfSize.z);

            // Top face
            Vector3 v4 = new Vector3(center.x - halfSize.x, center.y + halfSize.y, center.z - halfSize.z);
            Vector3 v5 = new Vector3(center.x + halfSize.x, center.y + halfSize.y, center.z - halfSize.z);
            Vector3 v6 = new Vector3(center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z);
            Vector3 v7 = new Vector3(center.x - halfSize.x, center.y + halfSize.y, center.z + halfSize.z);

            // Bottom
            DrawLine(v0, v1, color, duration);
            DrawLine(v1, v2, color, duration);
            DrawLine(v2, v3, color, duration);
            DrawLine(v3, v0, color, duration);

            // Top
            DrawLine(v4, v5, color, duration);
            DrawLine(v5, v6, color, duration);
            DrawLine(v6, v7, color, duration);
            DrawLine(v7, v4, color, duration);

            // Sides
            DrawLine(v0, v4, color, duration);
            DrawLine(v1, v5, color, duration);
            DrawLine(v2, v6, color, duration);
            DrawLine(v3, v7, color, duration);
        }

        /// <summary>
        /// Draw a wireframe sphere (simplified as 3 circles)
        /// </summary>
        public static void DrawWireSphere(Vector3 center, float radius, Color4 color, float duration = 0f)
        {
            int segments = 16;
            float angleStep = MathF.PI * 2f / segments;

            // XY circle
            for (int i = 0; i < segments; i++)
            {
                float angle1 = i * angleStep;
                float angle2 = (i + 1) * angleStep;

                Vector3 p1 = new Vector3(
                    center.x + MathF.Cos(angle1) * radius,
                    center.y + MathF.Sin(angle1) * radius,
                    center.z
                );
                Vector3 p2 = new Vector3(
                    center.x + MathF.Cos(angle2) * radius,
                    center.y + MathF.Sin(angle2) * radius,
                    center.z
                );
                DrawLine(p1, p2, color, duration);
            }

            // XZ circle
            for (int i = 0; i < segments; i++)
            {
                float angle1 = i * angleStep;
                float angle2 = (i + 1) * angleStep;

                Vector3 p1 = new Vector3(
                    center.x + MathF.Cos(angle1) * radius,
                    center.y,
                    center.z + MathF.Sin(angle1) * radius
                );
                Vector3 p2 = new Vector3(
                    center.x + MathF.Cos(angle2) * radius,
                    center.y,
                    center.z + MathF.Sin(angle2) * radius
                );
                DrawLine(p1, p2, color, duration);
            }

            // YZ circle
            for (int i = 0; i < segments; i++)
            {
                float angle1 = i * angleStep;
                float angle2 = (i + 1) * angleStep;

                Vector3 p1 = new Vector3(
                    center.x,
                    center.y + MathF.Cos(angle1) * radius,
                    center.z + MathF.Sin(angle1) * radius
                );
                Vector3 p2 = new Vector3(
                    center.x,
                    center.y + MathF.Cos(angle2) * radius,
                    center.z + MathF.Sin(angle2) * radius
                );
                DrawLine(p1, p2, color, duration);
            }
        }

        /// <summary>
        /// Render all debug lines
        /// </summary>
        public static void Render(Camera camera)
        {
            if (!_initialized || _lines.Count == 0)
                return;

            // Prepare vertex data
            float[] vertices = new float[_lines.Count * 2 * 7]; // 2 vertices per line, 7 floats per vertex
            int index = 0;

            foreach (var line in _lines)
            {
                // Start vertex
                vertices[index++] = line.Start.x;
                vertices[index++] = line.Start.y;
                vertices[index++] = line.Start.z;
                vertices[index++] = line.Color.R;
                vertices[index++] = line.Color.G;
                vertices[index++] = line.Color.B;
                vertices[index++] = line.Color.A;

                // End vertex
                vertices[index++] = line.End.x;
                vertices[index++] = line.End.y;
                vertices[index++] = line.End.z;
                vertices[index++] = line.Color.R;
                vertices[index++] = line.Color.G;
                vertices[index++] = line.Color.B;
                vertices[index++] = line.Color.A;
            }

            // Upload to GPU
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, vertices.Length * sizeof(float), vertices, BufferUsageHint.DynamicDraw);

            // Render
            _debugShader.Use();
            _debugShader.SetMatrix4("view", camera.GetViewMatrix());
            _debugShader.SetMatrix4("projection", camera.GetProjectionMatrix());

            GL.BindVertexArray(_vao);
            GL.DrawArrays(PrimitiveType.Lines, 0, _lines.Count * 2);
            GL.BindVertexArray(0);
        }

        /// <summary>
        /// Clear all debug lines
        /// </summary>
        public static void Clear()
        {
            _lines.Clear();
        }

        // ==================== LOGGING ====================

        /// <summary>
        /// Log a message to the console
        /// </summary>
        public static void Log(object message)
        {
            Console.WriteLine($"[LOG] {message}");
        }

        /// <summary>
        /// Log a warning message
        /// </summary>
        public static void LogWarning(object message)
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"[WARNING] {message}");
            Console.ResetColor();
        }

        /// <summary>
        /// Log an error message
        /// </summary>
        public static void LogError(object message)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"[ERROR] {message}");
            Console.ResetColor();
        }

        /// <summary>
        /// Assert a condition - logs error if false
        /// </summary>
        public static void Assert(bool condition, string message = "Assertion failed")
        {
            if (!condition)
            {
                LogError($"Assertion failed: {message}");
                throw new Exception(message);
            }
        }
    }

    /// <summary>
    /// FPS counter overlay
    /// </summary>
    public class FPSCounter
    {
        private float _fpsTimer;
        private int _frameCount;
        private float _currentFPS;

        public float CurrentFPS => _currentFPS;

        public void Update(float deltaTime)
        {
            _fpsTimer += deltaTime;
            _frameCount++;

            if (_fpsTimer >= 1.0f)
            {
                _currentFPS = _frameCount / _fpsTimer;
                _fpsTimer = 0f;
                _frameCount = 0;
            }
        }

        public void Draw()
        {
            // This would render text to screen
            // For now, just log periodically
            Console.WriteLine($"FPS: {_currentFPS:F1}");
        }
    }
}
