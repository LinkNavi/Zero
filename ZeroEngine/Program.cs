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
    public class FixedGameWindow : GameWindow
    {
        private Game _game;
        private Shader _shader;
        private Mesh _cubeMesh;
        private Mesh _sphereMesh;
        private Mesh _planeMesh;

        public FixedGameWindow(GameWindowSettings gameWindowSettings, NativeWindowSettings nativeWindowSettings)
            : base(gameWindowSettings, nativeWindowSettings)
        {
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            Console.WriteLine("=== ZeroEngine - Class-Based Scripts ===");
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

            Console.WriteLine("\n=== Scene Setup Complete ===");
            Console.WriteLine("Controls:");
            Console.WriteLine("  WASD - Move player");
            Console.WriteLine("  Space - Jump");
            Console.WriteLine("  ESC - Exit");
        }

        private void SetupScene()
        {
            var scene = _game.Scene;

            // ==================== CAMERA ====================
            var cameraGO = scene.CreateGameObject("Main Camera");
            cameraGO.tag = "MainCamera";
            cameraGO.transform.position = new ECSVector3(0, 8, 12);
            cameraGO.transform.LookAt(new ECSVector3(0, 0, 0)); // FIXED: Proper look-at
            
            var camera = cameraGO.AddComponent<CameraComponent>();
            camera.fieldOfView = 60f;
            camera.backgroundColor = new Color4(0.2f, 0.3f, 0.5f, 1.0f);

            // Add follow script to camera
            var camScript = cameraGO.AddComponent<ScriptComponent>();
            camScript.SetScriptType<FollowCamera>();
            // We'll set the target after creating the player

            Console.WriteLine("✓ Camera created");

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

            // ==================== PLAYER (with script) ====================
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

            // Add PlayerController script
            var playerScript = player.AddComponent<ScriptComponent>();
            playerScript.SetScriptType<PlayerController>();
            var controller = playerScript.GetScript<PlayerController>();
            controller.moveSpeed = 5f;
            controller.jumpForce = 8f;

            Console.WriteLine("✓ Player created with PlayerController script");

            // Set camera to follow player
            var followCam = camScript.GetScript<FollowCamera>();
            followCam.target = player.transform;
            followCam.offset = new ECSVector3(0, 8, 12);
            followCam.smoothSpeed = 5f;

            // ==================== ENEMY (with AI script) ====================
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

            // Add EnemyAI script
            var enemyScript = enemy.AddComponent<ScriptComponent>();
            enemyScript.SetScriptType<EnemyAI>();
            var ai = enemyScript.GetScript<EnemyAI>();
            ai.target = player.transform;
            ai.detectRange = 10f;
            ai.attackRange = 3f;
            ai.moveSpeed = 3f;

            Console.WriteLine("✓ Enemy created with EnemyAI script");

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

                // Add Collectible script
                var colScript = collectible.AddComponent<ScriptComponent>();
                colScript.SetScriptType<Collectible>();
            }

            Console.WriteLine("✓ Collectibles created");

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

                // Add Rotator script
                var rotScript = cube.AddComponent<ScriptComponent>();
                rotScript.SetScriptType<Rotator>();
                var rotator = rotScript.GetScript<Rotator>();
                rotator.rotationSpeed = 45f + i * 30f;
                rotator.axis = new ECSVector3(0, 1, i * 0.3f);
            }

            Console.WriteLine("✓ Rotating cubes created");
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

                Title = $"ZeroEngine | FPS: {1.0 / args.Time:F0}";

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
            Console.WriteLine("║       ZeroEngine - Class-Based Scripts       ║");
            Console.WriteLine("╠══════════════════════════════════════════════╣");
            Console.WriteLine("║  Features:                                   ║");
            Console.WriteLine("║  ✓ Unity/PlayCanvas Style Scripts            ║");
            Console.WriteLine("║  ✓ Inherit from ScriptBase                   ║");
            Console.WriteLine("║  ✓ OnStart/OnUpdate/OnDestroy                ║");
            Console.WriteLine("║  ✓ Fixed Camera System                       ║");
            Console.WriteLine("║  ✓ Improved Lighting                         ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝");

            var gameSettings = GameWindowSettings.Default;
            var nativeSettings = new NativeWindowSettings()
            {
                Title = "ZeroEngine - Fixed",
                ClientSize = new Vector2i(1280, 720),
                WindowBorder = WindowBorder.Fixed,
                API = ContextAPI.OpenGL,
                Profile = ContextProfile.Core,
                APIVersion = new Version(3, 3)
            };

            try
            {
                using var game = new FixedGameWindow(gameSettings, nativeSettings);
                game.Run();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Fatal error: {ex}");
            }
        }
    }
}
