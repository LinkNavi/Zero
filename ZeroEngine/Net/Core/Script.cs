using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using EngineCore.ECS;

namespace EngineCore.Scripting
{
    /// <summary>
    /// Native Magolor script engine bindings
    /// </summary>
    internal static class MagolorNative
    {
        private const string DllName = "magolor";

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr script_engine_new();

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void script_engine_free(IntPtr engine);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int script_engine_compile(IntPtr engine, 
            [MarshalAs(UnmanagedType.LPStr)] string name,
            [MarshalAs(UnmanagedType.LPStr)] string source);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern long script_engine_run(IntPtr engine,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void script_engine_set_int(IntPtr engine,
            [MarshalAs(UnmanagedType.LPStr)] string name, long value);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void script_engine_set_float(IntPtr engine,
            [MarshalAs(UnmanagedType.LPStr)] string name, double value);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern long script_engine_get_int(IntPtr engine,
            [MarshalAs(UnmanagedType.LPStr)] string name);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern double script_engine_get_float(IntPtr engine,
            [MarshalAs(UnmanagedType.LPStr)] string name);
    }

    /// <summary>
    /// Managed wrapper for the Magolor script engine
    /// </summary>
    public class ScriptEngine : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;
        private Dictionary<string, string> _loadedScripts = new Dictionary<string, string>();

        public bool IsInitialized => _handle != IntPtr.Zero;

        public ScriptEngine()
        {
            _handle = MagolorNative.script_engine_new();
            if (_handle == IntPtr.Zero)
            {
                throw new Exception("Failed to create script engine");
            }
            Console.WriteLine("Magolor script engine initialized");
        }

        /// <summary>
        /// Compile a script from source code
        /// </summary>
        public bool Compile(string name, string source)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            
            int result = MagolorNative.script_engine_compile(_handle, name, source);
            if (result == 0)
            {
                _loadedScripts[name] = source;
                Console.WriteLine($"Compiled script: {name}");
                return true;
            }
            Console.WriteLine($"Failed to compile script: {name}");
            return false;
        }

        /// <summary>
        /// Load and compile a script from file
        /// </summary>
        public bool LoadFile(string path)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            
            if (!File.Exists(path))
            {
                Console.WriteLine($"Script file not found: {path}");
                return false;
            }

            string source = File.ReadAllText(path);
            string name = Path.GetFileNameWithoutExtension(path);
            return Compile(name, source);
        }

        /// <summary>
        /// Run a compiled script
        /// </summary>
        public long Run(string name)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            return MagolorNative.script_engine_run(_handle, name);
        }

        /// <summary>
        /// Set an integer global variable
        /// </summary>
        public void SetInt(string name, long value)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            MagolorNative.script_engine_set_int(_handle, name, value);
        }

        /// <summary>
        /// Set a float global variable
        /// </summary>
        public void SetFloat(string name, double value)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            MagolorNative.script_engine_set_float(_handle, name, value);
        }

        /// <summary>
        /// Get an integer global variable
        /// </summary>
        public long GetInt(string name)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            return MagolorNative.script_engine_get_int(_handle, name);
        }

        /// <summary>
        /// Get a float global variable
        /// </summary>
        public double GetFloat(string name)
        {
            if (_disposed) throw new ObjectDisposedException(nameof(ScriptEngine));
            return MagolorNative.script_engine_get_float(_handle, name);
        }

        public void Dispose()
        {
            if (!_disposed && _handle != IntPtr.Zero)
            {
                MagolorNative.script_engine_free(_handle);
                _handle = IntPtr.Zero;
                _disposed = true;
                Console.WriteLine("Script engine disposed");
            }
        }
    }

    /// <summary>
    /// Script component for attaching scripts to GameObjects
    /// Uses managed interpretation when native library unavailable
    /// </summary>
    public class ScriptBehaviour : MonoBehaviour
    {
        public string scriptPath;
        public string scriptSource;
        
        private static ManagedScriptEngine _sharedEngine;
        private ManagedScript _script;
        private bool _initialized;

        public static ManagedScriptEngine SharedEngine
        {
            get
            {
                if (_sharedEngine == null)
                {
                    _sharedEngine = new ManagedScriptEngine();
                }
                return _sharedEngine;
            }
        }

        protected override void Awake()
        {
            if (!string.IsNullOrEmpty(scriptPath) && File.Exists(scriptPath))
            {
                scriptSource = File.ReadAllText(scriptPath);
            }

            if (!string.IsNullOrEmpty(scriptSource))
            {
                _script = SharedEngine.Compile(scriptSource);
                if (_script != null)
                {
                    // Bind engine callbacks
                    BindEngineCallbacks();
                    _initialized = true;
                }
            }
        }

        protected override void Start()
        {
            if (_initialized && _script != null)
            {
                _script.CallIfExists("on_start");
            }
        }

        protected override void Update()
        {
            if (_initialized && _script != null)
            {
                // Pass delta time to script
                _script.SetFloat("delta_time", Time.DeltaTime);
                _script.SetFloat("total_time", Time.TotalTime);
                
                // Pass transform data
                _script.SetFloat("pos_x", transform.position.x);
                _script.SetFloat("pos_y", transform.position.y);
                _script.SetFloat("pos_z", transform.position.z);
                
                _script.CallIfExists("on_update");
                
                // Read back transform changes
                float newX = (float)_script.GetFloat("pos_x");
                float newY = (float)_script.GetFloat("pos_y");
                float newZ = (float)_script.GetFloat("pos_z");
                transform.position = new Vector3(newX, newY, newZ);
            }
        }

        protected override void OnDestroy()
        {
            if (_initialized && _script != null)
            {
                _script.CallIfExists("on_destroy");
            }
        }

        private void BindEngineCallbacks()
        {
            // Register engine functions the script can call
            _script.RegisterNative("log", args =>
            {
                if (args.Length > 0)
                    Console.WriteLine($"[Script] {args[0]}");
                return 0.0;
            });

            _script.RegisterNative("get_key", args =>
            {
                // Would integrate with Input system
                return 0.0;
            });

            _script.RegisterNative("spawn_object", args =>
            {
                // Would integrate with Scene system
                return 0.0;
            });
        }
    }

    /// <summary>
    /// Pure C# script interpreter (fallback when native unavailable)
    /// </summary>
    public class ManagedScriptEngine
    {
        private Dictionary<string, ManagedScript> _scripts = new Dictionary<string, ManagedScript>();

        public ManagedScript Compile(string source)
        {
            try
            {
                var script = new ManagedScript();
                script.Parse(source);
                return script;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Script compilation error: {ex.Message}");
                return null;
            }
        }

        public ManagedScript LoadFile(string path)
        {
            if (!File.Exists(path)) return null;
            string source = File.ReadAllText(path);
            var script = Compile(source);
            if (script != null)
            {
                _scripts[Path.GetFileNameWithoutExtension(path)] = script;
            }
            return script;
        }
    }

    /// <summary>
    /// Represents a compiled script with variables and functions
    /// </summary>
    public class ManagedScript
    {
        private Dictionary<string, double> _floats = new Dictionary<string, double>();
        private Dictionary<string, long> _ints = new Dictionary<string, long>();
        private Dictionary<string, string> _strings = new Dictionary<string, string>();
        private Dictionary<string, Func<double[], double>> _natives = new Dictionary<string, Func<double[], double>>();
        private Dictionary<string, ScriptFunction> _functions = new Dictionary<string, ScriptFunction>();
        
        private string _source;
        private List<Token> _tokens;
        private int _pos;

        // Simple token types for parsing
        private enum TokenType { Ident, Number, String, Symbol, Keyword, EOF }
        private class Token { public TokenType Type; public string Value; public double NumValue; }
        
        private class ScriptFunction
        {
            public string Name;
            public List<string> Params = new List<string>();
            public List<Statement> Body = new List<Statement>();
        }
        
        private abstract class Statement { public abstract void Execute(ManagedScript ctx); }
        
        private class AssignStmt : Statement
        {
            public string Var;
            public Expression Value;
            public override void Execute(ManagedScript ctx)
            {
                ctx._floats[Var] = Value.Eval(ctx);
            }
        }
        
        private class IfStmt : Statement
        {
            public Expression Cond;
            public List<Statement> Then = new List<Statement>();
            public List<Statement> Else = new List<Statement>();
            public override void Execute(ManagedScript ctx)
            {
                if (Cond.Eval(ctx) != 0)
                    foreach (var s in Then) s.Execute(ctx);
                else
                    foreach (var s in Else) s.Execute(ctx);
            }
        }
        
        private class WhileStmt : Statement
        {
            public Expression Cond;
            public List<Statement> Body = new List<Statement>();
            public override void Execute(ManagedScript ctx)
            {
                while (Cond.Eval(ctx) != 0)
                    foreach (var s in Body) s.Execute(ctx);
            }
        }
        
        private class CallStmt : Statement
        {
            public string Func;
            public List<Expression> Args = new List<Expression>();
            public override void Execute(ManagedScript ctx)
            {
                double[] args = new double[Args.Count];
                for (int i = 0; i < Args.Count; i++)
                    args[i] = Args[i].Eval(ctx);
                
                if (ctx._natives.TryGetValue(Func, out var native))
                    native(args);
                else if (ctx._functions.TryGetValue(Func, out var func))
                    ctx.CallFunction(func, args);
            }
        }
        
        private class ReturnStmt : Statement
        {
            public Expression Value;
            public override void Execute(ManagedScript ctx)
            {
                ctx._returnValue = Value?.Eval(ctx) ?? 0;
                ctx._returning = true;
            }
        }
        
        private abstract class Expression { public abstract double Eval(ManagedScript ctx); }
        
        private class NumExpr : Expression
        {
            public double Value;
            public override double Eval(ManagedScript ctx) => Value;
        }
        
        private class VarExpr : Expression
        {
            public string Name;
            public override double Eval(ManagedScript ctx)
            {
                if (ctx._floats.TryGetValue(Name, out var f)) return f;
                if (ctx._ints.TryGetValue(Name, out var i)) return i;
                return 0;
            }
        }
        
        private class BinaryExpr : Expression
        {
            public string Op;
            public Expression Left, Right;
            public override double Eval(ManagedScript ctx)
            {
                double l = Left.Eval(ctx), r = Right.Eval(ctx);
                return Op switch
                {
                    "+" => l + r, "-" => l - r, "*" => l * r, "/" => r != 0 ? l / r : 0,
                    "%" => r != 0 ? l % r : 0,
                    "<" => l < r ? 1 : 0, ">" => l > r ? 1 : 0,
                    "<=" => l <= r ? 1 : 0, ">=" => l >= r ? 1 : 0,
                    "==" => l == r ? 1 : 0, "!=" => l != r ? 1 : 0,
                    "&&" => (l != 0 && r != 0) ? 1 : 0,
                    "||" => (l != 0 || r != 0) ? 1 : 0,
                    _ => 0
                };
            }
        }
        
        private class CallExpr : Expression
        {
            public string Func;
            public List<Expression> Args = new List<Expression>();
            public override double Eval(ManagedScript ctx)
            {
                double[] args = new double[Args.Count];
                for (int i = 0; i < Args.Count; i++)
                    args[i] = Args[i].Eval(ctx);
                
                if (ctx._natives.TryGetValue(Func, out var native))
                    return native(args);
                if (ctx._functions.TryGetValue(Func, out var func))
                    return ctx.CallFunction(func, args);
                
                // Built-in math
                return Func switch
                {
                    "sqrt" => Math.Sqrt(args[0]),
                    "abs" => Math.Abs(args[0]),
                    "sin" => Math.Sin(args[0]),
                    "cos" => Math.Cos(args[0]),
                    "floor" => Math.Floor(args[0]),
                    "ceil" => Math.Ceiling(args[0]),
                    "min" => Math.Min(args[0], args[1]),
                    "max" => Math.Max(args[0], args[1]),
                    "clamp" => Math.Max(args[1], Math.Min(args[2], args[0])),
                    "lerp" => args[0] + (args[1] - args[0]) * args[2],
                    _ => 0
                };
            }
        }
        
        private double _returnValue;
        private bool _returning;

        public void Parse(string source)
        {
            _source = source;
            _tokens = Tokenize(source);
            _pos = 0;
            
            while (_pos < _tokens.Count && _tokens[_pos].Type != TokenType.EOF)
            {
                ParseTopLevel();
            }
        }

        private List<Token> Tokenize(string src)
        {
            var tokens = new List<Token>();
            int i = 0;
            
            while (i < src.Length)
            {
                // Skip whitespace and comments
                while (i < src.Length && char.IsWhiteSpace(src[i])) i++;
                if (i >= src.Length) break;
                
                if (i + 1 < src.Length && src[i] == '/' && src[i + 1] == '/')
                {
                    while (i < src.Length && src[i] != '\n') i++;
                    continue;
                }
                
                char c = src[i];
                
                // Number
                if (char.IsDigit(c) || (c == '-' && i + 1 < src.Length && char.IsDigit(src[i + 1])))
                {
                    int start = i;
                    if (c == '-') i++;
                    while (i < src.Length && (char.IsDigit(src[i]) || src[i] == '.')) i++;
                    string numStr = src.Substring(start, i - start);
                    tokens.Add(new Token { Type = TokenType.Number, Value = numStr, NumValue = double.Parse(numStr) });
                    continue;
                }
                
                // Identifier/Keyword
                if (char.IsLetter(c) || c == '_')
                {
                    int start = i;
                    while (i < src.Length && (char.IsLetterOrDigit(src[i]) || src[i] == '_')) i++;
                    string word = src.Substring(start, i - start);
                    var kws = new[] { "fn", "let", "if", "else", "elif", "while", "for", "return", "true", "false", "i32", "f32", "bool", "void" };
                    tokens.Add(new Token { Type = Array.IndexOf(kws, word) >= 0 ? TokenType.Keyword : TokenType.Ident, Value = word });
                    continue;
                }
                
                // String
                if (c == '"')
                {
                    i++;
                    int start = i;
                    while (i < src.Length && src[i] != '"') i++;
                    tokens.Add(new Token { Type = TokenType.String, Value = src.Substring(start, i - start) });
                    i++;
                    continue;
                }
                
                // Multi-char symbols
                if (i + 1 < src.Length)
                {
                    string two = src.Substring(i, 2);
                    if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||" || two == "+=" || two == "-=")
                    {
                        tokens.Add(new Token { Type = TokenType.Symbol, Value = two });
                        i += 2;
                        continue;
                    }
                }
                
                // Single-char symbol
                tokens.Add(new Token { Type = TokenType.Symbol, Value = c.ToString() });
                i++;
            }
            
            tokens.Add(new Token { Type = TokenType.EOF });
            return tokens;
        }

        private Token Peek() => _pos < _tokens.Count ? _tokens[_pos] : new Token { Type = TokenType.EOF };
        private Token Advance() => _pos < _tokens.Count ? _tokens[_pos++] : new Token { Type = TokenType.EOF };
        private bool Match(string val) { if (Peek().Value == val) { Advance(); return true; } return false; }
        private void Expect(string val) { if (!Match(val)) throw new Exception($"Expected '{val}'"); }

        private void ParseTopLevel()
        {
            var tok = Peek();
            
            // Skip type specifiers
            if (tok.Type == TokenType.Keyword && (tok.Value == "i32" || tok.Value == "f32" || tok.Value == "bool" || tok.Value == "void"))
            {
                Advance();
                tok = Peek();
            }
            
            if (tok.Value == "fn")
            {
                Advance();
                var func = new ScriptFunction { Name = Advance().Value };
                Expect("(");
                while (!Match(")"))
                {
                    // Skip type
                    if (Peek().Type == TokenType.Keyword) Advance();
                    func.Params.Add(Advance().Value);
                    Match(",");
                }
                Expect("{");
                while (!Match("}"))
                {
                    func.Body.Add(ParseStatement());
                }
                _functions[func.Name] = func;
            }
            else if (tok.Value == "let")
            {
                Advance();
                // Skip type
                if (Peek().Type == TokenType.Keyword) Advance();
                string name = Advance().Value;
                if (Match("="))
                {
                    _floats[name] = ParseExpr().Eval(this);
                }
                Expect(";");
            }
            else
            {
                Advance(); // Skip unknown
            }
        }

        private Statement ParseStatement()
        {
            var tok = Peek();
            
            if (tok.Value == "let")
            {
                Advance();
                if (Peek().Type == TokenType.Keyword) Advance(); // Skip type
                string name = Advance().Value;
                Expression val = null;
                if (Match("=")) val = ParseExpr();
                Expect(";");
                return new AssignStmt { Var = name, Value = val ?? new NumExpr { Value = 0 } };
            }
            
            if (tok.Value == "if")
            {
                Advance();
                Expect("(");
                var cond = ParseExpr();
                Expect(")");
                Expect("{");
                var stmt = new IfStmt { Cond = cond };
                while (!Match("}")) stmt.Then.Add(ParseStatement());
                if (Match("else"))
                {
                    Expect("{");
                    while (!Match("}")) stmt.Else.Add(ParseStatement());
                }
                return stmt;
            }
            
            if (tok.Value == "while")
            {
                Advance();
                Expect("(");
                var cond = ParseExpr();
                Expect(")");
                Expect("{");
                var stmt = new WhileStmt { Cond = cond };
                while (!Match("}")) stmt.Body.Add(ParseStatement());
                return stmt;
            }
            
            if (tok.Value == "return")
            {
                Advance();
                Expression val = null;
                if (Peek().Value != ";") val = ParseExpr();
                Expect(";");
                return new ReturnStmt { Value = val };
            }
            
            // Assignment or function call
            if (tok.Type == TokenType.Ident)
            {
                string name = Advance().Value;
                
                if (Match("("))
                {
                    var call = new CallStmt { Func = name };
                    while (!Match(")"))
                    {
                        call.Args.Add(ParseExpr());
                        Match(",");
                    }
                    Expect(";");
                    return call;
                }
                
                if (Match("="))
                {
                    var val = ParseExpr();
                    Expect(";");
                    return new AssignStmt { Var = name, Value = val };
                }
                
                if (Match("+="))
                {
                    var val = ParseExpr();
                    Expect(";");
                    return new AssignStmt { Var = name, Value = new BinaryExpr { Op = "+", Left = new VarExpr { Name = name }, Right = val } };
                }
                
                if (Match("-="))
                {
                    var val = ParseExpr();
                    Expect(";");
                    return new AssignStmt { Var = name, Value = new BinaryExpr { Op = "-", Left = new VarExpr { Name = name }, Right = val } };
                }
            }
            
            Advance(); // Skip unknown
            return new AssignStmt { Var = "_", Value = new NumExpr { Value = 0 } };
        }

        private Expression ParseExpr() => ParseOr();
        
        private Expression ParseOr()
        {
            var left = ParseAnd();
            while (Match("||"))
                left = new BinaryExpr { Op = "||", Left = left, Right = ParseAnd() };
            return left;
        }
        
        private Expression ParseAnd()
        {
            var left = ParseEquality();
            while (Match("&&"))
                left = new BinaryExpr { Op = "&&", Left = left, Right = ParseEquality() };
            return left;
        }
        
        private Expression ParseEquality()
        {
            var left = ParseComparison();
            while (true)
            {
                if (Match("==")) left = new BinaryExpr { Op = "==", Left = left, Right = ParseComparison() };
                else if (Match("!=")) left = new BinaryExpr { Op = "!=", Left = left, Right = ParseComparison() };
                else break;
            }
            return left;
        }
        
        private Expression ParseComparison()
        {
            var left = ParseAdditive();
            while (true)
            {
                if (Match("<")) left = new BinaryExpr { Op = "<", Left = left, Right = ParseAdditive() };
                else if (Match(">")) left = new BinaryExpr { Op = ">", Left = left, Right = ParseAdditive() };
                else if (Match("<=")) left = new BinaryExpr { Op = "<=", Left = left, Right = ParseAdditive() };
                else if (Match(">=")) left = new BinaryExpr { Op = ">=", Left = left, Right = ParseAdditive() };
                else break;
            }
            return left;
        }
        
        private Expression ParseAdditive()
        {
            var left = ParseMultiplicative();
            while (true)
            {
                if (Match("+")) left = new BinaryExpr { Op = "+", Left = left, Right = ParseMultiplicative() };
                else if (Match("-")) left = new BinaryExpr { Op = "-", Left = left, Right = ParseMultiplicative() };
                else break;
            }
            return left;
        }
        
        private Expression ParseMultiplicative()
        {
            var left = ParseUnary();
            while (true)
            {
                if (Match("*")) left = new BinaryExpr { Op = "*", Left = left, Right = ParseUnary() };
                else if (Match("/")) left = new BinaryExpr { Op = "/", Left = left, Right = ParseUnary() };
                else if (Match("%")) left = new BinaryExpr { Op = "%", Left = left, Right = ParseUnary() };
                else break;
            }
            return left;
        }
        
        private Expression ParseUnary()
        {
            if (Match("-"))
                return new BinaryExpr { Op = "*", Left = new NumExpr { Value = -1 }, Right = ParseUnary() };
            if (Match("!"))
                return new BinaryExpr { Op = "==", Left = ParseUnary(), Right = new NumExpr { Value = 0 } };
            return ParsePrimary();
        }
        
        private Expression ParsePrimary()
        {
            var tok = Advance();
            
            if (tok.Type == TokenType.Number)
                return new NumExpr { Value = tok.NumValue };
            
            if (tok.Value == "true")
                return new NumExpr { Value = 1 };
            
            if (tok.Value == "false")
                return new NumExpr { Value = 0 };
            
            if (tok.Type == TokenType.Ident)
            {
                if (Match("("))
                {
                    var call = new CallExpr { Func = tok.Value };
                    while (!Match(")"))
                    {
                        call.Args.Add(ParseExpr());
                        Match(",");
                    }
                    return call;
                }
                return new VarExpr { Name = tok.Value };
            }
            
            if (tok.Value == "(")
            {
                var expr = ParseExpr();
                Expect(")");
                return expr;
            }
            
            return new NumExpr { Value = 0 };
        }

        private double CallFunction(ScriptFunction func, double[] args)
        {
            var oldVars = new Dictionary<string, double>(_floats);
            
            for (int i = 0; i < func.Params.Count && i < args.Length; i++)
                _floats[func.Params[i]] = args[i];
            
            _returning = false;
            _returnValue = 0;
            
            foreach (var stmt in func.Body)
            {
                stmt.Execute(this);
                if (_returning) break;
            }
            
            _floats = oldVars;
            return _returnValue;
        }

        public void RegisterNative(string name, Func<double[], double> func)
        {
            _natives[name] = func;
        }

        public void SetFloat(string name, double value) => _floats[name] = value;
        public void SetInt(string name, long value) => _ints[name] = value;
        public double GetFloat(string name) => _floats.TryGetValue(name, out var v) ? v : 0;
        public long GetInt(string name) => _ints.TryGetValue(name, out var v) ? v : 0;

        public double CallIfExists(string funcName)
        {
            if (_functions.TryGetValue(funcName, out var func))
                return CallFunction(func, Array.Empty<double>());
            return 0;
        }
    }
}
