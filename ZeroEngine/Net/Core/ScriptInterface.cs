using System;
using System.Collections.Generic;
using System.Reflection;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Scripting
{
    /// <summary>
    /// Delegate for native functions callable from scripts
    /// </summary>
    public delegate double ScriptFunction(double[] args);
    
    /// <summary>
    /// Attribute to mark methods as script-callable
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    public class ScriptCallableAttribute : Attribute
    {
        public string Name { get; }
        public ScriptCallableAttribute(string name = null) => Name = name;
    }

    /// <summary>
    /// Attribute to mark fields/properties as script-accessible
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public class ScriptVariableAttribute : Attribute
    {
        public string Name { get; }
        public bool ReadOnly { get; }
        public ScriptVariableAttribute(string name = null, bool readOnly = false)
        {
            Name = name;
            ReadOnly = readOnly;
        }
    }

    /// <summary>
    /// Interface for script-engine communication
    /// Allows defining custom functions, variables, and callbacks
    /// </summary>
    public class ScriptInterface
    {
        private ManagedScript _script;
        private Dictionary<string, ScriptFunction> _functions = new Dictionary<string, ScriptFunction>();
        private Dictionary<string, Func<double>> _getters = new Dictionary<string, Func<double>>();
        private Dictionary<string, Action<double>> _setters = new Dictionary<string, Action<double>>();
        private Dictionary<string, object> _boundObjects = new Dictionary<string, object>();
        
        // Event callbacks
        public Action OnStart;
        public Action OnUpdate;
        public Action OnLateUpdate;
        public Action OnDestroy;
        public Action<string> OnLog;

        public ManagedScript Script => _script;
        public bool IsLoaded => _script != null;

        public ScriptInterface()
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
        /// Load script from file
        /// </summary>
        public bool LoadFile(string path)
        {
            if (!System.IO.File.Exists(path))
            {
                Console.WriteLine($"Script file not found: {path}");
                return false;
            }
            return Load(System.IO.File.ReadAllText(path));
        }

        /// <summary>
        /// Register a native function callable from scripts
        /// </summary>
        public void RegisterFunction(string name, ScriptFunction func)
        {
            _functions[name] = func;
            _script?.RegisterNative(name, args => func(args));
        }

        /// <summary>
        /// Register a simple action (no args, no return)
        /// </summary>
        public void RegisterAction(string name, Action action)
        {
            RegisterFunction(name, _ => { action(); return 0; });
        }

        /// <summary>
        /// Register a function with typed parameters
        /// </summary>
        public void RegisterFunction(string name, Func<double> func)
        {
            RegisterFunction(name, _ => func());
        }

        public void RegisterFunction(string name, Func<double, double> func)
        {
            RegisterFunction(name, args => func(args.Length > 0 ? args[0] : 0));
        }

        public void RegisterFunction(string name, Func<double, double, double> func)
        {
            RegisterFunction(name, args => func(
                args.Length > 0 ? args[0] : 0,
                args.Length > 1 ? args[1] : 0));
        }

        public void RegisterFunction(string name, Func<double, double, double, double> func)
        {
            RegisterFunction(name, args => func(
                args.Length > 0 ? args[0] : 0,
                args.Length > 1 ? args[1] : 0,
                args.Length > 2 ? args[2] : 0));
        }

        /// <summary>
        /// Register a variable with getter and optional setter
        /// </summary>
        public void RegisterVariable(string name, Func<double> getter, Action<double> setter = null)
        {
            _getters[name] = getter;
            if (setter != null)
                _setters[name] = setter;
        }

        /// <summary>
        /// Register a reference to a float variable
        /// </summary>
        public void RegisterFloat(string name, ref float value)
        {
            float captured = value;
            _getters[name] = () => captured;
            _setters[name] = v => captured = (float)v;
        }

        /// <summary>
        /// Bind an object's marked methods and properties to the script
        /// </summary>
        public void BindObject(string prefix, object obj)
        {
            if (obj == null) return;
            
            _boundObjects[prefix] = obj;
            var type = obj.GetType();

            // Bind methods with ScriptCallable attribute
            foreach (var method in type.GetMethods(BindingFlags.Public | BindingFlags.Instance))
            {
                var attr = method.GetCustomAttribute<ScriptCallableAttribute>();
                if (attr != null)
                {
                    string name = string.IsNullOrEmpty(prefix) 
                        ? (attr.Name ?? method.Name)
                        : $"{prefix}_{attr.Name ?? method.Name}";
                    
                    RegisterFunction(name, args => InvokeMethod(obj, method, args));
                }
            }

            // Bind fields with ScriptVariable attribute
            foreach (var field in type.GetFields(BindingFlags.Public | BindingFlags.Instance))
            {
                var attr = field.GetCustomAttribute<ScriptVariableAttribute>();
                if (attr != null)
                {
                    string name = string.IsNullOrEmpty(prefix)
                        ? (attr.Name ?? field.Name)
                        : $"{prefix}_{attr.Name ?? field.Name}";
                    
                    BindField(name, obj, field, attr.ReadOnly);
                }
            }

            // Bind properties with ScriptVariable attribute
            foreach (var prop in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
            {
                var attr = prop.GetCustomAttribute<ScriptVariableAttribute>();
                if (attr != null)
                {
                    string name = string.IsNullOrEmpty(prefix)
                        ? (attr.Name ?? prop.Name)
                        : $"{prefix}_{attr.Name ?? prop.Name}";
                    
                    BindProperty(name, obj, prop, attr.ReadOnly);
                }
            }
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
                else convertedArgs[i] = argVal;
            }

            var result = method.Invoke(obj, convertedArgs);
            
            if (result == null) return 0;
            if (result is int i) return i;
            if (result is float f) return f;
            if (result is double d) return d;
            if (result is bool b) return b ? 1 : 0;
            return 0;
        }

        private void BindField(string name, object obj, FieldInfo field, bool readOnly)
        {
            var fType = field.FieldType;
            
            _getters[name] = () =>
            {
                var val = field.GetValue(obj);
                if (val is int i) return i;
                if (val is float f) return f;
                if (val is double d) return d;
                if (val is bool b) return b ? 1 : 0;
                return 0;
            };

            if (!readOnly)
            {
                _setters[name] = v =>
                {
                    if (fType == typeof(int)) field.SetValue(obj, (int)v);
                    else if (fType == typeof(float)) field.SetValue(obj, (float)v);
                    else if (fType == typeof(double)) field.SetValue(obj, v);
                    else if (fType == typeof(bool)) field.SetValue(obj, v != 0);
                };
            }
        }

        private void BindProperty(string name, object obj, PropertyInfo prop, bool readOnly)
        {
            var pType = prop.PropertyType;
            
            if (prop.CanRead)
            {
                _getters[name] = () =>
                {
                    var val = prop.GetValue(obj);
                    if (val is int i) return i;
                    if (val is float f) return f;
                    if (val is double d) return d;
                    if (val is bool b) return b ? 1 : 0;
                    return 0;
                };
            }

            if (prop.CanWrite && !readOnly)
            {
                _setters[name] = v =>
                {
                    if (pType == typeof(int)) prop.SetValue(obj, (int)v);
                    else if (pType == typeof(float)) prop.SetValue(obj, (float)v);
                    else if (pType == typeof(double)) prop.SetValue(obj, v);
                    else if (pType == typeof(bool)) prop.SetValue(obj, v != 0);
                };
            }
        }

        /// <summary>
        /// Sync variables from C# to script before update
        /// </summary>
        public void PushVariables()
        {
            if (_script == null) return;
            
            foreach (var kvp in _getters)
            {
                _script.SetFloat(kvp.Key, kvp.Value());
            }
        }

        /// <summary>
        /// Sync variables from script back to C# after update
        /// </summary>
        public void PullVariables()
        {
            if (_script == null) return;
            
            foreach (var kvp in _setters)
            {
                double val = _script.GetFloat(kvp.Key);
                kvp.Value(val);
            }
        }

        /// <summary>
        /// Set a script variable directly
        /// </summary>
        public void SetFloat(string name, double value) => _script?.SetFloat(name, value);
        public void SetInt(string name, long value) => _script?.SetInt(name, value);

        /// <summary>
        /// Get a script variable directly
        /// </summary>
        public double GetFloat(string name) => _script?.GetFloat(name) ?? 0;
        public long GetInt(string name) => _script?.GetInt(name) ?? 0;

        /// <summary>
        /// Call a script function by name
        /// </summary>
        public double Call(string funcName)
        {
            return _script?.CallIfExists(funcName) ?? 0;
        }

        /// <summary>
        /// Execute lifecycle callbacks
        /// </summary>
        public void ExecuteStart()
        {
            OnStart?.Invoke();
            Call("on_start");
        }

        public void ExecuteUpdate(float deltaTime, float totalTime)
        {
            if (_script == null) return;
            
            _script.SetFloat("delta_time", deltaTime);
            _script.SetFloat("total_time", totalTime);
            
            PushVariables();
            OnUpdate?.Invoke();
            Call("on_update");
            PullVariables();
        }

        public void ExecuteLateUpdate()
        {
            OnLateUpdate?.Invoke();
            Call("on_late_update");
        }

        public void ExecuteDestroy()
        {
            OnDestroy?.Invoke();
            Call("on_destroy");
        }

        private void RegisterBuiltinFunctions()
        {
            // Logging
            RegisterFunction("log", args =>
            {
                string msg = args.Length > 0 ? args[0].ToString() : "";
                OnLog?.Invoke(msg);
                Console.WriteLine($"[Script] {msg}");
                return 0;
            });

            RegisterFunction("log_int", args =>
            {
                int val = args.Length > 0 ? (int)args[0] : 0;
                OnLog?.Invoke(val.ToString());
                Console.WriteLine($"[Script] {val}");
                return 0;
            });

            // Math
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
            RegisterFunction("random", _ => rng.NextDouble());
            RegisterFunction("random_range", args => args[0] + rng.NextDouble() * (args[1] - args[0]));
            RegisterFunction("random_int", args => rng.Next((int)args[0], (int)args[1]));
        }
    }

    /// <summary>
    /// Enhanced ScriptBehaviour with ScriptInterface support
    /// </summary>
    public class ScriptComponent : MonoBehaviour
    {
        public string ScriptPath { get; set; }
        public string ScriptSource { get; set; }
        
        private ScriptInterface _interface;
        private bool _initialized;

        public ScriptInterface Interface => _interface;

        public ScriptComponent()
        {
            _interface = new ScriptInterface();
        }

        /// <summary>
        /// Configure the script before it starts
        /// </summary>
        public void Configure(Action<ScriptInterface> setup)
        {
            setup?.Invoke(_interface);
        }

        protected override void Awake()
        {
            // Load script
            if (!string.IsNullOrEmpty(ScriptPath) && System.IO.File.Exists(ScriptPath))
            {
                ScriptSource = System.IO.File.ReadAllText(ScriptPath);
            }

            if (!string.IsNullOrEmpty(ScriptSource))
            {
                // Bind transform
                BindTransform();
                
                // Bind GameObject info
                _interface.RegisterVariable("active", 
                    () => gameObject.activeSelf ? 1 : 0,
                    v => gameObject.activeSelf = v != 0);
                
                if (_interface.Load(ScriptSource))
                {
                    _initialized = true;
                    Console.WriteLine($"Script loaded for {gameObject.name}");
                }
            }
        }

        private void BindTransform()
        {
            // Position
            _interface.RegisterVariable("pos_x",
                () => transform.position.x,
                v => { var p = transform.position; p.x = (float)v; transform.position = p; });
            _interface.RegisterVariable("pos_y",
                () => transform.position.y,
                v => { var p = transform.position; p.y = (float)v; transform.position = p; });
            _interface.RegisterVariable("pos_z",
                () => transform.position.z,
                v => { var p = transform.position; p.z = (float)v; transform.position = p; });

            // Scale
            _interface.RegisterVariable("scale_x",
                () => transform.localScale.x,
                v => { var s = transform.localScale; s.x = (float)v; transform.localScale = s; });
            _interface.RegisterVariable("scale_y",
                () => transform.localScale.y,
                v => { var s = transform.localScale; s.y = (float)v; transform.localScale = s; });
            _interface.RegisterVariable("scale_z",
                () => transform.localScale.z,
                v => { var s = transform.localScale; s.z = (float)v; transform.localScale = s; });

            // Rotation helpers
            _interface.RegisterFunction("rotate", args =>
            {
                transform.Rotate((float)args[0], (float)args[1], (float)args[2]);
                return 0;
            });

            _interface.RegisterFunction("look_at", args =>
            {
                transform.LookAt(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });

            _interface.RegisterFunction("translate", args =>
            {
                transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), args.Length > 3 && args[3] != 0);
                return 0;
            });
        }

        protected override void Start()
        {
            if (_initialized)
            {
                _interface.ExecuteStart();
            }
        }

        protected override void Update()
        {
            if (_initialized)
            {
                _interface.ExecuteUpdate(Time.DeltaTime, Time.TotalTime);
            }
        }

        protected override void LateUpdate()
        {
            if (_initialized)
            {
                _interface.ExecuteLateUpdate();
            }
        }

        protected override void OnDestroy()
        {
            if (_initialized)
            {
                _interface.ExecuteDestroy();
            }
        }
    }
}
