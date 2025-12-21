using System;
using System.Collections.Generic;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Mathematics;

namespace EngineCore.Rendering
{
    // ==================== SHADER ====================
    
    public class Shader : IDisposable
    {
        public int Handle { get; private set; }
        private Dictionary<string, int> _uniformLocations = new Dictionary<string, int>();
        
        public Shader(string vertexSource, string fragmentSource)
        {
            int vertexShader = GL.CreateShader(ShaderType.VertexShader);
            GL.ShaderSource(vertexShader, vertexSource);
            GL.CompileShader(vertexShader);
            CheckShaderCompilation(vertexShader, "Vertex");
            
            int fragmentShader = GL.CreateShader(ShaderType.FragmentShader);
            GL.ShaderSource(fragmentShader, fragmentSource);
            GL.CompileShader(fragmentShader);
            CheckShaderCompilation(fragmentShader, "Fragment");
            
            Handle = GL.CreateProgram();
            GL.AttachShader(Handle, vertexShader);
            GL.AttachShader(Handle, fragmentShader);
            GL.LinkProgram(Handle);
            CheckProgramLinking(Handle);
            
            GL.DetachShader(Handle, vertexShader);
            GL.DetachShader(Handle, fragmentShader);
            GL.DeleteShader(vertexShader);
            GL.DeleteShader(fragmentShader);
        }
        
        public void Use()
        {
            GL.UseProgram(Handle);
        }
        
        public void SetInt(string name, int value)
        {
            GL.Uniform1(GetUniformLocation(name), value);
        }
        
        public void SetFloat(string name, float value)
        {
            GL.Uniform1(GetUniformLocation(name), value);
        }
        
        public void SetVector2(string name, Vector2 value)
        {
            GL.Uniform2(GetUniformLocation(name), value);
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
    
    public class Mesh : IDisposable
    {
        private int _vao;
        private int _vbo;
        private int _ebo;
        
        public int VertexCount { get; private set; }
        public int IndexCount { get; private set; }
        
        public struct Vertex
        {
            public Vector3 Position;
            public Vector3 Normal;
            public Vector2 TexCoord;
            public Vector4 Color;
            public Vector3 Tangent;
        }
        
        public Mesh(Vertex[] vertices, uint[] indices)
        {
            VertexCount = vertices.Length;
            IndexCount = indices.Length;
            
            _vao = GL.GenVertexArray();
            GL.BindVertexArray(_vao);
            
            _vbo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, 
                         vertices.Length * System.Runtime.InteropServices.Marshal.SizeOf<Vertex>(), 
                         vertices, 
                         BufferUsageHint.StaticDraw);
            
            int stride = System.Runtime.InteropServices.Marshal.SizeOf<Vertex>();
            
            // Position
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 3, VertexAttribPointerType.Float, false, stride, 0);
            
            // Normal
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 3, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("Normal"));
            
            // TexCoord
            GL.EnableVertexAttribArray(2);
            GL.VertexAttribPointer(2, 2, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("TexCoord"));
            
            // Color
            GL.EnableVertexAttribArray(3);
            GL.VertexAttribPointer(3, 4, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("Color"));
            
            // Tangent
            GL.EnableVertexAttribArray(4);
            GL.VertexAttribPointer(4, 3, VertexAttribPointerType.Float, false, stride, 
                                  System.Runtime.InteropServices.Marshal.OffsetOf<Vertex>("Tangent"));
            
            _ebo = GL.GenBuffer();
            GL.BindBuffer(BufferTarget.ElementArrayBuffer, _ebo);
            GL.BufferData(BufferTarget.ElementArrayBuffer, 
                         indices.Length * sizeof(uint), 
                         indices, 
                         BufferUsageHint.StaticDraw);
            
            GL.BindVertexArray(0);
        }
        
        public void Draw()
        {
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
                // Front face
                new Vertex { Position = new Vector3(-0.5f, -0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                
                // Back face
                new Vertex { Position = new Vector3( 0.5f, -0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(-1, 0, 0) },
                new Vertex { Position = new Vector3(-0.5f, -0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(-1, 0, 0) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(-1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 0, -1), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(-1, 0, 0) },
                
                // Top face
                new Vertex { Position = new Vector3(-0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f,  0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f, -0.5f), Normal = new Vector3(0, 1, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                
                // Bottom face
                new Vertex { Position = new Vector3(-0.5f, -0.5f, -0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f, -0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f,  0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3(-0.5f, -0.5f,  0.5f), Normal = new Vector3(0, -1, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                
                // Right face
                new Vertex { Position = new Vector3( 0.5f, -0.5f,  0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, -1) },
                new Vertex { Position = new Vector3( 0.5f, -0.5f, -0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, -1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f, -0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, -1) },
                new Vertex { Position = new Vector3( 0.5f,  0.5f,  0.5f), Normal = new Vector3(1, 0, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, -1) },
                
                // Left face
                new Vertex { Position = new Vector3(-0.5f, -0.5f, -0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, 1) },
                new Vertex { Position = new Vector3(-0.5f, -0.5f,  0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f,  0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, 1) },
                new Vertex { Position = new Vector3(-0.5f,  0.5f, -0.5f), Normal = new Vector3(-1, 0, 0), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(0, 0, 1) },
            };
            
            var indices = new uint[]
            {
                0, 1, 2,  2, 3, 0,
                4, 5, 6,  6, 7, 4,
                8, 9, 10, 10, 11, 8,
                12, 13, 14, 14, 15, 12,
                16, 17, 18, 18, 19, 16,
                20, 21, 22, 22, 23, 20
            };
            
            return new Mesh(vertices, indices);
        }
        
        public static Mesh CreateQuad()
        {
            var vertices = new Vertex[]
            {
                new Vertex { Position = new Vector3(-0.5f, -0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3(0.5f, -0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 0), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3(0.5f, 0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(1, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
                new Vertex { Position = new Vector3(-0.5f, 0.5f, 0), Normal = new Vector3(0, 0, 1), TexCoord = new Vector2(0, 1), Color = new Vector4(1, 1, 1, 1), Tangent = new Vector3(1, 0, 0) },
            };
            
            var indices = new uint[] { 0, 1, 2, 2, 3, 0 };
            
            return new Mesh(vertices, indices);
        }
        
        public static Mesh CreateSphere(int segments = 32, int rings = 16)
        {
            var vertices = new List<Vertex>();
            var indices = new List<uint>();
            
            for (int ring = 0; ring <= rings; ring++)
            {
                float phi = MathF.PI * ring / rings;
                float y = MathF.Cos(phi);
                float ringRadius = MathF.Sin(phi);
                
                for (int seg = 0; seg <= segments; seg++)
                {
                    float theta = 2 * MathF.PI * seg / segments;
                    float x = ringRadius * MathF.Cos(theta);
                    float z = ringRadius * MathF.Sin(theta);
                    
                    Vector3 pos = new Vector3(x, y, z) * 0.5f;
                    Vector3 normal = Vector3.Normalize(new Vector3(x, y, z));
                    Vector2 uv = new Vector2((float)seg / segments, (float)ring / rings);
                    
                    // Calculate tangent
                    Vector3 tangent = Vector3.Normalize(new Vector3(-MathF.Sin(theta), 0, MathF.Cos(theta)));
                    
                    vertices.Add(new Vertex
                    {
                        Position = pos,
                        Normal = normal,
                        TexCoord = uv,
                        Color = new Vector4(1, 1, 1, 1),
                        Tangent = tangent
                    });
                }
            }
            
            for (int ring = 0; ring < rings; ring++)
            {
                for (int seg = 0; seg < segments; seg++)
                {
                    uint current = (uint)(ring * (segments + 1) + seg);
                    uint next = current + (uint)segments + 1;
                    
                    indices.Add(current);
                    indices.Add(next);
                    indices.Add(current + 1);
                    
                    indices.Add(current + 1);
                    indices.Add(next);
                    indices.Add(next + 1);
                }
            }
            
            return new Mesh(vertices.ToArray(), indices.ToArray());
        }
    }
    
    // ==================== TEXTURE ====================
    
    public class Texture : IDisposable
    {
        public int Handle { get; private set; }
        public int Width { get; private set; }
        public int Height { get; private set; }
        
        public Texture(int handle, int width, int height)
        {
            Handle = handle;
            Width = width;
            Height = height;
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
    
    public class Material
    {
        public Shader Shader { get; set; }
        public Vector4 Color { get; set; } = new Vector4(1, 1, 1, 1);
        public Texture MainTexture { get; set; }
        public Texture NormalMap { get; set; }
        
        // PBR properties
        public float Metallic { get; set; } = 0.0f;
        public float Roughness { get; set; } = 0.5f;
        public float Shininess { get; set; } = 32.0f;
        public Vector3 Emission { get; set; } = Vector3.Zero;
        
        private Dictionary<string, object> _properties = new Dictionary<string, object>();
        
        public Material(Shader shader)
        {
            Shader = shader;
        }
        
        public void SetFloat(string name, float value) => _properties[name] = value;
        public void SetVector3(string name, Vector3 value) => _properties[name] = value;
        public void SetVector4(string name, Vector4 value) => _properties[name] = value;
        public void SetTexture(string name, Texture value) => _properties[name] = value;
        
        public void Apply(Vector3 viewPos)
        {
            Shader.Use();
            Shader.SetVector4("material.albedo", Color);
            Shader.SetFloat("material.metallic", Metallic);
            Shader.SetFloat("material.roughness", Roughness);
            Shader.SetFloat("material.shininess", Shininess);
            Shader.SetVector3("material.emission", Emission);
            
            if (MainTexture != null)
            {
                MainTexture.Bind(0);
                Shader.SetInt("material.albedoMap", 0);
                Shader.SetInt("material.hasAlbedoMap", 1);
            }
            else
            {
                Shader.SetInt("material.hasAlbedoMap", 0);
            }
            
            if (NormalMap != null)
            {
                NormalMap.Bind(1);
                Shader.SetInt("material.normalMap", 1);
                Shader.SetInt("material.hasNormalMap", 1);
            }
            else
            {
                Shader.SetInt("material.hasNormalMap", 0);
            }
            
            Shader.SetVector3("viewPos", viewPos);
            
            // Apply custom properties
            foreach (var prop in _properties)
            {
                switch (prop.Value)
                {
                    case float f: Shader.SetFloat(prop.Key, f); break;
                    case Vector3 v3: Shader.SetVector3(prop.Key, v3); break;
                    case Vector4 v4: Shader.SetVector4(prop.Key, v4); break;
                }
            }
        }
    }
    
    // ==================== LIGHT DATA ====================
    
    public struct LightData
    {
        public Vector3 Position;
        public Vector3 Direction;
        public Vector3 Color;
        public float Intensity;
        public int Type; // 0=Directional, 1=Point, 2=Spot
        public float Range;
        public float SpotAngle;
    }
    
    // ==================== CAMERA ====================
    
    public class Camera
    {
        public Vector3 Position { get; set; } = new Vector3(0, 0, 5);
        public Vector3 Target { get; set; } = Vector3.Zero;
        public Vector3 Up { get; set; } = Vector3.UnitY;
        
        public float FieldOfView { get; set; } = MathHelper.PiOver4;
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
    
    public class Renderer
    {
        private Camera _camera;
        private List<LightData> _lights = new List<LightData>();
        
        public Renderer(Camera camera)
        {
            _camera = camera;
            
            GL.Enable(EnableCap.DepthTest);
            GL.DepthFunc(DepthFunction.Less);
            GL.Enable(EnableCap.CullFace);
            GL.CullFace(CullFaceMode.Back);
        }
        
        public void Clear(Vector4 clearColor)
        {
            GL.ClearColor(clearColor.X, clearColor.Y, clearColor.Z, clearColor.W);
            GL.Clear(ClearBufferMask.ColorBufferBit | ClearBufferMask.DepthBufferBit);
        }
        
        public void SetLights(List<LightData> lights)
        {
            _lights = lights;
        }
        
        public void DrawMesh(Mesh mesh, Material material, Matrix4 modelMatrix)
        {
            material.Apply(_camera.Position);
            
            // Set matrices
            material.Shader.SetMatrix4("model", modelMatrix);
            material.Shader.SetMatrix4("view", _camera.GetViewMatrix());
            material.Shader.SetMatrix4("projection", _camera.GetProjectionMatrix());
            
            // Calculate normal matrix
            Matrix4 normalMatrix = Matrix4.Transpose(Matrix4.Invert(modelMatrix));
            material.Shader.SetMatrix4("normalMatrix", normalMatrix);
            
            // Set lights
            material.Shader.SetInt("numLights", Math.Min(_lights.Count, 8));
            for (int i = 0; i < Math.Min(_lights.Count, 8); i++)
            {
                var light = _lights[i];
                material.Shader.SetVector3($"lights[{i}].position", light.Position);
                material.Shader.SetVector3($"lights[{i}].direction", light.Direction);
                material.Shader.SetVector3($"lights[{i}].color", light.Color);
                material.Shader.SetFloat($"lights[{i}].intensity", light.Intensity);
                material.Shader.SetInt($"lights[{i}].type", light.Type);
                material.Shader.SetFloat($"lights[{i}].range", light.Range);
                material.Shader.SetFloat($"lights[{i}].spotAngle", light.SpotAngle);
            }
            
            mesh.Draw();
        }
        
        public void SetCamera(Camera camera)
        {
            _camera = camera;
        }
    }
    
    // ==================== IMPROVED SHADERS ====================
    
    public static class DefaultShaders
    {
        public static string LitVertexShader = @"
#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aColor;
layout (location = 4) in vec3 aTangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 Color;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPosition, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(normalMatrix) * aNormal;
    TexCoord = aTexCoord;
    Color = aColor;
    
    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(mat3(normalMatrix) * aTangent);
    vec3 N = normalize(Normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);
    
    gl_Position = projection * view * worldPos;
}
";
        
        public static string LitFragmentShader = @"
#version 330 core

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    int type; // 0=Directional, 1=Point, 2=Spot
    float range;
    float spotAngle;
};

struct Material {
    vec4 albedo;
    float metallic;
    float roughness;
    float shininess;
    vec3 emission;
    
    sampler2D albedoMap;
    sampler2D normalMap;
    int hasAlbedoMap;
    int hasNormalMap;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 Color;
in mat3 TBN;

out vec4 FragColor;

uniform Material material;
uniform vec3 viewPos;

#define MAX_LIGHTS 8
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

vec3 calculateLight(Light light, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir;
    float attenuation = 1.0;
    
    if (light.type == 0) // Directional
    {
        lightDir = normalize(-light.direction);
    }
    else if (light.type == 1) // Point
    {
        lightDir = normalize(light.position - FragPos);
        float distance = length(light.position - FragPos);
        attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        attenuation *= clamp(1.0 - distance / light.range, 0.0, 1.0);
    }
    else // Spot
    {
        lightDir = normalize(light.position - FragPos);
        float distance = length(light.position - FragPos);
        float theta = dot(lightDir, normalize(-light.direction));
        float epsilon = cos(radians(light.spotAngle)) - cos(radians(light.spotAngle + 10.0));
        float spotIntensity = clamp((theta - cos(radians(light.spotAngle + 10.0))) / epsilon, 0.0, 1.0);
        
        attenuation = spotIntensity / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        attenuation *= clamp(1.0 - distance / light.range, 0.0, 1.0);
    }
    
    // Ambient
    vec3 ambient = 0.1 * albedo;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = vec3(spec) * (1.0 - material.roughness);
    
    return (ambient + diffuse + specular) * light.color * light.intensity * attenuation;
}

void main()
{
    // Get albedo
    vec3 albedo = material.albedo.rgb * Color.rgb;
    if (material.hasAlbedoMap == 1)
    {
        albedo *= texture(material.albedoMap, TexCoord).rgb;
    }
    
    // Get normal
    vec3 normal = normalize(Normal);
    if (material.hasNormalMap == 1)
    {
        normal = texture(material.normalMap, TexCoord).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }
    
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Calculate lighting
    vec3 result = material.emission;
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
    {
        result += calculateLight(lights[i], normal, viewDir, albedo);
    }
    
    FragColor = vec4(result, material.albedo.a * Color.a);
}
";

        // Keep old shader for compatibility
        public static string BasicVertexShader = LitVertexShader;
        public static string BasicFragmentShader = LitFragmentShader;
    }
}
