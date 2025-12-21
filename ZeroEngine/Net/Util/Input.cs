using System;
using System.Collections.Generic;
using OpenTK.Windowing.GraphicsLibraryFramework;
using OpenTK.Mathematics;

namespace EngineCore.Input
{
    /// <summary>
    /// KeyCode enum for all keyboard keys
    /// </summary>
    public enum KeyCode
    {
        // Letters
        A = Keys.A, B = Keys.B, C = Keys.C, D = Keys.D, E = Keys.E,
        F = Keys.F, G = Keys.G, H = Keys.H, I = Keys.I, J = Keys.J,
        K = Keys.K, L = Keys.L, M = Keys.M, N = Keys.N, O = Keys.O,
        P = Keys.P, Q = Keys.Q, R = Keys.R, S = Keys.S, T = Keys.T,
        U = Keys.U, V = Keys.V, W = Keys.W, X = Keys.X, Y = Keys.Y, Z = Keys.Z,
        
        // Numbers
        Alpha0 = Keys.D0, Alpha1 = Keys.D1, Alpha2 = Keys.D2, Alpha3 = Keys.D3,
        Alpha4 = Keys.D4, Alpha5 = Keys.D5, Alpha6 = Keys.D6, Alpha7 = Keys.D7,
        Alpha8 = Keys.D8, Alpha9 = Keys.D9,
        
        // Keypad
        Keypad0 = Keys.KeyPad0, Keypad1 = Keys.KeyPad1, Keypad2 = Keys.KeyPad2,
        Keypad3 = Keys.KeyPad3, Keypad4 = Keys.KeyPad4, Keypad5 = Keys.KeyPad5,
        Keypad6 = Keys.KeyPad6, Keypad7 = Keys.KeyPad7, Keypad8 = Keys.KeyPad8,
        Keypad9 = Keys.KeyPad9,
        
        // Function keys
        F1 = Keys.F1, F2 = Keys.F2, F3 = Keys.F3, F4 = Keys.F4,
        F5 = Keys.F5, F6 = Keys.F6, F7 = Keys.F7, F8 = Keys.F8,
        F9 = Keys.F9, F10 = Keys.F10, F11 = Keys.F11, F12 = Keys.F12,
        
        // Special keys
        Space = Keys.Space,
        Enter = Keys.Enter,
        Escape = Keys.Escape,
        Backspace = Keys.Backspace,
        Tab = Keys.Tab,
        LeftShift = Keys.LeftShift,
        RightShift = Keys.RightShift,
        LeftControl = Keys.LeftControl,
        RightControl = Keys.RightControl,
        LeftAlt = Keys.LeftAlt,
        RightAlt = Keys.RightAlt,
        CapsLock = Keys.CapsLock,
        
        // Arrow keys
        UpArrow = Keys.Up,
        DownArrow = Keys.Down,
        LeftArrow = Keys.Left,
        RightArrow = Keys.Right,
        
        // Other
        Delete = Keys.Delete,
        Home = Keys.Home,
        End = Keys.End,
        PageUp = Keys.PageUp,
        PageDown = Keys.PageDown,
        Insert = Keys.Insert,
        
        // Symbols
        Minus = Keys.Minus,
        Plus = Keys.Equal,
        LeftBracket = Keys.LeftBracket,
        RightBracket = Keys.RightBracket,
        Semicolon = Keys.Semicolon,
        Quote = Keys.Apostrophe,
        Comma = Keys.Comma,
        Period = Keys.Period,
        Slash = Keys.Slash,
        Backslash = Keys.Backslash,
        Grave = Keys.GraveAccent
    }

    /// <summary>
    /// Mouse button indices
    /// </summary>
    public enum MouseButton
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4
    }

    /// <summary>
    /// Main Input system - Unity-style input handling
    /// </summary>
    public static class Input
    {
        private static KeyboardState _currentKeyboard;
        private static KeyboardState _previousKeyboard;
        
        private static MouseState _currentMouse;
        private static MouseState _previousMouse;
        
        private static Vector2 _mousePosition;
        private static Vector2 _previousMousePosition;
        private static Vector2 _mouseDelta;
        private static Vector2 _mouseScroll;
        
        // Gamepad support
        private static readonly Dictionary<int, GamepadState> _currentGamepads = new Dictionary<int, GamepadState>();
        private static readonly Dictionary<int, GamepadState> _previousGamepads = new Dictionary<int, GamepadState>();

        // ==================== INITIALIZATION ====================

        /// <summary>
        /// Initialize the input system (call once at startup)
        /// </summary>
        public static void Initialize()
        {
            _mousePosition = Vector2.Zero;
            _previousMousePosition = Vector2.Zero;
            _mouseDelta = Vector2.Zero;
            _mouseScroll = Vector2.Zero;
        }

        /// <summary>
        /// Update input state (call at the beginning of each frame)
        /// </summary>
        public static void Update(KeyboardState keyboard, MouseState mouse)
        {
            // Store previous states
            _previousKeyboard = _currentKeyboard;
            _previousMouse = _currentMouse;
            _previousMousePosition = _mousePosition;
            
            // Update current states
            _currentKeyboard = keyboard;
            _currentMouse = mouse;
            
            // Update mouse position and delta
            _mousePosition = new Vector2(mouse.X, mouse.Y);
            _mouseDelta = _mousePosition - _previousMousePosition;
            _mouseScroll = mouse.ScrollDelta;
            
            // Update gamepad states
            UpdateGamepads();
        }

        // ==================== KEYBOARD INPUT ====================

        /// <summary>
        /// Check if a key is currently held down
        /// </summary>
        public static bool GetKey(KeyCode key)
        {
            return _currentKeyboard.IsKeyDown((Keys)key);
        }

        /// <summary>
        /// Check if a key was pressed this frame
        /// </summary>
        public static bool GetKeyDown(KeyCode key)
        {
            return _currentKeyboard.IsKeyDown((Keys)key) && 
                   !_previousKeyboard.IsKeyDown((Keys)key);
        }

        /// <summary>
        /// Check if a key was released this frame
        /// </summary>
        public static bool GetKeyUp(KeyCode key)
        {
            return !_currentKeyboard.IsKeyDown((Keys)key) && 
                   _previousKeyboard.IsKeyDown((Keys)key);
        }

        /// <summary>
        /// Check if any key is currently held down
        /// </summary>
        public static bool AnyKey
        {
            get { return _currentKeyboard.IsAnyKeyDown; }
        }

        /// <summary>
        /// Check if any key was pressed this frame
        /// </summary>
        public static bool AnyKeyDown
        {
            get
            {
                // Check if any key is down now that wasn't down before
                foreach (Keys key in Enum.GetValues(typeof(Keys)))
                {
                    if (_currentKeyboard.IsKeyDown(key) && !_previousKeyboard.IsKeyDown(key))
                        return true;
                }
                return false;
            }
        }

        // ==================== MOUSE INPUT ====================

        /// <summary>
        /// Current mouse position in screen coordinates
        /// </summary>
        public static Vector2 MousePosition => _mousePosition;

        /// <summary>
        /// Mouse movement since last frame
        /// </summary>
        public static Vector2 MouseDelta => _mouseDelta;

        /// <summary>
        /// Mouse scroll wheel delta (Y = vertical, X = horizontal if supported)
        /// </summary>
        public static Vector2 MouseScroll => _mouseScroll;

        /// <summary>
        /// Check if a mouse button is currently held down
        /// </summary>
        public static bool GetMouseButton(MouseButton button)
        {
            return _currentMouse.IsButtonDown((OpenTK.Windowing.GraphicsLibraryFramework.MouseButton)button);
        }

        /// <summary>
        /// Check if a mouse button was pressed this frame
        /// </summary>
        public static bool GetMouseButtonDown(MouseButton button)
        {
            return _currentMouse.IsButtonDown((OpenTK.Windowing.GraphicsLibraryFramework.MouseButton)button) &&
                   !_previousMouse.IsButtonDown((OpenTK.Windowing.GraphicsLibraryFramework.MouseButton)button);
        }

        /// <summary>
        /// Check if a mouse button was released this frame
        /// </summary>
        public static bool GetMouseButtonUp(MouseButton button)
        {
            return !_currentMouse.IsButtonDown((OpenTK.Windowing.GraphicsLibraryFramework.MouseButton)button) &&
                   _previousMouse.IsButtonDown((OpenTK.Windowing.GraphicsLibraryFramework.MouseButton)button);
        }

        // ==================== GAMEPAD/CONTROLLER INPUT ====================

        private static void UpdateGamepads()
        {
            // Store previous states
            _previousGamepads.Clear();
            foreach (var kvp in _currentGamepads)
            {
                _previousGamepads[kvp.Key] = kvp.Value;
            }
            
            // Update current states for all connected joysticks
            for (int i = 0; i < 16; i++) // Check up to 16 controllers
            {
                var state = Joysticks.GetState(i);
                if (state.IsConnected)
                {
                    _currentGamepads[i] = state;
                }
                else if (_currentGamepads.ContainsKey(i))
                {
                    _currentGamepads.Remove(i);
                }
            }
        }

        /// <summary>
        /// Check if a gamepad is connected
        /// </summary>
        public static bool IsGamepadConnected(int gamepadIndex = 0)
        {
            return _currentGamepads.ContainsKey(gamepadIndex);
        }

        /// <summary>
        /// Get the number of connected gamepads
        /// </summary>
        public static int GamepadCount => _currentGamepads.Count;

        /// <summary>
        /// Get gamepad button state
        /// </summary>
        public static bool GetGamepadButton(int gamepadIndex, int button)
        {
            if (!_currentGamepads.TryGetValue(gamepadIndex, out var state))
                return false;
            
            if (button < 0 || button >= state.GetButtonCount())
                return false;
            
            return state.GetButton(button) == ButtonState.Pressed;
        }

        /// <summary>
        /// Get gamepad button pressed this frame
        /// </summary>
        public static bool GetGamepadButtonDown(int gamepadIndex, int button)
        {
            if (!_currentGamepads.TryGetValue(gamepadIndex, out var current))
                return false;
            
            if (button < 0 || button >= current.GetButtonCount())
                return false;
            
            bool currentPressed = current.GetButton(button) == ButtonState.Pressed;
            
            if (!_previousGamepads.TryGetValue(gamepadIndex, out var previous))
                return currentPressed;
            
            bool previousPressed = previous.GetButton(button) == ButtonState.Pressed;
            
            return currentPressed && !previousPressed;
        }

        /// <summary>
        /// Get gamepad axis value (-1 to 1)
        /// </summary>
        public static float GetGamepadAxis(int gamepadIndex, int axis)
        {
            if (!_currentGamepads.TryGetValue(gamepadIndex, out var state))
                return 0f;
            
            if (axis < 0 || axis >= state.GetAxisCount())
                return 0f;
            
            return state.GetAxis(axis);
        }

        /// <summary>
        /// Get left stick on gamepad (X and Y from -1 to 1)
        /// </summary>
        public static Vector2 GetGamepadLeftStick(int gamepadIndex = 0)
        {
            return new Vector2(
                GetGamepadAxis(gamepadIndex, 0), // Left stick X
                GetGamepadAxis(gamepadIndex, 1)  // Left stick Y
            );
        }

        /// <summary>
        /// Get right stick on gamepad (X and Y from -1 to 1)
        /// </summary>
        public static Vector2 GetGamepadRightStick(int gamepadIndex = 0)
        {
            return new Vector2(
                GetGamepadAxis(gamepadIndex, 2), // Right stick X
                GetGamepadAxis(gamepadIndex, 3)  // Right stick Y
            );
        }

        // ==================== CONVENIENCE METHODS ====================

        /// <summary>
        /// Get horizontal input axis (-1 to 1)
        /// Combines keyboard (A/D or Left/Right arrows) and gamepad left stick
        /// </summary>
        public static float GetAxisHorizontal()
        {
            float value = 0f;
            
            // Keyboard
            if (GetKey(KeyCode.D) || GetKey(KeyCode.RightArrow)) value += 1f;
            if (GetKey(KeyCode.A) || GetKey(KeyCode.LeftArrow)) value -= 1f;
            
            // Gamepad (if connected)
            if (IsGamepadConnected(0))
            {
                float stick = GetGamepadLeftStick(0).X;
                if (MathF.Abs(stick) > 0.1f) // Deadzone
                    value = stick;
            }
            
            return MathHelper.Clamp(value, -1f, 1f);
        }

        /// <summary>
        /// Get vertical input axis (-1 to 1)
        /// Combines keyboard (W/S or Up/Down arrows) and gamepad left stick
        /// </summary>
        public static float GetAxisVertical()
        {
            float value = 0f;
            
            // Keyboard (note: Up is positive)
            if (GetKey(KeyCode.W) || GetKey(KeyCode.UpArrow)) value += 1f;
            if (GetKey(KeyCode.S) || GetKey(KeyCode.DownArrow)) value -= 1f;
            
            // Gamepad (if connected)
            if (IsGamepadConnected(0))
            {
                float stick = GetGamepadLeftStick(0).Y;
                if (MathF.Abs(stick) > 0.1f) // Deadzone
                    value = stick;
            }
            
            return MathHelper.Clamp(value, -1f, 1f);
        }
    }
}
