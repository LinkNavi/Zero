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
        private static KeyboardState _currentKeyboard = default!;
        private static KeyboardState _previousKeyboard = default!;
        
        private static MouseState _currentMouse = default!;
        private static MouseState _previousMouse = default!;
        
        private static Vector2 _mousePosition;
        private static Vector2 _previousMousePosition;
        private static Vector2 _mouseDelta;
        private static Vector2 _mouseScroll;

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

        // ==================== CONVENIENCE METHODS ====================

        /// <summary>
        /// Get horizontal input axis (-1 to 1)
        /// Keyboard only (A/D or Left/Right arrows)
        /// </summary>
        public static float GetAxisHorizontal()
        {
            float value = 0f;
            
            // Keyboard
            if (GetKey(KeyCode.D) || GetKey(KeyCode.RightArrow)) value += 1f;
            if (GetKey(KeyCode.A) || GetKey(KeyCode.LeftArrow)) value -= 1f;
            
            return OpenTK.Mathematics.MathHelper.Clamp(value, -1f, 1f);
        }

        /// <summary>
        /// Get vertical input axis (-1 to 1)
        /// Keyboard only (W/S or Up/Down arrows)
        /// </summary>
        public static float GetAxisVertical()
        {
            float value = 0f;
            
            // Keyboard (note: Up is positive)
            if (GetKey(KeyCode.W) || GetKey(KeyCode.UpArrow)) value += 1f;
            if (GetKey(KeyCode.S) || GetKey(KeyCode.DownArrow)) value -= 1f;
            
            return OpenTK.Mathematics.MathHelper.Clamp(value, -1f, 1f);
        }
    }
}
