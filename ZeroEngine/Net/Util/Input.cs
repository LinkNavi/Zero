
using OpenTK.Mathematics;

namespace EngineCore.Utility
{
    /// <summary>
    /// Input helper class (simplified - would integrate with OpenTK input)
    /// </summary>
    public static class Input
    {
        public static bool GetKey(string key) => false; // TODO: Implement
        public static bool GetKeyDown(string key) => false; // TODO: Implement
        public static bool GetKeyUp(string key) => false; // TODO: Implement
        public static Vector2 MousePosition { get; set; } = Vector2.Zero;
        public static bool GetMouseButton(int button) => false; // TODO: Implement
        public static bool GetMouseButtonDown(int button) => false; // TODO: Implement
    }
}
