using System;
using System.Collections;
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

namespace EngineRuntime
{
    // ==================== EXAMPLE SCRIPTS ====================

    /// <summary>
    /// Rotating cube with input control
    /// </summary>
    public class PlayerController : MonoBehaviour
    {
        public float moveSpeed = 5f;
        public float rotationSpeed = 100f;
        public float jumpForce = 5f;
        
        private AudioSource _audioSource;
        private bool _isGrounded = true;

        protected override void Start()
        {
            Debug.Log($"PlayerController started on {gameObject.name}");
            
            // Get audio source if it exists
            _audioSource = GetComponent<AudioSource>();
        }

        protected override void Update()
        {
            // Movement input
            float horizontal = Input.GetAxisHorizontal();
            float vertical = Input.GetAxisVertical();
            
            if (Mathf.Abs(horizontal) > 0.1f || Mathf.Abs(vertical) > 0.1f)
            {
                Vector3 movement = new Vector3(horizontal, 0, vertical);
                transform.Translate(movement * moveSpeed * Time.DeltaTime, false);
            }
            
            // Rotation with Q/E
            if (Input.GetKey(KeyCode.Q))
            {
                transform.Rotate(0, -rotationSpeed * Time.DeltaTime, 0);
            }
            if (Input.GetKey(KeyCode.E))
            {
                transform.Rotate(0, rotationSpeed * Time.DeltaTime, 0);
            }
            
            // Jump with Space
            if (Input.GetKeyDown(KeyCode.Space) && _isGrounded)
            {
                Vector3 vel = transform.position;
                vel.y += jumpForce * Time.DeltaTime;
                transform.position = vel;
                _isGrounded = false;
                
                // Play jump sound
                _audioSource?.Play();
                
                Debug.Log("Jump!");
            }
            
            // Simple ground detection
            if (transform.position.y <= 0.5f)
            {
                Vector3 pos = transform.position;
                pos.y = 0.5f;
                transform.position = pos;
                _isGrounded = true;
            }
            
            // Draw debug ray showing forward direction
            Debug.DrawRay(transform.position, transform.forward * 2f, Color4.Green);
            
            // Debug box around player
            Debug.DrawWireBox(transform.position, new Vector3(1, 1, 1), Color4.Cyan);
        }
    }

    /// <summary>
    /// Object that orbits around a point using coroutines
    /// </summary>
    public class OrbitingObject : MonoBehaviour
    {
        public Vector3 centerPoint = Vector3.Zero;
        public float orbitRadius = 3f;
        public float orbitSpeed = 30f;
        
        private float _angle = 0f;

        protected override void Start()
        {
            StartCoroutine(BlinkCoroutine());
        }

        protected override void Update()
        {
            _angle += orbitSpeed * Time.DeltaTime;
            
            float x = centerPoint.x + Mathf.Cos(_angle * Mathf.Deg2Rad) * orbitRadius;
            float z = centerPoint.z + Mathf.Sin(_angle * Mathf.Deg2Rad) * orbitRadius;
            
            transform.position = new Vector3(x, centerPoint.y + 2f, z);
            
            // Look at center
            transform.LookAt(centerPoint);
            
            // Draw orbit path
            Vector3 nextPos = new Vector3(
                centerPoint.x + Mathf.Cos((_angle + 10f) * Mathf.Deg2Rad) * orbitRadius,
                centerPoint.y + 2f,
                centerPoint.z + Mathf.Sin((_angle + 10f) * Mathf.Deg2Rad) * orbitRadius
            );
            Debug.DrawLine(transform.position, nextPos, Color4.Yellow);
        }
        
        private IEnumerator BlinkCoroutine()
        {
            var renderer = GetComponent<MeshRenderer>();
            if (renderer == null) yield break;
            
            while (true)
            {
                // Fade out
                for (float t = 0; t < 1f; t += Time.DeltaTime * 2f)
                {
                    float alpha = Mathf.Lerp(1f, 0.2f, t);
                    renderer.material.Color = new Vector4(1, 1, 0, alpha);
                    yield return null;
                }
                
                // Fade in
                for (float t = 0; t < 1f; t += Time.DeltaTime * 2f)
                {
                    float alpha = Mathf.Lerp(0.2f, 1f, t);
                    renderer.material.Color = new Vector4(1, 1, 0, alpha);
                    yield return null;
                }
            }
        }
    }

    /// <summary>
    /// Camera controller with smooth follow
    /// </summary>
    public class CameraController : MonoBehaviour
    {
        public Transform target;
        public Vector3 offset = new Vector3(0, 5, 10);
        public float smoothSpeed = 5f;
        
        private float _currentVelocity = 0f;

        protected override void LateUpdate()
        {
            if (target == null) return;
            
            Vector3 desiredPosition = new Vector3(
                target.position.x + offset.x,
                target.position.y + offset.y,
                target.position.z + offset.z
            );
            
            // Smooth follow
            Vector3 currentPos = transform.position;
            transform.position = Vector3Extensions.Lerp(currentPos, desiredPosition, smoothSpeed * Time.DeltaTime);
            
            // Look at target
            transform.LookAt(target.position);
        }
    }

    // ==================== GAME WINDOW ====================

    public class EnhancedGameWindow : GameWindow
    {
        private Game _game;
        private Shader _defaultShader;
        private Mesh _cubeMesh;
        private AudioClip _jumpSound;

        public EnhancedGameWindow(GameWindowSettings gameWindowSettings, NativeWindowSettings nativeWindowSettings)
            : base(gameWindowSettings, nativeWindowSettings)
        {
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            Console.WriteLine("=== Game Engine Initialization ===");
            Console.WriteLine($"OpenGL Version: {GL.GetString(StringName.Version)}");
            Console.WriteLine($"GLSL Version: {GL.GetString(StringName.ShadingLanguageVersion)}");
            Console.WriteLine();

            // Enable depth testing
            GL.Enable(EnableCap.DepthTest);
            GL.DepthFunc(DepthFunction.Less);
            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);

            // Initialize systems
            Input.Initialize();
            AudioSystem.Initialize();
            Debug.Initialize();

            // Create resources
            GL.ClearColor(0.1f, 0.1f, 0.15f, 1.0f);
            _defaultShader = new Shader(DefaultShaders.BasicVertexShader, DefaultShaders.BasicFragmentShader);
            _cubeMesh = Mesh.CreateCube();

            // Load audio (you'll need to provide a WAV file)
            // _jumpSound = AudioSystem.LoadWav("Assets/jump.wav");

            // Initialize game
            _game = new Game();
            _game.Initialize();
            SetupScene();

            Console.WriteLine("=== Initialization Complete ===");
            Console.WriteLine("Controls:");
            Console.WriteLine("  WASD / Arrow Keys - Move");
            Console.WriteLine("  Q/E - Rotate");
            Console.WriteLine("  Space - Jump");
            Console.WriteLine("  ESC - Exit");
            Console.WriteLine();
        }

        private void SetupScene()
        {
            var scene = _game.Scene;

            // Camera with controller
            var cameraGO = scene.CreateGameObject("Main Camera");
            cameraGO.tag = "MainCamera";
            var camera = cameraGO.AddComponent<CameraComponent>();
            camera.fieldOfView = 60f;
            camera.backgroundColor = new Color4(0.2f, 0.3f, 0.4f, 1.0f);
            cameraGO.transform.position = new Vector3(0, 5, 10);
            
            var cameraController = cameraGO.AddComponent<CameraController>();
            cameraController.offset = new Vector3(0, 5, 10);

            // Player cube
            var playerGO = scene.CreateGameObject("Player");
            playerGO.transform.position = new Vector3(0, 0.5f, 0);
            
            var playerRenderer = playerGO.AddComponent<MeshRenderer>();
            playerRenderer.mesh = _cubeMesh;
            playerRenderer.material = new Material(_defaultShader)
            {
                Color = new Vector4(0.2f, 0.7f, 1.0f, 1.0f) // Blue
            };
            
            var playerController = playerGO.AddComponent<PlayerController>();
            
            // Add audio source to player
            var audioSource = playerGO.AddComponent<AudioSource>();
            audioSource.Clip = _jumpSound;
            audioSource.Spatial = false; // 2D sound
            
            // Set camera target
            cameraController.target = playerGO.transform;

            // Ground
            var groundGO = scene.CreateGameObject("Ground");
            groundGO.transform.position = new Vector3(0, -1, 0);
            groundGO.transform.localScale = new Vector3(20, 0.5f, 20);
            
            var groundRenderer = groundGO.AddComponent<MeshRenderer>();
            groundRenderer.mesh = _cubeMesh;
            groundRenderer.material = new Material(_defaultShader)
            {
                Color = new Vector4(0.3f, 0.5f, 0.3f, 1.0f)
            };

            // Orbiting objects
            for (int i = 0; i < 3; i++)
            {
                var orbitGO = scene.CreateGameObject($"Orbiter {i}");
                
                var orbitRenderer = orbitGO.AddComponent<MeshRenderer>();
                orbitRenderer.mesh = _cubeMesh;
                orbitRenderer.material = new Material(_defaultShader)
                {
                    Color = new Vector4(1, 1, 0, 1)
                };
                
                var orbiter = orbitGO.AddComponent<OrbitingObject>();
                orbiter.centerPoint = Vector3.Zero;
                orbiter.orbitRadius = 5f;
                orbiter.orbitSpeed = 30f + i * 15f;
            }

            // Directional light
            var lightGO = scene.CreateGameObject("Light");
            var light = lightGO.AddComponent<Light>();
            light.type = Light.LightType.Directional;
            light.intensity = 1.0f;
            lightGO.transform.SetEulerAngles(-45, 45, 0);

            Debug.Log($"Scene created with {scene.GetAllGameObjects().Length} GameObjects");
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            base.OnUpdateFrame(args);

            // Update Input
            Input.Update(KeyboardState, MouseState);

            // Update Time
            Time.DeltaTime = (float)args.Time;
            Time.TotalTime += Time.DeltaTime;

            // Update Debug
            Debug.Update(Time.DeltaTime);

            // Check for exit
            if (Input.GetKeyDown(KeyCode.Escape))
            {
                Close();
            }

            // Update game
            _game.Update(args.Time);
            _game.LateUpdate();

            // Update audio listener position
            var mainCam = CameraComponent.Main;
            if (mainCam != null)
            {
                AudioSystem.SetListenerPosition(mainCam.transform.position);
                var forward = mainCam.transform.forward;
                var up = mainCam.transform.up;
                AudioSystem.SetListenerOrientation(forward, up);
            }
        }

        protected override void OnRenderFrame(FrameEventArgs args)
        {
            base.OnRenderFrame(args);

            try
            {
                // Render scene
                _game.Render();

                // Render debug lines
                var mainCam = CameraComponent.Main;
                if (mainCam != null)
                {
                    Debug.Render(mainCam.GetCamera());
                }

                SwapBuffers();
            }
            catch (Exception ex)
            {
                Debug.LogError($"Render error: {ex.Message}");
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
            
            Debug.Log("Shutting down...");
            _defaultShader?.Dispose();
            _cubeMesh?.Dispose();
            _jumpSound?.Dispose();
            AudioSystem.Shutdown();
        }
    }

    // ==================== PROGRAM ENTRY POINT ====================

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("================================================");
            Console.WriteLine("  Enhanced Game Engine - Full Feature Demo");
            Console.WriteLine("================================================");
            Console.WriteLine();
            Console.WriteLine("Features:");
            Console.WriteLine("  ✓ Input System (Keyboard, Mouse, Gamepad)");
            Console.WriteLine("  ✓ Audio System (OpenAL 3D Audio)");
            Console.WriteLine("  ✓ Math Utilities (Lerp, Quaternions, etc.)");
            Console.WriteLine("  ✓ Debug Drawing (Lines, Rays, Boxes)");
            Console.WriteLine("  ✓ Transform Enhancements (Rotate, LookAt, etc.)");
            Console.WriteLine("  ✓ Coroutine System");
            Console.WriteLine();

            var gameWindowSettings = GameWindowSettings.Default;
            var nativeWindowSettings = new NativeWindowSettings()
            {
                Title = "Enhanced Game Engine - Full Demo",
                ClientSize = new Vector2i(1280, 720),
                WindowBorder = WindowBorder.Fixed,
                StartVisible = true,
                StartFocused = true,
                API = ContextAPI.OpenGL,
                Profile = ContextProfile.Core,
                APIVersion = new Version(3, 3)
            };

            try
            {
                using (var game = new EnhancedGameWindow(gameWindowSettings, nativeWindowSettings))
                {
                    game.Run();
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Fatal error: {ex}");
            }

            Console.WriteLine();
            Console.WriteLine("Engine shut down successfully.");
        }
    }
}
