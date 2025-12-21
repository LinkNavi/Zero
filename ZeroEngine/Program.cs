using System;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.Common;
using OpenTK.Mathematics;
using EngineCore;
using EngineCore.ECS;
using EngineCore.Rendering;
using EngineCore.Components;

namespace EngineRuntime
{
    // ==================== TIME CLASS ====================
    
    public static class Time
    {
        public static float DeltaTime { get; internal set; }
        public static float TotalTime { get; internal set; }
    }
    
    // ==================== TEST SCRIPT ====================
    
    /// <summary>
    /// Simple script that rotates a GameObject
    /// </summary>
    public class RotatingCube : MonoBehaviour
    {
        public float rotationSpeed = 45f; // degrees per second
        private float _rotation = 0f;
        
        protected override void Start()
        {
            Console.WriteLine($"RotatingCube started on {gameObject.name}");
        }
        
        protected override void Update()
        {
            // Rotate around Y axis
            _rotation += rotationSpeed * Time.DeltaTime;
            
            // Simple rotation (we'll improve this later with proper quaternions)
            float radians = MathHelper.DegreesToRadians(_rotation);
            
            // Update position in a circle for visual effect
            transform.position = new EngineCore.ECS.Vector3(
                MathF.Sin(radians) * 2f,
                0,
                MathF.Cos(radians) * 2f
            );
        }
    }
    
    // ==================== GAME WINDOW ====================
    
    public class TestGameWindow : GameWindow
    {
        private Game _game;
        private Shader _defaultShader;
        private Mesh _cubeMesh;
        private Mesh _quadMesh;
        
        public TestGameWindow(GameWindowSettings gameWindowSettings, NativeWindowSettings nativeWindowSettings)
            : base(gameWindowSettings, nativeWindowSettings)
        {
        }
        
        protected override void OnLoad()
        {
            base.OnLoad();
            
            Console.WriteLine("Initializing Game Engine...");
            
            try
            {
                // Set OpenGL clear color
                GL.ClearColor(0.1f, 0.1f, 0.15f, 1.0f);
                
                // Create shader
                Console.WriteLine("Creating shader...");
                _defaultShader = new Shader(
                    DefaultShaders.BasicVertexShader,
                    DefaultShaders.BasicFragmentShader
                );
                
                // Create meshes
                Console.WriteLine("Creating meshes...");
                _cubeMesh = Mesh.CreateCube();
                _quadMesh = Mesh.CreateQuad();
                
                // Initialize game
                Console.WriteLine("Initializing game...");
                _game = new Game();
                _game.Initialize();
                
                // Set up test scene
                SetupTestScene();
                
                Console.WriteLine("Game engine initialized!");
                Console.WriteLine("Controls: ESC to exit");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR during initialization: {ex.Message}");
                Console.WriteLine($"Stack trace: {ex.StackTrace}");
                throw;
            }
        }
        
        private void SetupTestScene()
        {
            var scene = _game.Scene;
            
            // 1. Create Main Camera
            Console.WriteLine("Creating camera...");
            var cameraGO = scene.CreateGameObject("Main Camera");
            cameraGO.tag = "MainCamera";
            var camera = cameraGO.AddComponent<CameraComponent>();
            camera.fieldOfView = 60f;
            camera.backgroundColor = new Color4(0.2f, 0.3f, 0.4f, 1.0f);
            cameraGO.transform.position = new EngineCore.ECS.Vector3(0, 3, 8);
            
            // 2. Create a rotating cube
            Console.WriteLine("Creating rotating cube...");
            var cubeGO = scene.CreateGameObject("Rotating Cube");
            cubeGO.transform.position = new EngineCore.ECS.Vector3(0, 0, 0);
            
            var cubeRenderer = cubeGO.AddComponent<MeshRenderer>();
            cubeRenderer.mesh = _cubeMesh;
            cubeRenderer.material = new Material(_defaultShader)
            {
                Color = new Vector4(1.0f, 0.5f, 0.2f, 1.0f) // Orange
            };
            
            cubeGO.AddComponent<RotatingCube>();
            
            // 3. Create a static cube (ground)
            Console.WriteLine("Creating ground...");
            var groundGO = scene.CreateGameObject("Ground");
            groundGO.transform.position = new EngineCore.ECS.Vector3(0, -2, 0);
            groundGO.transform.localScale = new EngineCore.ECS.Vector3(10, 0.5f, 10);
            
            var groundRenderer = groundGO.AddComponent<MeshRenderer>();
            groundRenderer.mesh = _cubeMesh;
            groundRenderer.material = new Material(_defaultShader)
            {
                Color = new Vector4(0.3f, 0.7f, 0.3f, 1.0f) // Green
            };
            
            // 4. Create a few more cubes for testing
            for (int i = 0; i < 3; i++)
            {
                var testCube = scene.CreateGameObject($"Test Cube {i}");
                testCube.transform.position = new EngineCore.ECS.Vector3(i * 2 - 2, 0, -3);
                
                var testRenderer = testCube.AddComponent<MeshRenderer>();
                testRenderer.mesh = _cubeMesh;
                testRenderer.material = new Material(_defaultShader)
                {
                    Color = new Vector4(
                        0.2f + i * 0.3f,
                        0.3f,
                        0.8f - i * 0.2f,
                        1.0f
                    )
                };
            }
            
            // 5. Create a directional light
            Console.WriteLine("Creating light...");
            var lightGO = scene.CreateGameObject("Directional Light");
            var light = lightGO.AddComponent<Light>();
            light.type = Light.LightType.Directional;
            light.color = Color4.White;
            light.intensity = 1.0f;
            
            Console.WriteLine($"Scene created with {scene.GetAllGameObjects().Length} GameObjects");
        }
        
        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            base.OnUpdateFrame(args);
            
            // Update time
            Time.DeltaTime = (float)args.Time;
            Time.TotalTime += Time.DeltaTime;
            
            // Check for exit
            if (KeyboardState.IsKeyDown(OpenTK.Windowing.GraphicsLibraryFramework.Keys.Escape))
            {
                Close();
            }
            
            // Update game logic
            _game.Update(args.Time);
            _game.LateUpdate();
        }
        
        protected override void OnRenderFrame(FrameEventArgs args)
        {
            base.OnRenderFrame(args);
            
            try
            {
                // Render the scene
                _game.Render();
                
                // Swap buffers to display
                SwapBuffers();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR during render: {ex.Message}");
                Console.WriteLine($"Stack trace: {ex.StackTrace}");
            }
        }
        
        protected override void OnResize(ResizeEventArgs e)
        {
            base.OnResize(e);
            
            // Update OpenGL viewport
            GL.Viewport(0, 0, e.Width, e.Height);
            
            // Update camera aspect ratio
            _game.OnResize(e.Width, e.Height);
        }
        
        protected override void OnUnload()
        {
            base.OnUnload();
            
            // Cleanup
            Console.WriteLine("Cleaning up...");
            _defaultShader?.Dispose();
            _cubeMesh?.Dispose();
            _quadMesh?.Dispose();
        }
    }
    
    // ==================== PROGRAM ENTRY POINT ====================
    
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("=================================");
            Console.WriteLine("  Custom Game Engine - Test Demo");
            Console.WriteLine("=================================");
            Console.WriteLine();
            
            var gameWindowSettings = GameWindowSettings.Default;
            var nativeWindowSettings = new NativeWindowSettings()
            {
                Title = "Game Engine Test",
                ClientSize = new Vector2i(1280, 720),
                WindowBorder = WindowBorder.Fixed,
                StartVisible = true,
                StartFocused = true,
                API = ContextAPI.OpenGL,
                Profile = ContextProfile.Core,
                APIVersion = new Version(3, 3)
            };
            
            using (var game = new TestGameWindow(gameWindowSettings, nativeWindowSettings))
            {
                // Run at 60 FPS
                game.Run();
            }
            
            Console.WriteLine();
            Console.WriteLine("Game engine shut down successfully.");
        }
    }
}


