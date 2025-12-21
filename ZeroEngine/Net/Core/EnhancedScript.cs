using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Scripting
{
    /// <summary>
    /// Enhanced ScriptInterface with Unity-like variable binding
    /// Supports automatic property/field discovery and binding
    /// </summary>
    public class EnhancedScriptInterface
    {
        private ManagedScript _script;
        private Dictionary<string, ScriptFunction> _functions = new Dictionary<string, ScriptFunction>();
        private Dictionary<string, ScriptVariable> _variables = new Dictionary<string, ScriptVariable>();
        private Dictionary<string, object> _boundObjects = new Dictionary<string, object>();

        // Lifecycle callbacks
        public Action OnStart;
        public Action OnUpdate;
        public Action OnLateUpdate;
        public Action OnFixedUpdate;
        public Action OnDestroy;

        public ManagedScript Script => _script;
        public bool IsLoaded => _script != null;

        private class ScriptVariable
        {
            public Func<double> Getter;
            public Action<double> Setter;
            public bool ReadOnly;
            public Type SourceType;
            public object SourceObject;
            public MemberInfo SourceMember;
        }

        public EnhancedScriptInterface()
        {
            RegisterBuiltinFunctions();
        }

        /// <summary>
        /// Load and compile a script from source
        /// </summary>
        public bool Load(string source)
        {
            try
            {
                _script = new ManagedScript();
                _script.Parse(source);

                // Register all functions
                foreach (var kvp in _functions)
                {
                    _script.RegisterNative(kvp.Key, args => kvp.Value(args));
                }

                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Script load error: {ex.Message}");
                _script = null;
                return false;
            }
        }

        /// <summary>
        /// Bind an object with automatic discovery of all public fields/properties
        /// Supports nested object access via dot notation
        /// </summary>
        public void BindObject(string prefix, object obj, bool bindMethods = true, bool bindFields = true, bool bindProperties = true)
        {
            if (obj == null) return;

            _boundObjects[prefix] = obj;
            var type = obj.GetType();

            // Bind methods
            if (bindMethods)
            {
                foreach (var method in type.GetMethods(BindingFlags.Public | BindingFlags.Instance))
                {
                    if (method.DeclaringType == typeof(object)) continue; // Skip Object methods
                    
                    var attr = method.GetCustomAttribute<ScriptCallableAttribute>();
                    if (attr != null || !method.IsSpecialName) // Bind all public methods or those with attribute
                    {
                        string name = FormatName(prefix, attr?.Name ?? method.Name);
                        RegisterFunction(name, args => InvokeMethod(obj, method, args));
                    }
                }
            }

            // Bind fields
            if (bindFields)
            {
                foreach (var field in type.GetFields(BindingFlags.Public | BindingFlags.Instance))
                {
                    var attr = field.GetCustomAttribute<ScriptVariableAttribute>();
                    string name = FormatName(prefix, attr?.Name ?? field.Name);
                    bool readOnly = attr?.ReadOnly ?? false;
                    
                    BindField(name, obj, field, readOnly);
                }
            }

            // Bind properties
            if (bindProperties)
            {
                foreach (var prop in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
                {
                    var attr = prop.GetCustomAttribute<ScriptVariableAttribute>();
                    string name = FormatName(prefix, attr?.Name ?? prop.Name);
                    bool readOnly = attr?.ReadOnly ?? !prop.CanWrite;
                    
                    BindProperty(name, obj, prop, readOnly);
                }
            }
        }

        /// <summary>
        /// Auto-bind a GameObject and all its common components
        /// This creates a comprehensive scripting API similar to Unity
        /// </summary>
        public void BindGameObject(GameObject gameObject)
        {
            if (gameObject == null) return;

            // Bind transform properties
            var transform = gameObject.transform;
            
            // Position
            RegisterVariable("position_x", () => transform.position.x, v => {
                var pos = transform.position; pos.x = (float)v; transform.position = pos;
            });
            RegisterVariable("position_y", () => transform.position.y, v => {
                var pos = transform.position; pos.y = (float)v; transform.position = pos;
            });
            RegisterVariable("position_z", () => transform.position.z, v => {
                var pos = transform.position; pos.z = (float)v; transform.position = pos;
            });

            // Rotation (Euler)
            RegisterVariable("rotation_x", () => transform.GetEulerAngles().x, v => {
                var euler = transform.GetEulerAngles();
                transform.SetEulerAngles((float)v, euler.y, euler.z);
            });
            RegisterVariable("rotation_y", () => transform.GetEulerAngles().y, v => {
                var euler = transform.GetEulerAngles();
                transform.SetEulerAngles(euler.x, (float)v, euler.z);
            });
            RegisterVariable("rotation_z", () => transform.GetEulerAngles().z, v => {
                var euler = transform.GetEulerAngles();
                transform.SetEulerAngles(euler.x, euler.y, (float)v);
            });

            // Scale
            RegisterVariable("scale_x", () => transform.localScale.x, v => {
                var scale = transform.localScale; scale.x = (float)v; transform.localScale = scale;
            });
            RegisterVariable("scale_y", () => transform.localScale.y, v => {
                var scale = transform.localScale; scale.y = (float)v; transform.localScale = scale;
            });
            RegisterVariable("scale_z", () => transform.localScale.z, v => {
                var scale = transform.localScale; scale.z = (float)v; transform.localScale = scale;
            });

            // Transform methods
            RegisterFunction("Translate", args => {
                if (args.Length >= 3)
                    transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), true);
                return 0;
            });

            RegisterFunction("Rotate", args => {
                if (args.Length >= 3)
                    transform.Rotate((float)args[0], (float)args[1], (float)args[2]);
                return 0;
            });

            RegisterFunction("LookAt", args => {
                if (args.Length >= 3)
                    transform.LookAt(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });

            // GameObject state
            RegisterVariable("active", () => gameObject.activeSelf ? 1.0 : 0.0, 
                v => gameObject.activeSelf = v > 0.5);

            // Bind input system
            BindInput();

            // Bind time
            BindTime();

            // Auto-bind common components if present
            var rigidbody = gameObject.GetComponent<Physics.PhysXRigidbody>();
            if (rigidbody != null) BindRigidbody(rigidbody);

            var collider = gameObject.GetComponent<Physics.PhysXCollider>();
            if (collider != null) BindCollider(collider);
        }

        private void BindInput()
        {
            // Keyboard
            RegisterFunction("GetKey", args => {
                if (args.Length > 0)
                    return Input.Input.GetKey((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0;
                return 0;
            });

            RegisterFunction("GetKeyDown", args => {
                if (args.Length > 0)
                    return Input.Input.GetKeyDown((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0;
                return 0;
            });

            RegisterFunction("GetKeyUp", args => {
                if (args.Length > 0)
                    return Input.Input.GetKeyUp((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0;
                return 0;
            });

            // Axis
            RegisterVariable("horizontal", () => Input.Input.GetAxisHorizontal());
            RegisterVariable("vertical", () => Input.Input.GetAxisVertical());

            // Mouse
            RegisterVariable("mouse_x", () => Input.Input.MousePosition.X);
            RegisterVariable("mouse_y", () => Input.Input.MousePosition.Y);
            RegisterVariable("mouse_delta_x", () => Input.Input.MouseDelta.X);
            RegisterVariable("mouse_delta_y", () => Input.Input.MouseDelta.Y);

            RegisterFunction("GetMouseButton", args => {
                if (args.Length > 0)
                    return Input.Input.GetMouseButton((Input.MouseButton)(int)args[0]) ? 1.0 : 0.0;
                return 0;
            });
        }

        private void BindTime()
        {
            RegisterVariable("delta_time", () => Time.DeltaTime);
            RegisterVariable("time", () => Time.TotalTime);
            RegisterVariable("fixed_delta_time", () => Time.UnscaledDeltaTime);
            RegisterVariable("time_scale", () => Time.TimeScale, v => Time.TimeScale = (float)v);
        }

        private void BindRigidbody(Physics.PhysXRigidbody rb)
        {
            // Velocity
            RegisterVariable("velocity_x", () => rb.LinearVelocity.x, 
                v => { var vel = rb.LinearVelocity; vel.x = (float)v; rb.LinearVelocity = vel; });
            RegisterVariable("velocity_y", () => rb.LinearVelocity.y, 
                v => { var vel = rb.LinearVelocity; vel.y = (float)v; rb.LinearVelocity = vel; });
            RegisterVariable("velocity_z", () => rb.LinearVelocity.z, 
                v => { var vel = rb.LinearVelocity; vel.z = (float)v; rb.LinearVelocity = vel; });

            // Angular velocity
            RegisterVariable("angular_velocity_x", () => rb.AngularVelocity.x, 
                v => { var vel = rb.AngularVelocity; vel.x = (float)v; rb.AngularVelocity = vel; });
            RegisterVariable("angular_velocity_y", () => rb.AngularVelocity.y, 
                v => { var vel = rb.AngularVelocity; vel.y = (float)v; rb.AngularVelocity = vel; });
            RegisterVariable("angular_velocity_z", () => rb.AngularVelocity.z, 
                v => { var vel = rb.AngularVelocity; vel.z = (float)v; rb.AngularVelocity = vel; });

            // Properties
            RegisterVariable("mass", () => rb.Mass, v => rb.Mass = (float)v);
            RegisterVariable("drag", () => rb.LinearDamping, v => rb.LinearDamping = (float)v);
            RegisterVariable("angular_drag", () => rb.AngularDamping, v => rb.AngularDamping = (float)v);
            RegisterVariable("use_gravity", () => rb.UseGravity ? 1.0 : 0.0, 
                v => rb.UseGravity = v > 0.5);
            RegisterVariable("is_kinematic", () => rb.IsKinematic ? 1.0 : 0.0, 
                v => rb.IsKinematic = v > 0.5);

            // Forces
            RegisterFunction("AddForce", args => {
                if (args.Length >= 3)
                    rb.AddForce(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });

            RegisterFunction("AddTorque", args => {
                if (args.Length >= 3)
                    rb.AddTorque(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });

            RegisterFunction("AddForceAtPosition", args => {
                if (args.Length >= 6)
                    rb.AddForceAtPosition(
                        new ECSVector3((float)args[0], (float)args[1], (float)args[2]),
                        new ECSVector3((float)args[3], (float)args[4], (float)args[5]));
                return 0;
            });
        }

        private void BindCollider(Physics.PhysXCollider collider)
        {
            RegisterVariable("is_trigger", () => collider.IsTrigger ? 1.0 : 0.0, 
                v => collider.IsTrigger = v > 0.5);
        }

        /// <summary>
        /// Register a variable with getter and optional setter
        /// </summary>
        public void RegisterVariable(string name, Func<double> getter, Action<double> setter = null)
        {
            _variables[name] = new ScriptVariable
            {
                Getter = getter,
                Setter = setter,
                ReadOnly = setter == null
            };
        }

        /// <summary>
        /// Register a native function
        /// </summary>
        public void RegisterFunction(string name, ScriptFunction func)
        {
            _functions[name] = func;
            _script?.RegisterNative(name, args => func(args));
        }

        /// <summary>
        /// Sync variables from C# to script before update
        /// </summary>
        public void PushVariables()
        {
            if (_script == null) return;

            foreach (var kvp in _variables)
            {
                try
                {
                    _script.SetFloat(kvp.Key, kvp.Value.Getter());
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error pushing variable {kvp.Key}: {ex.Message}");
                }
            }
        }

        /// <summary>
        /// Sync variables from script back to C# after update
        /// </summary>
        public void PullVariables()
        {
            if (_script == null) return;

            foreach (var kvp in _variables)
            {
                if (kvp.Value.Setter != null)
                {
                    try
                    {
                        double val = _script.GetFloat(kvp.Key);
                        kvp.Value.Setter(val);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Error pulling variable {kvp.Key}: {ex.Message}");
                    }
                }
            }
        }

        /// <summary>
        /// Execute lifecycle callbacks
        /// </summary>
        public void ExecuteStart()
        {
            OnStart?.Invoke();
            var result = Call("OnStart");
            if (result == null) Call("on_start");
        }

        public void ExecuteUpdate(float deltaTime, float totalTime)
        {
            if (_script == null) return;

            _script.SetFloat("delta_time", deltaTime);
            _script.SetFloat("total_time", totalTime);

            PushVariables();
            OnUpdate?.Invoke();
            var result = Call("OnUpdate");
            if (result == null) Call("on_update");
            PullVariables();
        }

        public void ExecuteFixedUpdate()
        {
            if (_script == null) return;

            PushVariables();
            OnFixedUpdate?.Invoke();
            var result = Call("OnFixedUpdate");
            if (result == null) Call("on_fixed_update");
            PullVariables();
        }

        public void ExecuteLateUpdate()
        {
            OnLateUpdate?.Invoke();
            var result = Call("OnLateUpdate");
            if (result == null) Call("on_late_update");
        }

        public void ExecuteDestroy()
        {
            OnDestroy?.Invoke();
            var result = Call("OnDestroy");
            if (result == null) Call("on_destroy");
        }

        /// <summary>
        /// Call a script function by name
        /// </summary>
        public double? Call(string funcName)
        {
            try
            {
                return _script?.CallIfExists(funcName);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error calling {funcName}: {ex.Message}");
                return null;
            }
        }

        private string FormatName(string prefix, string name)
        {
            // Convert PascalCase to snake_case for Magolor convention
            string converted = "";
            for (int i = 0; i < name.Length; i++)
            {
                if (i > 0 && char.IsUpper(name[i]) && !char.IsUpper(name[i - 1]))
                    converted += "_";
                converted += char.ToLower(name[i]);
            }

            return string.IsNullOrEmpty(prefix) ? converted : $"{prefix}_{converted}";
        }

        private double InvokeMethod(object obj, MethodInfo method, double[] args)
        {
            var parameters = method.GetParameters();
            object[] convertedArgs = new object[parameters.Length];

            for (int i = 0; i < parameters.Length; i++)
            {
                var pType = parameters[i].ParameterType;
                double argVal = i < args.Length ? args[i] : 0;

                if (pType == typeof(int)) convertedArgs[i] = (int)argVal;
                else if (pType == typeof(float)) convertedArgs[i] = (float)argVal;
                else if (pType == typeof(double)) convertedArgs[i] = argVal;
                else if (pType == typeof(bool)) convertedArgs[i] = argVal != 0;
                else if (pType == typeof(ECSVector3) && i + 2 < args.Length)
                {
                    convertedArgs[i] = new ECSVector3((float)args[i], (float)args[i+1], (float)args[i+2]);
                    i += 2; // Skip next 2 args
                }
                else convertedArgs[i] = argVal;
            }

            var result = method.Invoke(obj, convertedArgs);

            if (result == null) return 0;
            if (result is int intResult) return intResult;
            if (result is float floatResult) return floatResult;
            if (result is double doubleResult) return doubleResult;
            if (result is bool boolResult) return boolResult ? 1 : 0;
            return 0;
        }

        private void BindField(string name, object obj, FieldInfo field, bool readOnly)
        {
            var fType = field.FieldType;

            _variables[name] = new ScriptVariable
            {
                Getter = () => ConvertToDouble(field.GetValue(obj)),
                Setter = readOnly ? null : v => field.SetValue(obj, ConvertFromDouble(v, fType)),
                ReadOnly = readOnly,
                SourceType = fType,
                SourceObject = obj,
                SourceMember = field
            };
        }

        private void BindProperty(string name, object obj, PropertyInfo prop, bool readOnly)
        {
            var pType = prop.PropertyType;

            _variables[name] = new ScriptVariable
            {
                Getter = prop.CanRead ? () => ConvertToDouble(prop.GetValue(obj)) : null,
                Setter = !readOnly && prop.CanWrite ? v => prop.SetValue(obj, ConvertFromDouble(v, pType)) : null,
                ReadOnly = readOnly,
                SourceType = pType,
                SourceObject = obj,
                SourceMember = prop
            };
        }

        private double ConvertToDouble(object value)
        {
            if (value == null) return 0;
            if (value is int i) return i;
            if (value is float f) return f;
            if (value is double d) return d;
            if (value is bool b) return b ? 1 : 0;
            if (value is byte by) return by;
            if (value is short s) return s;
            if (value is long l) return l;
            return 0;
        }

        private object ConvertFromDouble(double value, Type targetType)
        {
            if (targetType == typeof(int)) return (int)value;
            if (targetType == typeof(float)) return (float)value;
            if (targetType == typeof(double)) return value;
            if (targetType == typeof(bool)) return value != 0;
            if (targetType == typeof(byte)) return (byte)value;
            if (targetType == typeof(short)) return (short)value;
            if (targetType == typeof(long)) return (long)value;
            return value;
        }

        private void RegisterBuiltinFunctions()
        {
            // Logging
            RegisterFunction("Log", args => {
                if (args.Length > 0)
                    Console.WriteLine($"[Script] {args[0]}");
                return 0;
            });

            RegisterFunction("LogWarning", args => {
                if (args.Length > 0)
                    Console.WriteLine($"[WARNING - Script] {args[0]}");
                return 0;
            });

            RegisterFunction("LogError", args => {
                if (args.Length > 0)
                    Console.WriteLine($"[ERROR - Script] {args[0]}");
                return 0;
            });

            // Math functions
            RegisterFunction("sqrt", args => Math.Sqrt(args[0]));
            RegisterFunction("abs", args => Math.Abs(args[0]));
            RegisterFunction("sin", args => Math.Sin(args[0]));
            RegisterFunction("cos", args => Math.Cos(args[0]));
            RegisterFunction("tan", args => Math.Tan(args[0]));
            RegisterFunction("asin", args => Math.Asin(args[0]));
            RegisterFunction("acos", args => Math.Acos(args[0]));
            RegisterFunction("atan", args => Math.Atan(args[0]));
            RegisterFunction("atan2", args => Math.Atan2(args[0], args[1]));
            RegisterFunction("pow", args => Math.Pow(args[0], args[1]));
            RegisterFunction("floor", args => Math.Floor(args[0]));
            RegisterFunction("ceil", args => Math.Ceiling(args[0]));
            RegisterFunction("round", args => Math.Round(args[0]));
            RegisterFunction("min", args => Math.Min(args[0], args[1]));
            RegisterFunction("max", args => Math.Max(args[0], args[1]));
            RegisterFunction("clamp", args => Math.Max(args[1], Math.Min(args[2], args[0])));
            RegisterFunction("lerp", args => args[0] + (args[1] - args[0]) * args[2]);
            RegisterFunction("sign", args => Math.Sign(args[0]));

            // Random
            var rng = new Random();
            RegisterFunction("random", args => rng.NextDouble());
            RegisterFunction("random_range", args => args[0] + rng.NextDouble() * (args[1] - args[0]));
            RegisterFunction("random_int", args => rng.Next((int)args[0], (int)args[1]));
        }
    }

    /// <summary>
    /// Enhanced MagolorScriptComponent using the new system
    /// </summary>
    public class EnhancedMagolorScript : MonoBehaviour
    {
        public string scriptPath;
        public string scriptSource;

        private EnhancedScriptInterface _interface;
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
            _interface = new EnhancedScriptInterface();

            // Auto-bind the GameObject and its components
            _interface.BindGameObject(gameObject);

            // Load and compile script
            if (_interface.Load(scriptSource))
            {
                Console.WriteLine($"[Magolor] Enhanced script loaded: {gameObject.name}");
            }
            else
            {
                Console.WriteLine($"[Magolor] Failed to load script for: {gameObject.name}");
            }
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
        /// Get the script interface for manual variable/function access
        /// </summary>
        public EnhancedScriptInterface GetInterface() => _interface;

        /// <summary>
        /// Create from file
        /// </summary>
        public static EnhancedMagolorScript CreateFromFile(GameObject gameObject, string scriptPath)
        {
            var component = gameObject.AddComponent<EnhancedMagolorScript>();
            component.scriptPath = scriptPath;
            return component;
        }

        /// <summary>
        /// Create from source
        /// </summary>
        public static EnhancedMagolorScript CreateFromSource(GameObject gameObject, string source)
        {
            var component = gameObject.AddComponent<EnhancedMagolorScript>();
            component.scriptSource = source;
            return component;
        }
    }
}
