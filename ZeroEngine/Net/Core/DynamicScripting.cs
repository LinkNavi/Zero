using System;
using System.Collections.Generic;
using System.IO;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Scripting
{
    public delegate double NativeFunction(double[] args);
    public delegate double NativeFunctionWithContext(ScriptContext ctx, double[] args);

    public class ScriptContext
    {
        public GameObject gameObject;
        public Transform transform;
        public DynamicScriptEngine engine;
        
        // Cached lookups
        private Dictionary<string, GameObject> _cachedObjects = new();
        private Dictionary<int, GameObject> _handleMap = new();
        private int _nextHandle = 1;

        public int GetHandle(GameObject go)
        {
            foreach (var kv in _handleMap)
                if (kv.Value == go) return kv.Key;
            int h = _nextHandle++;
            _handleMap[h] = go;
            return h;
        }

        public GameObject GetByHandle(int handle) => _handleMap.TryGetValue(handle, out var go) ? go : null;
        
        public void ClearCache() => _cachedObjects.Clear();
    }

    public class DynamicScriptEngine
    {
        private Dictionary<string, NativeFunction> _functions = new();
        private Dictionary<string, NativeFunctionWithContext> _contextFunctions = new();
        private Dictionary<string, Func<ScriptContext, double>> _dynamicGetters = new();
        private Dictionary<string, Action<ScriptContext, double>> _dynamicSetters = new();

        public DynamicScriptEngine()
        {
            RegisterCoreFunctions();
            RegisterEntityFunctions();
            RegisterTransformFunctions();
            RegisterComponentFunctions();
            RegisterMathFunctions();
            RegisterInputFunctions();
            RegisterTimeFunctions();
        }

        void RegisterCoreFunctions()
        {
            // Logging
            RegisterContextFn("Log", (ctx, args) => {
                Console.WriteLine($"[{ctx.gameObject?.name ?? "Script"}] {args[0]}");
                return 0;
            });
            RegisterContextFn("LogWarning", (ctx, args) => {
                Console.WriteLine($"[WARNING - {ctx.gameObject?.name ?? "Script"}] {args[0]}");
                return 0;
            });
            RegisterContextFn("LogError", (ctx, args) => {
                Console.WriteLine($"[ERROR - {ctx.gameObject?.name ?? "Script"}] {args[0]}");
                return 0;
            });
        }

        void RegisterEntityFunctions()
        {
            // Find by name - returns handle
            RegisterContextFn("FindByName", (ctx, args) => {
                if (args.Length == 0) return 0;
                int nameHash = (int)args[0];
                string name = GetStringFromHash(nameHash);
                if (string.IsNullOrEmpty(name)) return 0;
                var go = GameObject.Find(name);
                return go != null ? ctx.GetHandle(go) : 0;
            });

            // Find by tag - returns handle
            RegisterContextFn("FindByTag", (ctx, args) => {
                if (args.Length == 0) return 0;
                int tagHash = (int)args[0];
                string tag = GetStringFromHash(tagHash);
                if (string.IsNullOrEmpty(tag)) return 0;
                var go = GameObject.FindWithTag(tag);
                return go != null ? ctx.GetHandle(go) : 0;
            });

            // Get position of entity by handle
            RegisterContextFn("GetEntityX", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.position.x ?? 0;
            });
            RegisterContextFn("GetEntityY", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.position.y ?? 0;
            });
            RegisterContextFn("GetEntityZ", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.position.z ?? 0;
            });

            // Set position of entity by handle
            RegisterContextFn("SetEntityX", (ctx, args) => {
                if (args.Length < 2) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                if (go != null) { var p = go.transform.position; p.x = (float)args[1]; go.transform.position = p; }
                return 0;
            });
            RegisterContextFn("SetEntityY", (ctx, args) => {
                if (args.Length < 2) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                if (go != null) { var p = go.transform.position; p.y = (float)args[1]; go.transform.position = p; }
                return 0;
            });
            RegisterContextFn("SetEntityZ", (ctx, args) => {
                if (args.Length < 2) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                if (go != null) { var p = go.transform.position; p.z = (float)args[1]; go.transform.position = p; }
                return 0;
            });

            // Get rotation
            RegisterContextFn("GetEntityRotX", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.GetEulerAngles().x ?? 0;
            });
            RegisterContextFn("GetEntityRotY", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.GetEulerAngles().y ?? 0;
            });
            RegisterContextFn("GetEntityRotZ", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.GetEulerAngles().z ?? 0;
            });

            // Get scale
            RegisterContextFn("GetEntityScaleX", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.localScale.x ?? 1;
            });
            RegisterContextFn("GetEntityScaleY", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.localScale.y ?? 1;
            });
            RegisterContextFn("GetEntityScaleZ", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.transform.localScale.z ?? 1;
            });

            // Entity state
            RegisterContextFn("IsEntityActive", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                return go?.activeSelf == true ? 1.0 : 0.0;
            });
            RegisterContextFn("SetEntityActive", (ctx, args) => {
                if (args.Length < 2) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                if (go != null) go.activeSelf = args[1] > 0.5;
                return 0;
            });

            // Distance between entities
            RegisterContextFn("DistanceTo", (ctx, args) => {
                if (args.Length == 0) return float.MaxValue;
                var target = ctx.GetByHandle((int)args[0]);
                if (target == null || ctx.transform == null) return float.MaxValue;
                return Vector3Extensions.Distance(ctx.transform.position, target.transform.position);
            });

            // Distance between two entities
            RegisterContextFn("DistanceBetween", (ctx, args) => {
                if (args.Length < 2) return float.MaxValue;
                var a = ctx.GetByHandle((int)args[0]);
                var b = ctx.GetByHandle((int)args[1]);
                if (a == null || b == null) return float.MaxValue;
                return Vector3Extensions.Distance(a.transform.position, b.transform.position);
            });

            // Entity valid check
            RegisterContextFn("IsValidEntity", (ctx, args) => {
                if (args.Length == 0) return 0;
                return ctx.GetByHandle((int)args[0]) != null ? 1.0 : 0.0;
            });

            // Self handle
            RegisterContextFn("GetSelf", (ctx, args) => {
                return ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0;
            });

            // Destroy entity
            RegisterContextFn("DestroyEntity", (ctx, args) => {
                if (args.Length == 0) return 0;
                var go = ctx.GetByHandle((int)args[0]);
                go?.Destroy();
                return 0;
            });

            // Destroy self
            RegisterContextFn("DestroySelf", (ctx, args) => {
                ctx.gameObject?.Destroy();
                return 0;
            });
        }

        void RegisterTransformFunctions()
        {
            // Self transform accessors
            RegisterDynamicVar("position_x", 
                ctx => ctx.transform?.position.x ?? 0,
                (ctx, v) => { if (ctx.transform != null) { var p = ctx.transform.position; p.x = (float)v; ctx.transform.position = p; } });
            RegisterDynamicVar("position_y",
                ctx => ctx.transform?.position.y ?? 0,
                (ctx, v) => { if (ctx.transform != null) { var p = ctx.transform.position; p.y = (float)v; ctx.transform.position = p; } });
            RegisterDynamicVar("position_z",
                ctx => ctx.transform?.position.z ?? 0,
                (ctx, v) => { if (ctx.transform != null) { var p = ctx.transform.position; p.z = (float)v; ctx.transform.position = p; } });

            RegisterDynamicVar("rotation_x",
                ctx => ctx.transform?.GetEulerAngles().x ?? 0,
                (ctx, v) => { if (ctx.transform != null) { var e = ctx.transform.GetEulerAngles(); ctx.transform.SetEulerAngles((float)v, e.y, e.z); } });
            RegisterDynamicVar("rotation_y",
                ctx => ctx.transform?.GetEulerAngles().y ?? 0,
                (ctx, v) => { if (ctx.transform != null) { var e = ctx.transform.GetEulerAngles(); ctx.transform.SetEulerAngles(e.x, (float)v, e.z); } });
            RegisterDynamicVar("rotation_z",
                ctx => ctx.transform?.GetEulerAngles().z ?? 0,
                (ctx, v) => { if (ctx.transform != null) { var e = ctx.transform.GetEulerAngles(); ctx.transform.SetEulerAngles(e.x, e.y, (float)v); } });

            RegisterDynamicVar("scale_x",
                ctx => ctx.transform?.localScale.x ?? 1,
                (ctx, v) => { if (ctx.transform != null) { var s = ctx.transform.localScale; s.x = (float)v; ctx.transform.localScale = s; } });
            RegisterDynamicVar("scale_y",
                ctx => ctx.transform?.localScale.y ?? 1,
                (ctx, v) => { if (ctx.transform != null) { var s = ctx.transform.localScale; s.y = (float)v; ctx.transform.localScale = s; } });
            RegisterDynamicVar("scale_z",
                ctx => ctx.transform?.localScale.z ?? 1,
                (ctx, v) => { if (ctx.transform != null) { var s = ctx.transform.localScale; s.z = (float)v; ctx.transform.localScale = s; } });

            // Transform methods
            RegisterContextFn("Translate", (ctx, args) => {
                if (args.Length >= 3 && ctx.transform != null)
                    ctx.transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), true);
                return 0;
            });
            RegisterContextFn("TranslateWorld", (ctx, args) => {
                if (args.Length >= 3 && ctx.transform != null)
                    ctx.transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), false);
                return 0;
            });
            RegisterContextFn("Rotate", (ctx, args) => {
                if (args.Length >= 3 && ctx.transform != null)
                    ctx.transform.Rotate((float)args[0], (float)args[1], (float)args[2]);
                return 0;
            });
            RegisterContextFn("LookAt", (ctx, args) => {
                if (args.Length >= 3 && ctx.transform != null)
                    ctx.transform.LookAt(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });
            RegisterContextFn("LookAtEntity", (ctx, args) => {
                if (args.Length >= 1 && ctx.transform != null) {
                    var target = ctx.GetByHandle((int)args[0]);
                    if (target != null) ctx.transform.LookAt(target.transform.position);
                }
                return 0;
            });

            // Move towards
            RegisterContextFn("MoveTowards", (ctx, args) => {
                if (args.Length >= 4 && ctx.transform != null) {
                    var target = new ECSVector3((float)args[0], (float)args[1], (float)args[2]);
                    var maxDist = (float)args[3];
                    ctx.transform.position = Vector3Extensions.MoveTowards(ctx.transform.position, target, maxDist);
                }
                return 0;
            });
            RegisterContextFn("MoveTowardsEntity", (ctx, args) => {
                if (args.Length >= 2 && ctx.transform != null) {
                    var target = ctx.GetByHandle((int)args[0]);
                    if (target != null) {
                        var maxDist = (float)args[1];
                        ctx.transform.position = Vector3Extensions.MoveTowards(ctx.transform.position, target.transform.position, maxDist);
                    }
                }
                return 0;
            });
        }

        void RegisterComponentFunctions()
        {
            // Rigidbody
            RegisterContextFn("HasRigidbody", (ctx, args) => {
                int handle = args.Length > 0 ? (int)args[0] : (ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0);
                var go = ctx.GetByHandle(handle);
                return go?.GetComponent<Physics.Rigidbody>() != null ? 1.0 : 0.0;
            });

            RegisterContextFn("GetVelocityX", (ctx, args) => {
                int handle = args.Length > 0 ? (int)args[0] : (ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0);
                var go = ctx.GetByHandle(handle);
                var rb = go?.GetComponent<Physics.Rigidbody>();
                return rb?.velocity.x ?? 0;
            });
            RegisterContextFn("GetVelocityY", (ctx, args) => {
                int handle = args.Length > 0 ? (int)args[0] : (ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0);
                var go = ctx.GetByHandle(handle);
                var rb = go?.GetComponent<Physics.Rigidbody>();
                return rb?.velocity.y ?? 0;
            });
            RegisterContextFn("GetVelocityZ", (ctx, args) => {
                int handle = args.Length > 0 ? (int)args[0] : (ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0);
                var go = ctx.GetByHandle(handle);
                var rb = go?.GetComponent<Physics.Rigidbody>();
                return rb?.velocity.z ?? 0;
            });

            RegisterContextFn("SetVelocity", (ctx, args) => {
                if (args.Length < 3) return 0;
                int handle = args.Length > 3 ? (int)args[3] : (ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0);
                var go = ctx.GetByHandle(handle);
                var rb = go?.GetComponent<Physics.Rigidbody>();
                if (rb != null) rb.velocity = new ECSVector3((float)args[0], (float)args[1], (float)args[2]);
                return 0;
            });

            RegisterContextFn("AddForce", (ctx, args) => {
                if (args.Length < 3) return 0;
                int handle = args.Length > 3 ? (int)args[3] : (ctx.gameObject != null ? ctx.GetHandle(ctx.gameObject) : 0);
                var go = ctx.GetByHandle(handle);
                var rb = go?.GetComponent<Physics.Rigidbody>();
                rb?.AddForce(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });

            // Self velocity shortcuts
            RegisterDynamicVar("velocity_x",
                ctx => ctx.gameObject?.GetComponent<Physics.Rigidbody>()?.velocity.x ?? 0,
                (ctx, v) => { var rb = ctx.gameObject?.GetComponent<Physics.Rigidbody>(); if (rb != null) { rb.velocity.x = (float)v; } });
            RegisterDynamicVar("velocity_y",
                ctx => ctx.gameObject?.GetComponent<Physics.Rigidbody>()?.velocity.y ?? 0,
                (ctx, v) => { var rb = ctx.gameObject?.GetComponent<Physics.Rigidbody>(); if (rb != null) { rb.velocity.y = (float)v; } });
            RegisterDynamicVar("velocity_z",
                ctx => ctx.gameObject?.GetComponent<Physics.Rigidbody>()?.velocity.z ?? 0,
                (ctx, v) => { var rb = ctx.gameObject?.GetComponent<Physics.Rigidbody>(); if (rb != null) { rb.velocity.z = (float)v; } });
        }

        void RegisterMathFunctions()
        {
            RegisterFn("sqrt", args => Math.Sqrt(args[0]));
            RegisterFn("abs", args => Math.Abs(args[0]));
            RegisterFn("sin", args => Math.Sin(args[0]));
            RegisterFn("cos", args => Math.Cos(args[0]));
            RegisterFn("tan", args => Math.Tan(args[0]));
            RegisterFn("asin", args => Math.Asin(args[0]));
            RegisterFn("acos", args => Math.Acos(args[0]));
            RegisterFn("atan", args => Math.Atan(args[0]));
            RegisterFn("atan2", args => Math.Atan2(args[0], args[1]));
            RegisterFn("pow", args => Math.Pow(args[0], args[1]));
            RegisterFn("floor", args => Math.Floor(args[0]));
            RegisterFn("ceil", args => Math.Ceiling(args[0]));
            RegisterFn("round", args => Math.Round(args[0]));
            RegisterFn("min", args => Math.Min(args[0], args[1]));
            RegisterFn("max", args => Math.Max(args[0], args[1]));
            RegisterFn("clamp", args => Math.Max(args[1], Math.Min(args[2], args[0])));
            RegisterFn("lerp", args => args[0] + (args[1] - args[0]) * args[2]);
            RegisterFn("sign", args => Math.Sign(args[0]));

            var rng = new Random();
            RegisterFn("random", _ => rng.NextDouble());
            RegisterFn("random_range", args => args[0] + rng.NextDouble() * (args[1] - args[0]));
            RegisterFn("random_int", args => rng.Next((int)args[0], (int)args[1]));
        }

        void RegisterInputFunctions()
        {
            RegisterFn("GetKey", args => Input.Input.GetKey((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0);
            RegisterFn("GetKeyDown", args => Input.Input.GetKeyDown((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0);
            RegisterFn("GetKeyUp", args => Input.Input.GetKeyUp((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0);
            RegisterFn("GetMouseButton", args => Input.Input.GetMouseButton((Input.MouseButton)(int)args[0]) ? 1.0 : 0.0);
            RegisterFn("GetMouseButtonDown", args => Input.Input.GetMouseButtonDown((Input.MouseButton)(int)args[0]) ? 1.0 : 0.0);
            RegisterFn("GetMouseButtonUp", args => Input.Input.GetMouseButtonUp((Input.MouseButton)(int)args[0]) ? 1.0 : 0.0);

            RegisterDynamicVar("horizontal", _ => Input.Input.GetAxisHorizontal(), null);
            RegisterDynamicVar("vertical", _ => Input.Input.GetAxisVertical(), null);
            RegisterDynamicVar("mouse_x", _ => Input.Input.MousePosition.X, null);
            RegisterDynamicVar("mouse_y", _ => Input.Input.MousePosition.Y, null);
            RegisterDynamicVar("mouse_delta_x", _ => Input.Input.MouseDelta.X, null);
            RegisterDynamicVar("mouse_delta_y", _ => Input.Input.MouseDelta.Y, null);
        }

        void RegisterTimeFunctions()
        {
            RegisterDynamicVar("delta_time", _ => Time.DeltaTime, null);
            RegisterDynamicVar("time", _ => Time.TotalTime, null);
            RegisterDynamicVar("unscaled_delta_time", _ => Time.UnscaledDeltaTime, null);
            RegisterDynamicVar("time_scale", _ => Time.TimeScale, (_, v) => Time.TimeScale = (float)v);
            RegisterDynamicVar("frame_count", _ => Time.FrameCount, null);
            RegisterDynamicVar("fps", _ => Time.FPS, null);
        }

        // String hash system for passing strings to scripts
        private Dictionary<int, string> _stringTable = new();
        private Dictionary<string, int> _stringHashes = new();
        private int _nextStringHash = 1;

        public int RegisterString(string s)
        {
            if (_stringHashes.TryGetValue(s, out int hash)) return hash;
            hash = _nextStringHash++;
            _stringTable[hash] = s;
            _stringHashes[s] = hash;
            return hash;
        }

        public string GetStringFromHash(int hash) => _stringTable.TryGetValue(hash, out var s) ? s : null;

        public void RegisterFn(string name, NativeFunction fn) => _functions[name] = fn;
        public void RegisterContextFn(string name, NativeFunctionWithContext fn) => _contextFunctions[name] = fn;
        public void RegisterDynamicVar(string name, Func<ScriptContext, double> getter, Action<ScriptContext, double> setter)
        {
            if (getter != null) _dynamicGetters[name] = getter;
            if (setter != null) _dynamicSetters[name] = setter;
        }

        public double CallFunction(string name, ScriptContext ctx, double[] args)
        {
            if (_contextFunctions.TryGetValue(name, out var cfn)) return cfn(ctx, args);
            if (_functions.TryGetValue(name, out var fn)) return fn(args);
            return 0;
        }

        public double GetVariable(string name, ScriptContext ctx)
        {
            if (_dynamicGetters.TryGetValue(name, out var getter)) return getter(ctx);
            return 0;
        }

        public void SetVariable(string name, ScriptContext ctx, double value)
        {
            if (_dynamicSetters.TryGetValue(name, out var setter)) setter(ctx, value);
        }

        public bool HasGetter(string name) => _dynamicGetters.ContainsKey(name);
        public bool HasSetter(string name) => _dynamicSetters.ContainsKey(name);
        public bool HasFunction(string name) => _functions.ContainsKey(name) || _contextFunctions.ContainsKey(name);
    }

    public class DynamicMagolorScript : ManagedScript
    {
        private ScriptContext _ctx;
        private DynamicScriptEngine _engine;

        public DynamicMagolorScript(DynamicScriptEngine engine) : base()
        {
            _engine = engine;
        }

        public void SetContext(ScriptContext ctx) => _ctx = ctx;

        public new void RegisterNative(string name, Func<double[], double> func)
        {
            base.RegisterNative(name, args => _engine.CallFunction(name, _ctx, args));
        }

        public void SyncFromEngine()
        {
            // Push dynamic variables from engine to script
            foreach (var name in GetVariableNames())
            {
                if (_engine.HasGetter(name))
                    SetFloat(name, _engine.GetVariable(name, _ctx));
            }
        }

        public void SyncToEngine()
        {
            // Pull dynamic variables from script back to engine
            foreach (var name in GetVariableNames())
            {
                if (_engine.HasSetter(name))
                    _engine.SetVariable(name, _ctx, GetFloat(name));
            }
        }

        private HashSet<string> _varNames = new();
        public void TrackVariable(string name) => _varNames.Add(name);
        public IEnumerable<string> GetVariableNames() => _varNames;
    }

    public class DynamicScriptComponent : MonoBehaviour
    {
        public string scriptPath;
        public string scriptSource;

        private static DynamicScriptEngine _sharedEngine;
        private DynamicMagolorScript _script;
        private ScriptContext _ctx;
        private bool _started;

        public static DynamicScriptEngine SharedEngine
        {
            get
            {
                if (_sharedEngine == null) _sharedEngine = new DynamicScriptEngine();
                return _sharedEngine;
            }
        }

        protected override void Awake()
        {
            if (!string.IsNullOrEmpty(scriptPath) && File.Exists(scriptPath))
                scriptSource = File.ReadAllText(scriptPath);

            if (!string.IsNullOrEmpty(scriptSource))
                InitializeScript();
        }

        private void InitializeScript()
        {
            _ctx = new ScriptContext
            {
                gameObject = this.gameObject,
                transform = this.transform,
                engine = SharedEngine
            };

            _script = new DynamicMagolorScript(SharedEngine);
            _script.SetContext(_ctx);

            // Register all engine functions
            RegisterEngineFunctions();

            try
            {
                _script.Parse(scriptSource);
                Console.WriteLine($"[DynamicScript] Loaded: {gameObject.name}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[DynamicScript] Parse error in {gameObject.name}: {ex.Message}");
            }
        }

        private void RegisterEngineFunctions()
        {
            var engine = SharedEngine;

            // Register all context functions as natives
            _script.RegisterNative("FindByName", args => {
                string name = GetStringArg(args, 0);
                var go = GameObject.Find(name);
                return go != null ? _ctx.GetHandle(go) : 0;
            });

            _script.RegisterNative("FindByTag", args => {
                string tag = GetStringArg(args, 0);
                var go = GameObject.FindWithTag(tag);
                return go != null ? _ctx.GetHandle(go) : 0;
            });

            _script.RegisterNative("GetEntityX", args => {
                var go = _ctx.GetByHandle((int)args[0]);
                return go?.transform.position.x ?? 0;
            });
            _script.RegisterNative("GetEntityY", args => {
                var go = _ctx.GetByHandle((int)args[0]);
                return go?.transform.position.y ?? 0;
            });
            _script.RegisterNative("GetEntityZ", args => {
                var go = _ctx.GetByHandle((int)args[0]);
                return go?.transform.position.z ?? 0;
            });

            _script.RegisterNative("SetEntityPos", args => {
                if (args.Length < 4) return 0;
                var go = _ctx.GetByHandle((int)args[0]);
                if (go != null) go.transform.position = new ECSVector3((float)args[1], (float)args[2], (float)args[3]);
                return 0;
            });

            _script.RegisterNative("DistanceTo", args => {
                var target = _ctx.GetByHandle((int)args[0]);
                if (target == null) return float.MaxValue;
                return Vector3Extensions.Distance(transform.position, target.transform.position);
            });

            _script.RegisterNative("DistanceBetween", args => {
                var a = _ctx.GetByHandle((int)args[0]);
                var b = _ctx.GetByHandle((int)args[1]);
                if (a == null || b == null) return float.MaxValue;
                return Vector3Extensions.Distance(a.transform.position, b.transform.position);
            });

            _script.RegisterNative("IsValidEntity", args => _ctx.GetByHandle((int)args[0]) != null ? 1.0 : 0.0);
            _script.RegisterNative("GetSelf", _ => _ctx.GetHandle(gameObject));
            _script.RegisterNative("DestroyEntity", args => { _ctx.GetByHandle((int)args[0])?.Destroy(); return 0; });
            _script.RegisterNative("DestroySelf", _ => { gameObject.Destroy(); return 0; });

            _script.RegisterNative("LookAtEntity", args => {
                var target = _ctx.GetByHandle((int)args[0]);
                if (target != null) transform.LookAt(target.transform.position);
                return 0;
            });

            _script.RegisterNative("MoveTowardsEntity", args => {
                var target = _ctx.GetByHandle((int)args[0]);
                if (target != null)
                    transform.position = Vector3Extensions.MoveTowards(transform.position, target.transform.position, (float)args[1]);
                return 0;
            });

            _script.RegisterNative("IsEntityActive", args => _ctx.GetByHandle((int)args[0])?.activeSelf == true ? 1.0 : 0.0);
            _script.RegisterNative("SetEntityActive", args => {
                var go = _ctx.GetByHandle((int)args[0]);
                if (go != null) go.activeSelf = args[1] > 0.5;
                return 0;
            });

            // Transform
            _script.RegisterNative("Translate", args => {
                transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), true);
                return 0;
            });
            _script.RegisterNative("TranslateWorld", args => {
                transform.Translate(new ECSVector3((float)args[0], (float)args[1], (float)args[2]), false);
                return 0;
            });
            _script.RegisterNative("Rotate", args => {
                transform.Rotate((float)args[0], (float)args[1], (float)args[2]);
                return 0;
            });
            _script.RegisterNative("LookAt", args => {
                transform.LookAt(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });

            // Physics
            _script.RegisterNative("AddForce", args => {
                var rb = gameObject.GetComponent<Physics.Rigidbody>();
                rb?.AddForce(new ECSVector3((float)args[0], (float)args[1], (float)args[2]));
                return 0;
            });
            _script.RegisterNative("SetVelocity", args => {
                var rb = gameObject.GetComponent<Physics.Rigidbody>();
                if (rb != null) rb.velocity = new ECSVector3((float)args[0], (float)args[1], (float)args[2]);
                return 0;
            });

            // Input
            _script.RegisterNative("GetKey", args => Input.Input.GetKey((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0);
            _script.RegisterNative("GetKeyDown", args => Input.Input.GetKeyDown((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0);
            _script.RegisterNative("GetKeyUp", args => Input.Input.GetKeyUp((Input.KeyCode)(int)args[0]) ? 1.0 : 0.0);
            _script.RegisterNative("GetMouseButton", args => Input.Input.GetMouseButton((Input.MouseButton)(int)args[0]) ? 1.0 : 0.0);

            // Math
            _script.RegisterNative("sqrt", args => Math.Sqrt(args[0]));
            _script.RegisterNative("abs", args => Math.Abs(args[0]));
            _script.RegisterNative("sin", args => Math.Sin(args[0]));
            _script.RegisterNative("cos", args => Math.Cos(args[0]));
            _script.RegisterNative("tan", args => Math.Tan(args[0]));
            _script.RegisterNative("atan2", args => Math.Atan2(args[0], args[1]));
            _script.RegisterNative("pow", args => Math.Pow(args[0], args[1]));
            _script.RegisterNative("floor", args => Math.Floor(args[0]));
            _script.RegisterNative("ceil", args => Math.Ceiling(args[0]));
            _script.RegisterNative("round", args => Math.Round(args[0]));
            _script.RegisterNative("min", args => Math.Min(args[0], args[1]));
            _script.RegisterNative("max", args => Math.Max(args[0], args[1]));
            _script.RegisterNative("clamp", args => Math.Max(args[1], Math.Min(args[2], args[0])));
            _script.RegisterNative("lerp", args => args[0] + (args[1] - args[0]) * args[2]);
            _script.RegisterNative("sign", args => Math.Sign(args[0]));

            var rng = new Random();
            _script.RegisterNative("random", _ => rng.NextDouble());
            _script.RegisterNative("random_range", args => args[0] + rng.NextDouble() * (args[1] - args[0]));
            _script.RegisterNative("random_int", args => rng.Next((int)args[0], (int)args[1]));

            // Logging
            _script.RegisterNative("Log", args => { Console.WriteLine($"[{gameObject.name}] {args[0]}"); return 0; });
            _script.RegisterNative("LogWarning", args => { Console.WriteLine($"[WARNING - {gameObject.name}] {args[0]}"); return 0; });
            _script.RegisterNative("LogError", args => { Console.WriteLine($"[ERROR - {gameObject.name}] {args[0]}"); return 0; });
        }

        // String table for script
        private Dictionary<string, int> _stringToId = new();
        private Dictionary<int, string> _idToString = new();
        private int _nextStrId = 1;

        public int GetStringId(string s)
        {
            if (_stringToId.TryGetValue(s, out int id)) return id;
            id = _nextStrId++;
            _stringToId[s] = id;
            _idToString[id] = s;
            return id;
        }

        private string GetStringArg(double[] args, int index)
        {
            if (args.Length <= index) return "";
            int id = (int)args[index];
            return _idToString.TryGetValue(id, out var s) ? s : "";
        }

        protected override void Start()
        {
            if (_script != null && !_started)
            {
                _started = true;
                PushVariables();
                _script.CallIfExists("OnStart");
                if (_script.CallIfExists("OnStart") == 0)
                    _script.CallIfExists("on_start");
                PullVariables();
            }
        }

        protected override void Update()
        {
            if (_script == null || !enabled) return;

            PushVariables();
            _script.CallIfExists("OnUpdate");
            if (_script.CallIfExists("OnUpdate") == 0)
                _script.CallIfExists("on_update");
            PullVariables();
        }

        protected override void LateUpdate()
        {
            if (_script == null || !enabled) return;
            _script.CallIfExists("OnLateUpdate");
            if (_script.CallIfExists("OnLateUpdate") == 0)
                _script.CallIfExists("on_late_update");
        }

        protected override void OnDestroy()
        {
            if (_script != null)
            {
                _script.CallIfExists("OnDestroy");
                if (_script.CallIfExists("OnDestroy") == 0)
                    _script.CallIfExists("on_destroy");
            }
        }

        private void PushVariables()
        {
            // Push dynamic vars
            _script.SetFloat("delta_time", Time.DeltaTime);
            _script.SetFloat("time", Time.TotalTime);
            _script.SetFloat("unscaled_delta_time", Time.UnscaledDeltaTime);
            _script.SetFloat("time_scale", Time.TimeScale);

            _script.SetFloat("position_x", transform.position.x);
            _script.SetFloat("position_y", transform.position.y);
            _script.SetFloat("position_z", transform.position.z);

            var euler = transform.GetEulerAngles();
            _script.SetFloat("rotation_x", euler.x);
            _script.SetFloat("rotation_y", euler.y);
            _script.SetFloat("rotation_z", euler.z);

            _script.SetFloat("scale_x", transform.localScale.x);
            _script.SetFloat("scale_y", transform.localScale.y);
            _script.SetFloat("scale_z", transform.localScale.z);

            _script.SetFloat("horizontal", Input.Input.GetAxisHorizontal());
            _script.SetFloat("vertical", Input.Input.GetAxisVertical());
            _script.SetFloat("mouse_x", Input.Input.MousePosition.X);
            _script.SetFloat("mouse_y", Input.Input.MousePosition.Y);
            _script.SetFloat("mouse_delta_x", Input.Input.MouseDelta.X);
            _script.SetFloat("mouse_delta_y", Input.Input.MouseDelta.Y);

            var rb = gameObject.GetComponent<Physics.Rigidbody>();
            if (rb != null)
            {
                _script.SetFloat("velocity_x", rb.velocity.x);
                _script.SetFloat("velocity_y", rb.velocity.y);
                _script.SetFloat("velocity_z", rb.velocity.z);
            }

            _script.SetFloat("active", gameObject.activeSelf ? 1.0 : 0.0);
        }

        private void PullVariables()
        {
            // Pull position back
            transform.position = new ECSVector3(
                (float)_script.GetFloat("position_x"),
                (float)_script.GetFloat("position_y"),
                (float)_script.GetFloat("position_z")
            );

            // Pull rotation back
            transform.SetEulerAngles(
                (float)_script.GetFloat("rotation_x"),
                (float)_script.GetFloat("rotation_y"),
                (float)_script.GetFloat("rotation_z")
            );

            // Pull scale back
            transform.localScale = new ECSVector3(
                (float)_script.GetFloat("scale_x"),
                (float)_script.GetFloat("scale_y"),
                (float)_script.GetFloat("scale_z")
            );

            var rb = gameObject.GetComponent<Physics.Rigidbody>();
            if (rb != null)
            {
                rb.velocity = new ECSVector3(
                    (float)_script.GetFloat("velocity_x"),
                    (float)_script.GetFloat("velocity_y"),
                    (float)_script.GetFloat("velocity_z")
                );
            }

            gameObject.activeSelf = _script.GetFloat("active") > 0.5;
            Time.TimeScale = (float)_script.GetFloat("time_scale");
        }

        public static DynamicScriptComponent CreateFromFile(GameObject go, string path)
        {
            var comp = go.AddComponent<DynamicScriptComponent>();
            comp.scriptPath = path;
            return comp;
        }

        public static DynamicScriptComponent CreateFromSource(GameObject go, string source)
        {
            var comp = go.AddComponent<DynamicScriptComponent>();
            comp.scriptSource = source;
            return comp;
        }

        public DynamicMagolorScript GetScript() => _script;
        public ScriptContext GetContext() => _ctx;
    }
}
