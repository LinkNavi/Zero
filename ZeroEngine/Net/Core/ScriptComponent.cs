using System;
using System.IO;
using System.Collections.Generic;
using EngineCore.ECS;
using EngineCore.Scripting;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Scripting
{
    /// <summary>
    /// Component that runs Magolor scripts with Unity-like API
    /// Scripts are written in Magolor language with class-based structure
    /// </summary>
    public class MagolorScriptComponent : MonoBehaviour
    {
        public string scriptPath;
        public string scriptSource;

        private ScriptInterface _interface;
        private bool _started = false;

        protected override void Awake()
        {
            if (!string.IsNullOrEmpty(scriptPath) && File.Exists(scriptPath))
            {
                scriptSource = File.ReadAllText(scriptPath);
            }

            if (!string.IsNullOrEmpty(scriptSource))
            {
                InitializeScript();
            }
        }

        private void InitializeScript()
        {
            _interface = new ScriptInterface();

            // Bind GameObject and Transform
            BindTransform();
            BindInput();
            BindPhysics();
            BindUtilities();
            BindGameObject();

            // Load and compile script
            if (_interface.Load(scriptSource))
            {
                Console.WriteLine($"[Magolor] Script loaded: {gameObject.name}");
            }
            else
            {
                Console.WriteLine($"[Magolor] Failed to load script for: {gameObject.name}");
            }
        }

        private void BindTransform()
        {
            // Position
            _interface.RegisterVariable("position_x", () => transform.position.x, v =>
            {
                var pos = transform.position;
                 pos.x = (float)v;
                transform.position = pos;
            });
            _interface.RegisterVariable("position_y", () => transform.position.y, v =>
            {
                var pos = transform.position;
                pos.y = (float)v;
                transform.position = pos;
            });
            _interface.RegisterVariable("position_z", () => transform.position.z, v =>
            {
                var pos = transform.position;
                pos.z = (float)v;
                transform.position = pos;
            });

            // Rotation (Euler angles)
            _interface.RegisterVariable("rotation_x", () => transform.GetEulerAngles().x, v =>
            {
                var euler = transform.GetEulerAngles();
                transform.SetEulerAngles((float)v, euler.y, euler.z);
            });
            _interface.RegisterVariable("rotation_y", () => transform.GetEulerAngles().y, v =>
            {
                var euler = transform.GetEulerAngles();
                transform.SetEulerAngles(euler.x, (float)v, euler.z);
            });
            _interface.RegisterVariable("rotation_z", () => transform.GetEulerAngles().z, v =>
            {
                var euler = transform.GetEulerAngles();
                transform.SetEulerAngles(euler.x, euler.y, (float)v);
            });

            // Scale
            _interface.RegisterVariable("scale_x", () => transform.localScale.x, v =>
            {
                var scale = transform.localScale;
                scale.x = (float)v;
                transform.localScale = scale;
            });
            _interface.RegisterVariable("scale_y", () => transform.localScale.y, v =>
            {
                var scale = transform.localScale;
                scale.y = (float)v;
                transform.localScale = scale;
            });
            _interface.RegisterVariable("scale_z", () => transform.localScale.z, v =>
            {
                var scale = transform.localScale;
                scale.z = (float)v;
                transform.localScale = scale;
            });

            // Transform methods
            _interface.RegisterFunction("Translate", (args) =>
            {
                if (args.Length >= 3)
                {
                    transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), true);
                }
                return 0;
            });

            _interface.RegisterFunction("Rotate", (args) =>
            {
                if (args.Length >= 3)
                {
                    transform.Rotate((float)args[0], (float)args[1], (float)args[2]);
                }
                return 0;
            });

            _interface.RegisterFunction("LookAt", (args) =>
            {
                if (args.Length >= 3)
                {
                    transform.LookAt(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                }
                return 0;
            });
        }

        private void BindInput()
        {
            // Keyboard
            _interface.RegisterFunction("GetKey", (args) =>
            {
                if (args.Length > 0)
                {
                    var keyCode = (Input.KeyCode)(int)args[0];
                    return Input.Input.GetKey(keyCode) ? 1.0 : 0.0;
                }
                return 0;
            });

            _interface.RegisterFunction("GetKeyDown", (args) =>
            {
                if (args.Length > 0)
                {
                    var keyCode = (Input.KeyCode)(int)args[0];
                    return Input.Input.GetKeyDown(keyCode) ? 1.0 : 0.0;
                }
                return 0;
            });

            _interface.RegisterFunction("GetKeyUp", (args) =>
            {
                if (args.Length > 0)
                {
                    var keyCode = (Input.KeyCode)(int)args[0];
                    return Input.Input.GetKeyUp(keyCode) ? 1.0 : 0.0;
                }
                return 0;
            });

            // Axis
            _interface.RegisterVariable("horizontal", () => Input.Input.GetAxisHorizontal());
            _interface.RegisterVariable("vertical", () => Input.Input.GetAxisVertical());

            // Mouse
            _interface.RegisterVariable("mouse_x", () => Input.Input.MousePosition.X);
            _interface.RegisterVariable("mouse_y", () => Input.Input.MousePosition.Y);
            _interface.RegisterVariable("mouse_delta_x", () => Input.Input.MouseDelta.X);
            _interface.RegisterVariable("mouse_delta_y", () => Input.Input.MouseDelta.Y);

            _interface.RegisterFunction("GetMouseButton", (args) =>
            {
                if (args.Length > 0)
                {
                    return Input.Input.GetMouseButton((Input.MouseButton)(int)args[0]) ? 1.0 : 0.0;
                }
                return 0;
            });
        }

        private void BindPhysics()
        {
            // Try to get Rigidbody component
            var rigidbody = gameObject.GetComponent<Physics.Rigidbody>();

            if (rigidbody != null)
            {
                _interface.RegisterVariable("velocity_x", () => rigidbody.velocity.x, v =>
                {
                    rigidbody.velocity.x = (float)v;
                });
                _interface.RegisterVariable("velocity_y", () => rigidbody.velocity.y, v =>
                {
                    rigidbody.velocity.y = (float)v;
                });
                _interface.RegisterVariable("velocity_z", () => rigidbody.velocity.z, v =>
                {
                    rigidbody.velocity.z = (float)v;
                });

                _interface.RegisterFunction("AddForce", (args) =>
                {
                    if (args.Length >= 3)
                    {
                        rigidbody.AddForce(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                    }
                    return 0;
                });

                _interface.RegisterVariable("use_gravity", () => rigidbody.useGravity ? 1.0 : 0.0, v =>
                {
                    rigidbody.useGravity = v > 0.5;
                });
            }
            else
            {
                // Provide dummy variables
                double vel_x = 0, vel_y = 0, vel_z = 0;
                _interface.RegisterVariable("velocity_x", () => vel_x, v => vel_x = v);
                _interface.RegisterVariable("velocity_y", () => vel_y, v => vel_y = v);
                _interface.RegisterVariable("velocity_z", () => vel_z, v => vel_z = v);
            }
        }

        private void BindUtilities()
        {
            // Time
            _interface.RegisterVariable("delta_time", () => Time.DeltaTime);
            _interface.RegisterVariable("time", () => Time.TotalTime);
            _interface.RegisterVariable("time_scale", () => Time.TimeScale, v => Time.TimeScale = (float)v);

            // Logging
            _interface.RegisterFunction("Log", (args) =>
            {
                if (args.Length > 0)
                {
                    Console.WriteLine($"[{gameObject.name}] {args[0]}");
                }
                return 0;
            });

            _interface.RegisterFunction("LogWarning", (args) =>
            {
                if (args.Length > 0)
                {
                    Console.WriteLine($"[WARNING - {gameObject.name}] {args[0]}");
                }
                return 0;
            });

            _interface.RegisterFunction("LogError", (args) =>
            {
                if (args.Length > 0)
                {
                    Console.WriteLine($"[ERROR - {gameObject.name}] {args[0]}");
                }
                return 0;
            });

            // GameObject
            _interface.RegisterVariable("active", () => gameObject.activeSelf ? 1.0 : 0.0, v =>
            {
                gameObject.activeSelf = v > 0.5;
            });


            _interface.RegisterFunction("Destroy", (double[] args) =>
            {
                gameObject.Destroy();
                return 0;
            });

        }

        private void BindGameObject()
        {
            // Find objects
            //   _interface.RegisterFunction("FindGameObject", (args) => {
            // This would need string support in Magolor
            // For now, we'll use a simple approach
            //     return 0;
            //});

            // _interface.RegisterFunction("GetComponent", (args) => {
            // Component access would need more sophisticated binding
            //   return 0;
            // });
        }

        protected override void Start()
        {
            if (_interface != null && !_started)
            {
                _started = true;
                _interface.ExecuteStart();
            }
        }

        protected override void Update()
        {
            if (_interface != null && enabled)
            {
                _interface.ExecuteUpdate(Time.DeltaTime, Time.TotalTime);
            }
        }

        protected override void LateUpdate()
        {
            if (_interface != null && enabled)
            {
                _interface.ExecuteLateUpdate();
            }
        }

        protected override void OnDestroy()
        {
            if (_interface != null)
            {
                _interface.ExecuteDestroy();
            }
        }

        /// <summary>
        /// Create a MagolorScriptComponent from a script file
        /// </summary>
        public static MagolorScriptComponent CreateFromFile(GameObject gameObject, string scriptPath)
        {
            var component = gameObject.AddComponent<MagolorScriptComponent>();
            component.scriptPath = scriptPath;
            return component;
        }

        /// <summary>
        /// Create a MagolorScriptComponent from source code
        /// </summary>
        public static MagolorScriptComponent CreateFromSource(GameObject gameObject, string source)
        {
            var component = gameObject.AddComponent<MagolorScriptComponent>();
            component.scriptSource = source;
            return component;
        }
    }
}
