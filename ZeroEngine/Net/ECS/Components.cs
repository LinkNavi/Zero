using System;
using System.Collections.Generic;
using System.Linq;
using OpenTK.Mathematics;
using OpenTK.Graphics.OpenGL4;
using EngineCore.ECS;
using EngineCore.Rendering;

namespace EngineCore.Components
{
    // ==================== CAMERA COMPONENT ====================

    /// <summary>
    /// Camera component - Unity style
    /// Attach this to a GameObject to make it a camera
    /// </summary>
    public class CameraComponent : Component
    {
        private Camera _camera;

        // Camera settings
        public float fieldOfView = 60f;
        public float nearClipPlane = 0.1f;
        public float farClipPlane = 1000f;
        public Color4 backgroundColor = new Color4(0.2f, 0.3f, 0.4f, 1.0f);

        // Projection type
        public enum ProjectionType { Perspective, Orthographic }
        public ProjectionType projectionType = ProjectionType.Perspective;

        // Orthographic settings
        public float orthographicSize = 5f;

        // Render order (lower renders first)
        public int depth = 0;

        public CameraComponent()
        {
            _camera = new Camera();
        }

        /// <summary>
        /// Get the internal Camera object for rendering
        /// </summary>
        public Camera GetCamera()
        {
            UpdateCameraFromTransform();
            return _camera;
        }

        /// <summary>
        /// Sync the camera with the GameObject's transform
        /// </summary>

        private void UpdateCameraFromTransform()
        {
            // Use the GameObject's transform position
            _camera.Position = new OpenTK.Mathematics.Vector3(
                transform.position.x,
                transform.position.y,
                transform.position.z
            );

            // FIX: Make camera look toward negative Z (standard OpenGL convention)
            // When camera is at (0, 2, 5), it should look at (0, 2, 0) or lower Z values
            _camera.Target = _camera.Position + new OpenTK.Mathematics.Vector3(0, 0, -1);

            // Standard up vector
            _camera.Up = new OpenTK.Mathematics.Vector3(0, 1, 0);

            // Update camera settings
            _camera.FieldOfView = MathHelper.DegreesToRadians(fieldOfView);
            _camera.NearClip = nearClipPlane;
            _camera.FarClip = farClipPlane;
        }


        /// <summary>
        /// Find the main camera in the scene (tagged "MainCamera")
        /// </summary>
        public static CameraComponent Main
        {
            get
            {
                var mainCameraGO = GameObject.FindWithTag("MainCamera");
                return mainCameraGO?.GetComponent<CameraComponent>();
            }
        }
    }

    // ==================== MESH RENDERER COMPONENT ====================

    /// <summary>
    /// MeshRenderer component - makes a GameObject visible
    /// Holds a reference to a Mesh and Material
    /// </summary>
    public class MeshRenderer : Component
    {
        public Mesh mesh;
        public Material material;

        // Should this renderer cast shadows?
        public bool castShadows = true;

        // Should this renderer receive shadows?
        public bool receiveShadows = true;

        // Rendering layer (for selective rendering)
        public int renderLayer = 0;

        public MeshRenderer()
        {
        }

        public MeshRenderer(Mesh mesh, Material material)
        {
            this.mesh = mesh;
            this.material = material;
        }

        /// <summary>
        /// Get the model matrix for this renderer (from Transform)
        /// </summary>
        public Matrix4 GetModelMatrix()
        {
            var t = transform;

            // Build transformation matrix: Translation * Rotation * Scale
            var translation = Matrix4.CreateTranslation(t.position.x, t.position.y, t.position.z);

            // Convert quaternion to rotation matrix (proper implementation)
            var q = t.localRotation;
            float xx = q.x * q.x;
            float yy = q.y * q.y;
            float zz = q.z * q.z;
            float xy = q.x * q.y;
            float xz = q.x * q.z;
            float yz = q.y * q.z;
            float wx = q.w * q.x;
            float wy = q.w * q.y;
            float wz = q.w * q.z;

            var rotation = new Matrix4(
                1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0,
                2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx), 0,
                2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy), 0,
                0, 0, 0, 1
            );

            var scale = Matrix4.CreateScale(t.localScale.x, t.localScale.y, t.localScale.z);

            return scale * rotation * translation;
        }
    }

    // ==================== LIGHT COMPONENT ====================

    /// <summary>
    /// Light component - defines a light source
    /// </summary>
    public class Light : Component
    {
        public enum LightType { Directional, Point, Spot }

        public LightType type = LightType.Directional;
        public Color4 color = Color4.White;
        public float intensity = 1.0f;

        // Point/Spot light settings
        public float range = 10f;

        // Spot light settings
        public float spotAngle = 45f;

        /// <summary>
        /// Get the main directional light in the scene
        /// </summary>
        public static Light MainDirectionalLight
        {
            get
            {
                // Find first directional light
                var scene = Scene.Active;
                if (scene == null) return null;

                foreach (var go in scene.GetAllGameObjects())
                {
                    var light = go.GetComponent<Light>();
                    if (light != null && light.type == LightType.Directional && light.enabled)
                        return light;
                }
                return null;
            }
        }
    }

    // ==================== RENDERING SYSTEM ====================

    /// <summary>
    /// RenderingSystem - manages the rendering pipeline
    /// Call Render() every frame to draw everything
    /// </summary>
    public class RenderingSystem
    {
        private Renderer _renderer;
        private List<(MeshRenderer, Matrix4)> _renderQueue = new List<(MeshRenderer, Matrix4)>();

        public RenderingSystem()
        {
            // We'll set the camera later
            _renderer = new Renderer(new Camera());
        }

        /// <summary>
        /// Main render function - call this every frame
        /// </summary>
        public void Render(Scene scene)
        {
            // Step 1: Find the active camera
            CameraComponent cameraComponent = CameraComponent.Main;
            if (cameraComponent == null || !cameraComponent.enabled)
            {
                Console.WriteLine("WARNING: No active camera found!");
                return;
            }

            Camera camera = cameraComponent.GetCamera();
            _renderer.SetCamera(camera);

            // Step 2: Clear the screen
            var bgColor = cameraComponent.backgroundColor;
            _renderer.Clear(new Vector4(bgColor.R, bgColor.G, bgColor.B, bgColor.A));

            // Step 3: Build render queue
            _renderQueue.Clear();
            int activeRenderers = 0;

            foreach (var gameObject in scene.GetAllGameObjects())
            {
                if (!gameObject.activeInHierarchy)
                    continue;

                var meshRenderer = gameObject.GetComponent<MeshRenderer>();
                if (meshRenderer != null && meshRenderer.enabled &&
                    meshRenderer.mesh != null && meshRenderer.material != null)
                {
                    var modelMatrix = meshRenderer.GetModelMatrix();
                    _renderQueue.Add((meshRenderer, modelMatrix));
                    activeRenderers++;
                }
            }

            // Debug output (only print once every 60 frames to avoid spam)
            int frameCount = 0;
            if (frameCount++ % 60 == 0)
            {

            }

            // Step 5: Render all objects
            foreach (var (meshRenderer, modelMatrix) in _renderQueue)
            {
                _renderer.DrawMesh(meshRenderer.mesh, meshRenderer.material, modelMatrix);
            }
        }

        /// <summary>
        /// Update aspect ratio when window resizes
        /// </summary>
        public void UpdateAspectRatio(float width, float height)
        {
            var mainCamera = CameraComponent.Main;
            if (mainCamera != null)
            {
                mainCamera.GetCamera().AspectRatio = width / height;
            }
        }
    }

    // ==================== MESH FILTER (Optional - Unity has this separate) ====================

    /// <summary>
    /// MeshFilter - holds just the mesh data
    /// Unity separates MeshFilter (mesh data) and MeshRenderer (rendering)
    /// This is optional - you can just use MeshRenderer directly
    /// </summary>
    public class MeshFilter : Component
    {
        public Mesh mesh;

        public MeshFilter()
        {
        }

        public MeshFilter(Mesh mesh)
        {
            this.mesh = mesh;
        }
    }
}

// ==================== GAME LOOP INTEGRATION ====================

namespace EngineCore
{
    using EngineCore.Components;

    /// <summary>
    /// Main game class - integrates ECS with rendering
    /// This is what your Runtime or Editor would use
    /// </summary>
    public class Game
    {
        private Scene _scene;
        private Components.RenderingSystem _renderingSystem;

        // Timing
        private double _lastUpdateTime;
        private double _deltaTime;

        public Game()
        {
            _renderingSystem = new RenderingSystem();
        }

        /// <summary>
        /// Initialize the game
        /// </summary>
        public void Initialize()
        {
            // Create and set up the scene
            _scene = new Scene("MainScene");
            _scene.SetActive();

            // Example: Create a camera
            var cameraGO = _scene.CreateGameObject("Main Camera");
            cameraGO.tag = "MainCamera";
            var camera = cameraGO.AddComponent<CameraComponent>();
            camera.backgroundColor = new Color4(0.1f, 0.1f, 0.15f, 1.0f);
            cameraGO.transform.position = new ECS.Vector3(0, 2, 5);

            // Example: Create a cube
            var cubeGO = _scene.CreateGameObject("Cube");
            var meshRenderer = cubeGO.AddComponent<MeshRenderer>();

            // Set up the mesh and material
            // (You'd create these from your rendering system)
            // meshRenderer.mesh = Mesh.CreateCube();
            // meshRenderer.material = new Material(yourShader);

            // Example: Create a light
            var lightGO = _scene.CreateGameObject("Directional Light");
            var light = lightGO.AddComponent<Light>();
            light.type = Light.LightType.Directional;
            light.color = Color4.White;
            light.intensity = 1.0f;
            lightGO.transform.localRotation = ECS.Quaternion.Identity; // Point down
        }

        /// <summary>
        /// Update game logic (call every frame)
        /// </summary>
        public void Update(double currentTime)
        {
            _deltaTime = currentTime - _lastUpdateTime;
            _lastUpdateTime = currentTime;

            // Update ECS (runs Update() on all MonoBehaviours)
            _scene?.Update();
        }

        /// <summary>
        /// Late update (after Update, before Render)
        /// </summary>
        public void LateUpdate()
        {
            _scene?.LateUpdate();
        }

        /// <summary>
        /// Render the scene (call after Update)
        /// </summary>
        public void Render()
        {
            _renderingSystem?.Render(_scene);
        }

        /// <summary>
        /// Handle window resize
        /// </summary>
        public void OnResize(int width, int height)
        {
            GL.Viewport(0, 0, width, height);
            _renderingSystem?.UpdateAspectRatio(width, height);
        }

        /// <summary>
        /// Get delta time for this frame
        /// </summary>
        public double DeltaTime => _deltaTime;

        /// <summary>
        /// Get the active scene
        /// </summary>
        public Scene Scene => _scene;
    }
}

// ==================== EXAMPLE USAGE ====================

/*

EXAMPLE: How to use this in your Runtime/Editor with OpenTK GameWindow:

using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.Common;

public class GameWindow : OpenTK.Windowing.Desktop.GameWindow
{
    private Game _game;
    private Shader _defaultShader;
    private Dictionary<string, Mesh> _meshCache;
    
    public GameWindow() : base(GameWindowSettings.Default, NativeWindowSettings.Default)
    {
        Title = "My Game Engine";
        Size = new OpenTK.Mathematics.Vector2i(1280, 720);
    }
    
    protected override void OnLoad()
    {
        base.OnLoad();
        
        // Initialize OpenGL state
        GL.ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        
        // Create default shader
        _defaultShader = new Shader(
            DefaultShaders.BasicVertexShader,
            DefaultShaders.BasicFragmentShader
        );
        
        // Cache common meshes
        _meshCache = new Dictionary<string, Mesh>
        {
            ["cube"] = Mesh.CreateCube(),
            ["quad"] = Mesh.CreateQuad()
        };
        
        // Initialize game
        _game = new Game();
        _game.Initialize();
        
        // Add some test objects
        CreateTestCube();
    }
    
    private void CreateTestCube()
    {
        var cubeGO = _game.Scene.CreateGameObject("Test Cube");
        cubeGO.transform.position = new Vector3(0, 0, 0);
        
        var renderer = cubeGO.AddComponent<MeshRenderer>();
        renderer.mesh = _meshCache["cube"];
        renderer.material = new Material(_defaultShader)
        {
            Color = new Vector4(1.0f, 0.5f, 0.2f, 1.0f)
        };
        
        // Add a script to rotate it
        cubeGO.AddComponent<RotatingCube>();
    }
    
    protected override void OnUpdateFrame(FrameEventArgs args)
    {
        base.OnUpdateFrame(args);
        
        // Update game logic
        _game.Update(args.Time);
        _game.LateUpdate();
    }
    
    protected override void OnRenderFrame(FrameEventArgs args)
    {
        base.OnRenderFrame(args);
        
        // Render the scene
        _game.Render();
        
        // Swap buffers
        SwapBuffers();
    }
    
    protected override void OnResize(ResizeEventArgs e)
    {
        base.OnResize(e);
        _game.OnResize(e.Width, e.Height);
    }
}

// Example MonoBehaviour script
public class RotatingCube : MonoBehaviour
{
    public float rotationSpeed = 90f; // degrees per second
    
    protected override void Update()
    {
        // Rotate the cube
        float rotation = rotationSpeed * Time.DeltaTime;
        // transform.Rotate(0, rotation, 0); // You'd implement Rotate helper
    }
}

// Program entry point
class Program
{
    static void Main()
    {
        using (var game = new GameWindow())
        {
            game.Run();
        }
    }
}

*/
