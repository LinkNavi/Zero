using System;
using System.Collections.Generic;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Mathematics;

namespace EngineCore.Rendering
{
    // ==================== SHADER ====================
    
    /// <summary>
    /// Wrapper for OpenGL shader programs
    /// HOW IT WORKS: Shaders are small programs that run on the GPU.
    /// - Vertex Shader: Transforms 3D positions to screen space
    /// - Fragment Shader: Determines the color of each pixel
    /// </summary>
    public class Shader : IDisposable
    {
        public int Handle { get; private set; }
        private Dictionary<string, int> _uniformLocations = new Dictionary<string, int>();
        
        public Shader(string vertexSource, string fragmentSource)
        {
            // Step 1: Compile vertex shader
            int vertexShader = GL.CreateShader(ShaderType.VertexShader);
            GL.ShaderSource(vertexShader, vertexSource);
            GL.CompileShader(vertexShader);
            CheckShaderCompilation(vertexShader, "Vertex");
            
            // Step 2: Compile fragment shader
            int fragmentShader = GL.CreateShader(ShaderType.FragmentShader);
            GL.ShaderSource(fragmentShader, fragmentSource);
            GL.CompileShader(fragmentShader);
            CheckShaderCompilation(fragmentShader, "Fragment");
            
            // Step 3: Link shaders into a program
            Handle = GL.CreateProgram();
            GL.AttachShader(Handle, vertexShader);
            GL.AttachShader(Handle, fragmentShader);
            GL.LinkProgram(Handle);
            CheckProgramLinking(Handle);
            
            // Step 4: Cleanup - we don't need the individual shaders anymore
            GL.DetachShader(Handle, vertexShader);
            GL.DetachShader(Handle, fragmentShader);
            GL.DeleteShader(vertexShader);
            GL.DeleteShader(fragmentShader);
        }
        
        public void Use()
        {
            // Tell OpenGL to use this shader for drawing
            GL.UseProgram(Handle);
        }
        
        // Uniforms are variables we pass from C# to the shader
        // Examples: transformation matrices, colors, texture samplers
        
        public void SetInt(string name, int value)
        {
            GL.Uniform1(GetUniformLocation(name), value);
        }
        
        public void SetFloat(string name, float value)
        {
            GL.Uniform1(GetUniformLocation(name), value);
        }
        
        public void SetVector3(string name, Vector3 value)
        {
            GL.Uniform3(GetUniformLocation(name), value);
        }
        
        public void SetVector4(string name, Vector4 value)
        {
            GL.Uniform4(GetUniformLocation(name), value);
        }
        
        public void SetMatrix4(string name, Matrix4 value)
        {
            GL.UniformMatrix4(GetUniformLocation(name), false, ref value);
        }
        
        private int GetUniformLocation(string name)
        {
            // Cache uniform locations for performance
            if (_uniformLocations.TryGetValue(name, out int location))
                return location;
            
            location = GL.GetUniformLocation(Handle, name);
            _uniformLocations[name] = location;
            
            if (location == -1)
                Console.WriteLine($"Warning: Uniform '{name}' not found in shader");
            
            return location;
        }
        
        private void CheckShaderCompilation(int shader, string type)
        {
            GL.GetShader(shader, ShaderParameter.CompileStatus, out int success);
            if (success == 0)
            {
                string infoLog = GL.GetShaderInfoLog(shader);
                throw new Exception($"{type} Shader compilation failed: {infoLog}");
            }
        }
        
        private void CheckProgramLinking(int program)
        {
            GL.GetProgram(program, GetProgramParameterName.LinkStatus, out int success);
            if (success == 0)
            {
                string infoLog = GL.GetProgramInfoLog(program);
                throw new Exception($"Shader program linking failed: {infoLog}");
            }
        }
        
        public void Dispose()
        {
            GL.DeleteProgram(Handle);
        }
    }
    
    // ==================== MESH ====================
    
    /// <summary>
    /// Represents 3D geometry
    /// HOW IT WORKS: 
    /// - Vertices: Array of positions, normals, UVs, etc.
    /// - Indices: Which vertices form triangles (0,1,2 = first triangle)
    /// - VAO: Describes how the vertex data is laid out
    /// - VBOs: Actual data stored on GPU
    /// </summary>
    public class Mesh : IDisposable
    {
        // OpenGL handles
        private int _vao; // Vertex Array Object
        private int _vbo; // Vertex Buffer Object
        private int _ebo; // Element Buffer Object (indices)
        
        public int VertexCount { get; private set; }
        public int IndexCount { get; private set; }
        
        // Vertex structure - what data each vertex has
        public struct Vertex
        {
            public Vector3 Position;
            public Vector3 Normal;
            public Vector2 TexCoord;
            public Vector4 Color;
        }
        
        public Mesh(Vertex[] vertices, uint[] indices)
        {
            VertexCount = vertices.Length;
            IndexCount = indices.Length;
            
            // Step 1: Generate and bind VAO
            // VAO stores the vertex attribute configuration
            _vao = GL.GenVertexArray();
            GL.BindVertexArray(_vao);
            
            // Step 2: Create and fill VBO with vertex data
            _vbo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, 
                         vertices.Length * System.Runtime.InteropServices.Marshal.SizeOf<Vertex>(), 
                         vertices, 
                         BufferUsageHint.StaticDraw);
            
            // Step 3: Tell OpenGL how to interpret the vertex data
            int stride = System.Runtime.InteropServices.Marshal.SizeOf<Vertex>();
            
            // Position attribute (location = 0 in shader)
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, stride, 0);
            
            // Normal attribute (location = 1)
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 3, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("Normal"));
            
            // TexCoord attribute (location = 2)
            GL.EnableVertexAttribArray(2);
            GL.VertexAttribPointer(2, 2, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("TexCoord"));
            
            // Color attribute (location = 3)
            GL.EnableVertexAttribArray(3);
            GL.VertexAttribPointer(3, 4, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("Color"));
            
            // Step 4: Create and fill EBO with index data
            _ebo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _ebo);
            GL.BufferData(BufferTarget.ElementArrayBuffer, 
                         indices.Length * sizeof(uint), 
                         indices, 
                         BufferUsageHint.StaticDraw);
            
            // Unbind
            GL.BindVertexArray(0);
        }
        
        public void Draw()
        {
            // Bind the VAO and draw
            GL.BindVertexArray(_vao);
            GL.DrawElements(PrimitiveType.Triangles, IndexCount, DrawElementsType.UnsignedInt, 0);
            GL.BindVertexArray(0);
        }
        
        public void Dispose()
        {
            GL.DeleteBuffer(_vbo);
            GL.DeleteBuffer(_ebo);
            GL.DeleteVertexArray(_vao);
        }
        
        // ==================== PRIMITIVE MESHES ====================
        
        public static Mesh CreateCube()
        {
            var vertices = new Vertex[]
            {
                // Front face (Z+)
                new Vertex { Position = new Vector3(-0.5f, -0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
                
                // Back face (Z-)
                new Vertex { Position = new Vector3( 0.5f, -0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f, -0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
                
                // Top face (Y+)
                new Vertex { Position = new Vector3(-0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
                
                // Bottom face (Y-)
                new Vertex { Position = new Vector3(-0.5f, -0.5f, -0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f, -0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f,  0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f, -0.5f,  0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
                
                // Right face (X+)
                new Vertex { Position = new Vector3( 0.5f, -0.5f,  0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f, -0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f, -0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f,  0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
                
                // Left face (X-)
                new Vertex { Position = new Vector3(-0.5f, -0.5f, -0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f, -0.5f,  0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f,  0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f, -0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
            };
            
            var indices = new uint[]
            {
                0, 1, 2,  2, 3, 0,    // Front
                4, 5, 6,  6, 7, 4,    // Back
                8, 9, 10, 10, 11, 8,  // Top
                12, 13, 14, 14, 15, 12, // Bottom
                16, 17, 18, 18, 19, 16, // Right
                20, 21, 22, 22, 23, 20  // Left
            };
            
            return new Mesh(vertices, indices);
        }
        
        public static Mesh CreateQuad()
        {
            var vertices = new Vertex[]
            {
                new Vertex { Position = new Vector3(-0.5f, -0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(0.5f, -0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(0.5f, 0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1) },
                new Vertex { Position = new Vector3(-0.5f, 0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1) },
            };
            
            var indices = new uint[] { 0, 1, 2, 2, 3, 0 };
            
            return new Mesh(vertices, indices);
        }
    }
    
    // ==================== TEXTURE ====================
    
    /// <summary>
    /// Wrapper for OpenGL textures
    /// </summary>
    public class Texture : IDisposable
    {
        public int Handle { get; private set; }
        public int Width { get; private set; }
        public int Height { get; private set; }
        
        public Texture(int width, int height, IntPtr data)
        {
            Width = width;
            Height = height;
            
            Handle = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, Handle);
            
            // Upload texture data
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba, 
                         width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, data);
            
            // Set texture parameters
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.Repeat);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.Repeat);
            
            GL.BindTexture(TextureTarget.Texture2D, 0);
        }
        
        public void Bind(int unit = 0)
        {
            GL.ActiveTexture(TextureUnit.Texture0 + unit);
            GL.BindTexture(TextureTarget.Texture2D, Handle);
        }
        
        public void Dispose()
        {
            GL.DeleteTexture(Handle);
        }
    }
    
    // ==================== MATERIAL ====================
    
    /// <summary>
    /// Combines a shader with properties (colors, textures, etc.)
    /// </summary>
    public class Material
    {
        public Shader Shader { get; set; }
        public Vector4 Color { get; set; } = new Vector4(1, 1, 1, 1);
        public Texture MainTexture { get; set; }
        
        private Dictionary<string, object> _properties = new Dictionary<string, object>();
        
        public Material(Shader shader)
        {
            Shader = shader;
        }
        
        public void SetFloat(string name, float value) => _properties[name] = value;
        public void SetVector3(string name, Vector3 value) => _properties[name] = value;
        public void SetVector4(string name, Vector4 value) => _properties[name] = value;
        public void SetTexture(string name, Texture value) => _properties[name] = value;
        
        public void Apply()
        {
            Shader.Use();
            Shader.SetVector4("_Color", Color);
            
            if (MainTexture != null)
            {
                MainTexture.Bind(0);
                Shader.SetInt("_MainTex", 0);
            }
            
            // Apply custom properties
            foreach (var prop in _properties)
            {
                switch (prop.Value)
                {
                    case float f: Shader.SetFloat(prop.Key, f); break;
                    case Vector3 v3: Shader.SetVector3(prop.Key, v3); break;
                    case Vector4 v4: Shader.SetVector4(prop.Key, v4); break;
                    case Texture tex: 
                        // Handle additional textures
                        break;
                }
            }
        }
    }
    
    // ==================== CAMERA ====================
    
    /// <summary>
    /// Camera that creates view and projection matrices
    /// </summary>
    public class Camera
    {
        public Vector3 Position { get; set; } = new Vector3(0, 0, 5);
        public Vector3 Target { get; set; } = Vector3.Zero;
        public Vector3 Up { get; set; } = Vector3.UnitY;
        
        public float FieldOfView { get; set; } = MathHelper.PiOver4; // 45 degrees
        public float AspectRatio { get; set; } = 16f / 9f;
        public float NearClip { get; set; } = 0.1f;
        public float FarClip { get; set; } = 100f;
        
        public Matrix4 GetViewMatrix()
        {
            return Matrix4.LookAt(Position, Target, Up);
        }
        
        public Matrix4 GetProjectionMatrix()
        {
            return Matrix4.CreatePerspectiveFieldOfView(FieldOfView, AspectRatio, NearClip, FarClip);
        }
    }
    
    // ==================== RENDERER ====================
    
    /// <summary>
    /// Main rendering system that ties everything together
    /// </summary>
    public class Renderer
    {
        private Camera _camera;
        
        public Renderer(Camera camera)
        {
            _camera = camera;
            
            // Enable depth testing so objects in front hide objects behind
            GL.Enable(EnableCap.DepthTest);
            GL.DepthFunc(DepthFunction.Less);
            
            // Enable backface culling for performance
            GL.Enable(EnableCap.CullFace);
            GL.CullFace(CullFaceMode.Back);
        }
        
        public void Clear(Vector4 clearColor)
        {
            GL.ClearColor(clearColor.X, clearColor.Y, clearColor.Z, clearColor.W);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
        }
        
        public void DrawMesh(Mesh mesh, Material material, Matrix4 modelMatrix)
        {
            // Apply the material (sets up shader and properties)
            material.Apply();
            
            // Set transformation matrices
            material.Shader.SetMatrix4("model", modelMatrix);
            material.Shader.SetMatrix4("view", _camera.GetViewMatrix());
            material.Shader.SetMatrix4("projection", _camera.GetProjectionMatrix());
            
            // Draw the mesh
            mesh.Draw();
        }
        
        public void SetCamera(Camera camera)
        {
            _camera = camera;
        }
    }
    
    // ==================== DEFAULT SHADERS ====================
    
    public static class DefaultShaders
    {
        public static string BasicVertexShader = @"
#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPosition, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    Color = aColor;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
";
        
        public static string BasicFragmentShader = @"
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 Color;

out vec4 FragColor;

uniform vec4 _Color;
uniform sampler2D _MainTex;

void main()
{
    // Simple lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    vec3 lighting = vec3(0.3 + 0.7 * diff); // Ambient + diffuse
    
    vec4 texColor = texture(_MainTex, TexCoord);
    FragColor = _Color * texColor * Color * vec4(lighting, 1.0);
}
";
    }
}
