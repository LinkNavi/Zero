using System;
using System.Collections;
using System.IO;
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
    // ==================== SCRIPT-DRIVEN PLAYER ====================
    
    /// <summary>
    /// Example class that exposes properties to scripts via attributes
    /// </summary>
    public class PlayerStats
    {
        [ScriptVariable("health")]
        public float Health = 100f;
        
        [ScriptVariable("max_health", readOnly: true)]
        public float MaxHealth = 100f;
        
        [ScriptVariable("speed")]
        public float MoveSpeed = 5f;
        
        [ScriptVariable("jump_force")]
        public float JumpForce = 8f;
        
        [ScriptVariable("is_grounded")]
        public bool IsGrounded = true;
        
        [ScriptCallable("take_damage")]
        public float TakeDamage(float amount)
        {
            Health = Math.Max(0, Health - amount);
            Console.WriteLine($"Player took {amount} damage! Health: {Health}");
            return Health;
        }
        
        [ScriptCallable("heal")]
        public float Heal(float amount)
        {
            Health = Math.Min(MaxHealth, Health + amount);
            return Health;
        }
        
        [ScriptCallable("is_alive")]
        public bool IsAlive() => Health > 0;
    }

    /// <summary>
    /// Camera controller with smooth follow (C# implementation)
    /// </summary>
    public class CameraController : MonoBehaviour
    {
        public Transform target;
        public ECSVector3 offset = new ECSVector3(0, 5, 10);
        public float smoothSpeed = 5f;

        protected override void LateUpdate()
        {
            if (target == null) return;
            
            ECSVector3 desiredPosition = new ECSVector3(
                target.position.x + offset.x,
                target.position.y + offset.y,
                target.position.z + offset.z
            );
            
            ECSVector3 currentPos = transform.position;
            transform.position = Vector3Extensions.Lerp(currentPos, desiredPosition, smoothSpeed * Time.DeltaTime);
            transform.LookAt(target.position);
        }
    }

    // ==================== GAME WINDOW ====================

    public class ScriptedGameWindow : GameWindow
    {
        private Game _game;
        private Shader _defaultShader;
        private Mesh _cubeMesh;
        
        // Script-related
        private PlayerStats _playerStats;
        private ScriptComponent _playerScript;
        private ScriptComponent _enemyScript;
        private GameObject _player;
        private GameObject _enemy;

        public ScriptedGameWindow(GameWindowSettings gameWindowSettings, NativeWindowSettings nativeWindowSettings)
            : base(gameWindowSettings, nativeWindowSettings)
        {
        }

        protected override void OnLoad()
        {
            base.OnLoad();

            Console.WriteLine("=== ZeroEngine with Magolor Scripting ===");
            Console.WriteLine($"OpenGL: {GL.GetString(StringName.Version)}");
            Console.WriteLine();

            // OpenGL setup
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

            // Initialize game
            _game = new Game();
            _game.Initialize();
            
            // Create scripts directory if needed
            Directory.CreateDirectory("Scripts");
            CreateExampleScripts();
            
            SetupScene();

            Console.WriteLine("\n=== Initialization Complete ===");
            Console.WriteLine("Controls:");
            Console.WriteLine("  WASD - Move (script-controlled)");
            Console.WriteLine("  Space - Jump");
            Console.WriteLine("  H - Heal player");
            Console.WriteLine("  K - Damage player");
            Console.WriteLine("  R - Reload scripts");
            Console.WriteLine("  ESC - Exit");
            Console.WriteLine();
        }

        private void CreateExampleScripts()
        {
            // Player controller script
            string playerScript = @"// player_controller.mg - Player movement script

void fn on_start() {
    log(""Player script started!"");
}

void fn on_update() {
    let f32 dt = delta_time;
    let f32 spd = speed;
    
    // Get input (set by engine)
    let f32 h = input_h;
    let f32 v = input_v;
    
    // Move
    if (abs(h) > 0.1 || abs(v) > 0.1) {
        pos_x = pos_x + h * spd * dt;
        pos_z = pos_z + v * spd * dt;
    }
    
    // Jump
    if (input_jump > 0.5 && is_grounded > 0.5) {
        velocity_y = jump_force;
        is_grounded = 0.0;
    }
    
    // Apply gravity
    if (is_grounded < 0.5) {
        velocity_y = velocity_y - 20.0 * dt;
        pos_y = pos_y + velocity_y * dt;
    }
    
    // Ground check
    if (pos_y <= 0.5) {
        pos_y = 0.5;
        velocity_y = 0.0;
        is_grounded = 1.0;
    }
    
    // Rotate based on movement
    if (abs(h) > 0.1 || abs(v) > 0.1) {
        let f32 target_rot = atan2(h, v) * 57.2958;
        rotate(0.0, target_rot * dt * 5.0, 0.0);
    }
}

void fn on_destroy() {
    log(""Player script destroyed"");
}
";

            // Enemy AI script
            string enemyScript = @"// enemy_ai.mg - Simple enemy AI

let f32 state = 0.0;  // 0=idle, 1=chase, 2=attack
let f32 timer = 0.0;
let f32 attack_range = 3.0;
let f32 detect_range = 8.0;
let f32 move_speed = 2.5;

void fn on_start() {
    log(""Enemy AI started!"");
}

void fn on_update() {
    let f32 dt = delta_time;
    timer = timer + dt;
    
    // Calculate distance to player
    let f32 dx = player_x - pos_x;
    let f32 dz = player_z - pos_z;
    let f32 dist = sqrt(dx * dx + dz * dz);
    
    // State machine
    if (state < 0.5) {
        // Idle - bob up and down
        pos_y = 1.0 + sin(total_time * 2.0) * 0.2;
        
        // Check for player
        if (dist < detect_range) {
            state = 1.0;
            log(""Enemy detected player!"");
        }
    } elif (state < 1.5) {
        // Chase
        if (dist > 0.1) {
            let f32 nx = dx / dist;
            let f32 nz = dz / dist;
            pos_x = pos_x + nx * move_speed * dt;
            pos_z = pos_z + nz * move_speed * dt;
        }
        
        // Attack if close
        if (dist < attack_range) {
            state = 2.0;
            timer = 0.0;
        }
        
        // Lose interest if too far
        if (dist > detect_range * 1.5) {
            state = 0.0;
            log(""Enemy lost player"");
        }
    } else {
        // Attack
        if (timer > 1.0) {
            log(""Enemy attacks!"");
            // Could call damage function here
            timer = 0.0;
        }
        
        // Chase if player moved away
        if (dist > attack_range * 1.2) {
            state = 1.0;
        }
    }
    
    // Visual feedback - change color based on state (via scale hack)
    scale_y = 1.0 + state * 0.2;
}

void fn on_destroy() {
    log(""Enemy destroyed"");
}
";

            File.WriteAllText("Scripts/player_controller.mg", playerScript);
            File.WriteAllText("Scripts/enemy_ai.mg", enemyScript);
            Console.WriteLine("Created example scripts in Scripts/");
        }

        private void SetupScene()
        {
            var scene = _game.Scene;

            // Camera
            var cameraGO = scene.CreateGameObject("Main Camera");
            cameraGO.tag = "MainCamera";
            var camera = cameraGO.AddComponent<CameraComponent>();
            camera.fieldOfView = 60f;
            camera.backgroundColor = new Color4(0.2f, 0.3f, 0.4f, 1.0f);
            cameraGO.transform.position = new ECSVector3(0, 8, 12);
            
            var camController = cameraGO.AddComponent<CameraController>();
            camController.offset = new ECSVector3(0, 8, 12);

            // Ground
            var ground = scene.CreateGameObject("Ground");
            ground.transform.position = new ECSVector3(0, -0.5f, 0);
            ground.transform.localScale = new ECSVector3(30, 1, 30);
            var groundRenderer = ground.AddComponent<MeshRenderer>();
            groundRenderer.mesh = _cubeMesh;
            groundRenderer.material = new Material(_defaultShader) { Color = new Vector4(0.3f, 0.5f, 0.3f, 1.0f) };

            // Player with script
            _player = scene.CreateGameObject("Player");
            _player.transform.position = new ECSVector3(0, 0.5f, 0);
            
            var playerRenderer = _player.AddComponent<MeshRenderer>();
            playerRenderer.mesh = _cubeMesh;
            playerRenderer.material = new Material(_defaultShader) { Color = new Vector4(0.2f, 0.6f, 1.0f, 1.0f) };
            
            // Setup player script with custom bindings
            _playerStats = new PlayerStats();
            _playerScript = _player.AddComponent<ScriptComponent>();
            _playerScript.ScriptPath = "Scripts/player_controller.mg";
            
            _playerScript.Configure(iface =>
            {
                // Bind PlayerStats object (auto-binds all [ScriptVariable] and [ScriptCallable])
                iface.BindObject("", _playerStats);
                
                // Custom input variables
                float velocityY = 0;
                iface.RegisterVariable("velocity_y", () => velocityY, v => velocityY = (float)v);
                iface.RegisterVariable("input_h", () => Input.GetAxisHorizontal());
                iface.RegisterVariable("input_v", () => Input.GetAxisVertical());
                iface.RegisterVariable("input_jump", () => Input.GetKeyDown(KeyCode.Space) ? 1.0 : 0.0);
                
                // Log callback
                iface.OnLog = msg => Debug.Log($"[Player] {msg}");
            });
            
            camController.target = _player.transform;

            // Enemy with script
            _enemy = scene.CreateGameObject("Enemy");
            _enemy.transform.position = new ECSVector3(5, 1, 5);
            
            var enemyRenderer = _enemy.AddComponent<MeshRenderer>();
            enemyRenderer.mesh = _cubeMesh;
            enemyRenderer.material = new Material(_defaultShader) { Color = new Vector4(1.0f, 0.3f, 0.3f, 1.0f) };
            
            _enemyScript = _enemy.AddComponent<ScriptComponent>();
            _enemyScript.ScriptPath = "Scripts/enemy_ai.mg";
            
            _enemyScript.Configure(iface =>
            {
                // Give enemy access to player position
                iface.RegisterVariable("player_x", () => _player.transform.position.x);
                iface.RegisterVariable("player_y", () => _player.transform.position.y);
                iface.RegisterVariable("player_z", () => _player.transform.position.z);
                
                // Enemy can damage player
                iface.RegisterFunction("damage_player", args =>
                {
                    _playerStats.TakeDamage((float)args[0]);
                    return _playerStats.Health;
                });
                
                iface.OnLog = msg => Debug.Log($"[Enemy] {msg}");
            });

            // Some decoration cubes
            for (int i = 0; i < 5; i++)
            {
                var deco = scene.CreateGameObject($"Deco_{i}");
                float angle = i * 72 * Mathf.Deg2Rad;
                deco.transform.position = new ECSVector3(
                    Mathf.Cos(angle) * 10,
                    0.5f,
                    Mathf.Sin(angle) * 10
                );
                var decoRenderer = deco.AddComponent<MeshRenderer>();
                decoRenderer.mesh = _cubeMesh;
                decoRenderer.material = new Material(_defaultShader) 
                { 
                    Color = new Vector4(0.5f + i * 0.1f, 0.5f, 0.8f - i * 0.1f, 1.0f) 
                };
            }

            // Light
            var light = scene.CreateGameObject("Light");
            var lightComp = light.AddComponent<Light>();
            lightComp.type = Light.LightType.Directional;
            lightComp.intensity = 1.0f;
            light.transform.SetEulerAngles(-45, 45, 0);

            Debug.Log($"Scene created with {scene.GetAllGameObjects().Length} GameObjects");
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            base.OnUpdateFrame(args);

            Input.Update(KeyboardState, MouseState);
            Time.Update((float)args.Time);
            Debug.Update(Time.DeltaTime);

            // Exit
            if (Input.GetKeyDown(KeyCode.Escape))
                Close();

            // Debug keys
            if (Input.GetKeyDown(KeyCode.H))
            {
                _playerStats.Heal(20);
                Debug.Log($"Healed! Health: {_playerStats.Health}");
            }
            
            if (Input.GetKeyDown(KeyCode.K))
            {
                _playerStats.TakeDamage(10);
            }
            
            if (Input.GetKeyDown(KeyCode.R))
            {
                ReloadScripts();
            }

            // Update game
            _game.Update(args.Time);
            _game.LateUpdate();

            // Update audio listener
            var mainCam = CameraComponent.Main;
            if (mainCam != null)
            {
                AudioSystem.SetListenerPosition(mainCam.transform.position);
                AudioSystem.SetListenerOrientation(mainCam.transform.forward, mainCam.transform.up);
            }
            
            // Debug visualization
            Debug.DrawWireBox(_player.transform.position, new ECSVector3(1, 1, 1), Color4.Cyan);
            Debug.DrawWireBox(_enemy.transform.position, new ECSVector3(1, 1, 1), Color4.Red);
            Debug.DrawLine(_enemy.transform.position, _player.transform.position, Color4.Yellow);
        }

        private void ReloadScripts()
        {
            Debug.Log("Reloading scripts...");
            
            // In a full implementation, you'd reload the ScriptComponent
            // For now just log
            Debug.Log("Scripts reloaded (hot-reload not fully implemented)");
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

                // Simple UI overlay
                Title = $"ZeroEngine | FPS: {1.0 / args.Time:F0} | Health: {_playerStats.Health:F0}";

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
            AudioSystem.Shutdown();
        }
    }

    // ==================== ENTRY POINT ====================

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("╔══════════════════════════════════════════════╗");
            Console.WriteLine("║  ZeroEngine with Magolor Scripting Support   ║");
            Console.WriteLine("╠══════════════════════════════════════════════╣");
            Console.WriteLine("║  Features:                                   ║");
            Console.WriteLine("║  ✓ Magolor Scripting Language                ║");
            Console.WriteLine("║  ✓ Script-to-C# Variable Binding             ║");
            Console.WriteLine("║  ✓ Attribute-based Function Export           ║");
            Console.WriteLine("║  ✓ Hot Variables (delta_time, pos_x, etc)    ║");
            Console.WriteLine("║  ✓ Custom Native Functions                   ║");
            Console.WriteLine("╚══════════════════════════════════════════════╝");
            Console.WriteLine();

            var gameSettings = GameWindowSettings.Default;
            var nativeSettings = new NativeWindowSettings()
            {
                Title = "ZeroEngine - Scripted",
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
                using var game = new ScriptedGameWindow(gameSettings, nativeSettings);
                game.Run();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Fatal error: {ex}");
            }

            Console.WriteLine("\nEngine shut down.");
        }
    }
}
