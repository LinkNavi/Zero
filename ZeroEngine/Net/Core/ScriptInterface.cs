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
            ScriptFunction wrapper = _ => { action(); return 0; };
            RegisterFunction(name, wrapper);
        }

        /// <summary>
        /// Register a function with typed parameters
        /// </summary>
        public void RegisterFunction(string name, Func<double> func)
        {
            ScriptFunction wrapper = _ => func();
            RegisterFunction(name, wrapper);
        }

        public void RegisterFunction(string name, Func<double, double> func)
        {
            ScriptFunction wrapper = args => func(args.Length > 0 ? args[0] : 0);
            RegisterFunction(name, wrapper);
        }

        public void RegisterFunction(string name, Func<double, double, double> func)
        {
            ScriptFunction wrapper = args => func(
                args.Length > 0 ? args[0] : 0,
                args.Length > 1 ? args[1] : 0);
            RegisterFunction(name, wrapper);
        }

        public void RegisterFunction(string name, Func<double, double, double, double> func)
        {
            ScriptFunction wrapper = args => func(
                args.Length > 0 ? args[0] : 0,
                args.Length > 1 ? args[1] : 0,
                args.Length > 2 ? args[2] : 0);
            RegisterFunction(name, wrapper);
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

            for (int idx = 0; idx < parameters.Length; idx++)
            {
                var pType = parameters[idx].ParameterType;
                double argVal = idx < args.Length ? args[idx] : 0;

                if (pType == typeof(int)) convertedArgs[idx] = (int)argVal;
                else if (pType == typeof(float)) convertedArgs[idx] = (float)argVal;
                else if (pType == typeof(double)) convertedArgs[idx] = argVal;
                else if (pType == typeof(bool)) convertedArgs[idx] = argVal != 0;
                else convertedArgs[idx] = argVal;
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

            _getters[name] = () =>
            {
                var val = field.GetValue(obj);
                if (val is int intVal) return intVal;
                if (val is float floatVal) return floatVal;
                if (val is double doubleVal) return doubleVal;
                if (val is bool boolVal) return boolVal ? 1 : 0;
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
                    if (val is int intVal) return intVal;
                    if (val is float floatVal) return floatVal;
                    if (val is double doubleVal) return doubleVal;
                    if (val is bool boolVal) return boolVal ? 1 : 0;
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
            RegisterFunction("log", (double[] args) =>
            {
                string msg = args.Length > 0 ? args[0].ToString() : "";
                OnLog?.Invoke(msg);
                Console.WriteLine($"[Script] {msg}");
                return 0;
            });

            RegisterFunction("log_int", (double[] args) =>
            {
                int val = args.Length > 0 ? (int)args[0] : 0;
                OnLog?.Invoke(val.ToString());
                Console.WriteLine($"[Script] {val}");
                return 0;
            });

            RegisterFunction("print_str", (double[] args) =>
            {
                string msg = args.Length > 0 ? args[0].ToString() : "";
                OnLog?.Invoke(msg);
                Console.WriteLine($"[Script] {msg}");
                return 0;
            });

            // Math
            RegisterFunction("sqrt", (double[] args) => Math.Sqrt(args[0]));
            RegisterFunction("abs", (double[] args) => Math.Abs(args[0]));
            RegisterFunction("sin", (double[] args) => Math.Sin(args[0]));
            RegisterFunction("cos", (double[] args) => Math.Cos(args[0]));
            RegisterFunction("tan", (double[] args) => Math.Tan(args[0]));
            RegisterFunction("asin", (double[] args) => Math.Asin(args[0]));
            RegisterFunction("acos", (double[] args) => Math.Acos(args[0]));
            RegisterFunction("atan", (double[] args) => Math.Atan(args[0]));
            RegisterFunction("atan2", (double[] args) => Math.Atan2(args[0], args[1]));
            RegisterFunction("pow", (double[] args) => Math.Pow(args[0], args[1]));
            RegisterFunction("floor", (double[] args) => Math.Floor(args[0]));
            RegisterFunction("ceil", (double[] args) => Math.Ceiling(args[0]));
            RegisterFunction("round", (double[] args) => Math.Round(args[0]));
            RegisterFunction("min", (double[] args) => Math.Min(args[0], args[1]));
            RegisterFunction("max", (double[] args) => Math.Max(args[0], args[1]));
            RegisterFunction("clamp", (double[] args) => Math.Max(args[1], Math.Min(args[2], args[0])));
            RegisterFunction("lerp", (double[] args) => args[0] + (args[1] - args[0]) * args[2]);
            RegisterFunction("sign", (double[] args) => Math.Sign(args[0]));

            // Random
            var rng = new Random();
            RegisterFunction("random", (double[] args) => rng.NextDouble());
            RegisterFunction("random_range", (double[] args) => args[0] + rng.NextDouble() * (args[1] - args[0]));
            RegisterFunction("random_int", (double[] args) => rng.Next((int)args[0], (int)args[1]));
        }
    }

  
}
