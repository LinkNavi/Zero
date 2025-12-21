using System;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.Common;
using OpenTK.Mathematics;
using EngineCore;
using EngineCore.ECS;
using EngineCore.Rendering;
using EngineCore.Components;
using EngineCore.Audio;
using EngineCore.Input;
using EngineCore.Scripting;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineRuntime
{
    public class MagolorGameWindow : GameWindow
    {
        private Game _game;
        private Shader _shader;
        private Mesh _cubeMesh;
        private Mesh _sphereMesh;
        private Mesh _planeMesh;

        public MagolorGameWindow(GameWindowSettings gameWindowSettings, NativeWindowSettings nativeWindowSettings)
            : base(gameWindowSettings, nativeWindowSettings)
        {
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            Console.WriteLine("╔══════════════════════════════════════════════╗");
            Console.WriteLine("║       ZeroEngine - Magolor Scripting         ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝");
            Console.WriteLine($"OpenGL: {GL.GetString(StringName.Version)}");

            // OpenGL setup
            GL.Enable(EnableCap.DepthTest);
            GL.DepthFunc(DepthFunction.Less);
            GL.Enable(EnableCap.CullFace);
            GL.CullFace(CullFaceMode.Back);
            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

            // Initialize systems
            Input.Initialize();
            Debug.Initialize();

            try { AudioSystem.Initialize(); }
            catch (Exception ex) { Console.WriteLine($"Audio init failed: {ex.Message}"); }

            // Create improved shader
            _shader = new Shader(ImprovedShaders.VertexShader, ImprovedShaders.FragmentShader);
            
            // Create meshes
            _cubeMesh = Mesh.CreateCube();
            _sphereMesh = Mesh.CreateSphere();
            _planeMesh = Mesh.CreateQuad();

            // Initialize game
            _game = new Game();
            _game.Initialize();

            SetupScene();

            Console.WriteLine("\n╔══════════════════════════════════════════════╗");
            Console.WriteLine("║           Scene Setup Complete               ║");
            Console.WriteLine("╠══════════════════════════════════════════════╣");
            Console.WriteLine("║ Controls:                                    ║");
            Console.WriteLine("║   WASD     - Move player                     ║");
            Console.WriteLine("║   Space    - Jump                            ║");
            Console.WriteLine("║   ESC      - Exit                            ║");
            Console.WriteLine("╠══════════════════════════════════════════════╣");
            Console.WriteLine("║ Magolor Scripts Active:                      ║");
            Console.WriteLine("║   ✓ PlayerController.mg                      ║");
            Console.WriteLine("║   ✓ EnemyAI.mg                               ║");
            Console.WriteLine("║   ✓ FollowCamera.mg                          ║");
            Console.WriteLine("║   ✓ Collectible.mg                           ║");
            Console.WriteLine("║   ✓ Rotator.mg                               ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝");
        }

        private void SetupScene()
        {
            var scene = _game.Scene;

            // ==================== CAMERA ====================
            var cameraGO = scene.CreateGameObject("Main Camera");
            cameraGO.tag = "MainCamera";
            cameraGO.transform.position = new ECSVector3(0, 8, 12);
            cameraGO.transform.LookAt(new ECSVector3(0, 0, 0));
            
            var camera = cameraGO.AddComponent<CameraComponent>();
            camera.fieldOfView = 60f;
            camera.backgroundColor = new Color4(0.2f, 0.3f, 0.5f, 1.0f);

            // Add Magolor follow camera script
            var camScript = MagolorScriptComponent.CreateFromFile(cameraGO, "Scripts/FollowCamera.mg");

            Console.WriteLine("✓ Camera created with Magolor FollowCamera.mg");

            // ==================== GROUND ====================
            var ground = scene.CreateGameObject("Ground");
            ground.transform.position = new ECSVector3(0, -0.5f, 0);
            ground.transform.localScale = new ECSVector3(50, 1, 50);
            
            var groundRenderer = ground.AddComponent<MeshRenderer>();
            groundRenderer.mesh = _cubeMesh;
            groundRenderer.material = new Material(_shader)
            {
                Color = new Vector4(0.3f, 0.6f, 0.3f, 1.0f),
                Shininess = 16f,
                Roughness = 0.8f
            };

            Console.WriteLine("✓ Ground created");

            // ==================== DIRECTIONAL LIGHT ====================
            var lightGO = scene.CreateGameObject("Sun");
            lightGO.transform.SetEulerAngles(-45, 45, 0);
            
            var light = lightGO.AddComponent<Light>();
            light.type = Light.LightType.Directional;
            light.color = new Color4(1f, 0.95f, 0.8f, 1f);
            light.intensity = 1.2f;

            Console.WriteLine("✓ Light created");

            // ==================== PLAYER (with Magolor script) ====================
            var player = scene.CreateGameObject("Player");
            player.transform.position = new ECSVector3(0, 0.5f, 0);
            
            var playerRenderer = player.AddComponent<MeshRenderer>();
            playerRenderer.mesh = _cubeMesh;
            playerRenderer.material = new Material(_shader)
            {
                Color = new Vector4(0.2f, 0.6f, 1.0f, 1.0f),
                Shininess = 32f,
                Roughness = 0.3f
            };

            // Add optional Rigidbody for physics
            var rb = player.AddComponent<EngineCore.Physics.Rigidbody>();
            rb.useGravity = false; // We handle gravity in script
            rb.isKinematic = true;

            // Add Magolor PlayerController script
            var playerScript = MagolorScriptComponent.CreateFromFile(player, "Scripts/PlayerController.mg");

            Console.WriteLine("✓ Player created with Magolor PlayerController.mg");

            // Set camera to follow player
            // Note: In a real implementation, you'd need to pass the target reference to the script
            // For now, the script would need to find the player by name

            // ==================== ENEMY (with Magolor AI script) ====================
            var enemy = scene.CreateGameObject("Enemy");
            enemy.transform.position = new ECSVector3(8, 1f, 5);
            
            var enemyRenderer = enemy.AddComponent<MeshRenderer>();
            enemyRenderer.mesh = _cubeMesh;
            enemyRenderer.material = new Material(_shader)
            {
                Color = new Vector4(1.0f, 0.3f, 0.3f, 1.0f),
                Shininess = 32f,
                Roughness = 0.4f
            };

            // Add Magolor EnemyAI script
            var enemyScript = MagolorScriptComponent.CreateFromFile(enemy, "Scripts/EnemyAI.mg");

            Console.WriteLine("✓ Enemy created with Magolor EnemyAI.mg");

            // ==================== COLLECTIBLES ====================
            for (int i = 0; i < 5; i++)
            {
                float angle = (i / 5f) * MathF.PI * 2f;
                float radius = 8f;
                
                var collectible = scene.CreateGameObject($"Collectible_{i}");
                collectible.transform.position = new ECSVector3(
                    MathF.Cos(angle) * radius,
                    1f,
                    MathF.Sin(angle) * radius
                );
                collectible.transform.localScale = new ECSVector3(0.5f, 0.5f, 0.5f);
                
                var colRenderer = collectible.AddComponent<MeshRenderer>();
                colRenderer.mesh = _sphereMesh;
                colRenderer.material = new Material(_shader)
                {
                    Color = new Vector4(1f, 0.8f, 0.2f, 1f),
                    Emission = new OpenTK.Mathematics.Vector3(0.2f, 0.15f, 0.05f),
                    Shininess = 64f,
                    Roughness = 0.2f
                };

                // Add Magolor Collectible script
                var colScript = MagolorScriptComponent.CreateFromFile(collectible, "Scripts/Collectible.mg");
            }

            Console.WriteLine($"✓ 5 Collectibles created with Magolor Collectible.mg");

            // ==================== ROTATING CUBES ====================
            for (int i = 0; i < 3; i++)
            {
                var cube = scene.CreateGameObject($"RotatingCube_{i}");
                cube.transform.position = new ECSVector3(-5 + i * 5, 2f, -5);
                
                var cubeRenderer = cube.AddComponent<MeshRenderer>();
                cubeRenderer.mesh = _cubeMesh;
                cubeRenderer.material = new Material(_shader)
                {
                    Color = new Vector4(
                        0.5f + i * 0.2f,
                        0.3f,
                        1f - i * 0.2f,
                        1f
                    ),
                    Shininess = 32f
                };

                // Add Magolor Rotator script
                var rotScript = MagolorScriptComponent.CreateFromFile(cube, "Scripts/Rotator.mg");
            }

            Console.WriteLine($"✓ 3 Rotating cubes created with Magolor Rotator.mg");
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            base.OnUpdateFrame(args);

            Input.Update(KeyboardState, MouseState);
            Time.Update((float)args.Time);
            Debug.Update(Time.DeltaTime);

            if (Input.GetKeyDown(KeyCode.Escape))
                Close();

            _game.Update(args.Time);
            _game.LateUpdate();

            if (AudioSystem.IsInitialized)
            {
                var mainCam = CameraComponent.Main;
                if (mainCam != null)
                {
                    AudioSystem.SetListenerPosition(mainCam.transform.position);
                    AudioSystem.SetListenerOrientation(mainCam.transform.forward, mainCam.transform.up);
                }
            }
        }

        protected override void OnRenderFrame(FrameEventArgs args)
        {
            base.OnRenderFrame(args);

            try
            {
                _game.Render();

                var mainCam = CameraComponent.Main;
                if (mainCam != null)
                    Debug.Render(mainCam.GetCamera());

                Title = $"ZeroEngine - Magolor Scripting | FPS: {1.0 / args.Time:F0}";

                SwapBuffers();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Render error: {ex.Message}");
            }
        }

        protected override void OnResize(ResizeEventArgs e)
        {
            base.OnResize(e);
            GL.Viewport(0, 0, e.Width, e.Height);
            _game.OnResize(e.Width, e.Height);
        }

        protected override void OnUnload()
        {
            base.OnUnload();
            _shader?.Dispose();
            _cubeMesh?.Dispose();
            _sphereMesh?.Dispose();
            _planeMesh?.Dispose();
            AudioSystem.Shutdown();
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("╔══════════════════════════════════════════════╗");
            Console.WriteLine("║       ZeroEngine - Magolor Scripting         ║");
            Console.WriteLine("╠══════════════════════════════════════════════╣");
            Console.WriteLine("║  Unity-Style Class-Based Scripts             ║");
            Console.WriteLine("║  Powered by Magolor Language                 ║");
            Console.WriteLine("╠══════════════════════════════════════════════╣");
            Console.WriteLine("║  Script Template:                            ║");
            Console.WriteLine("║                                              ║");
            Console.WriteLine("║    class MyScript {                          ║");
            Console.WriteLine("║        void fn OnStart() { }                 ║");
            Console.WriteLine("║        void fn OnUpdate() { }                ║");
            Console.WriteLine("║        void fn OnDestroy() { }               ║");
            Console.WriteLine("║    }                                         ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝\n");

            var gameSettings = GameWindowSettings.Default;
            var nativeSettings = new NativeWindowSettings()
            {
                Title = "ZeroEngine - Magolor Scripting",
                ClientSize = new Vector2i(1280, 720),
                WindowBorder = WindowBorder.Fixed,
                API = ContextAPI.OpenGL,
                Profile = ContextProfile.Core,
                APIVersion = new Version(3, 3)
            };

            try
            {
                using var game = new MagolorGameWindow(gameSettings, nativeSettings);
                game.Run();
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"\n╔══════════════════════════════════════════════╗");
                Console.WriteLine($"║              FATAL ERROR                     ║");
                Console.WriteLine($"╚══════════════════════════════════════════════╝");
                Console.WriteLine($"\n{ex}");
                Console.ResetColor();
            }
        }
    }
}
