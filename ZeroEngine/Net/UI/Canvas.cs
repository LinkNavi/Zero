
using EngineCore.ECS;
using EngineCore.Components;
using OpenTK.Mathematics;

namespace EngineCore.UI
{
    /// <summary>
    /// Canvas component for UI rendering
    /// All UI elements must be children of a Canvas
    /// </summary>
    public class Canvas : Component
    {
        public enum RenderMode { ScreenSpaceOverlay, ScreenSpaceCamera, WorldSpace }
        
        public RenderMode renderMode = RenderMode.ScreenSpaceOverlay;
        public int sortOrder = 0;
        
        // For ScreenSpaceCamera mode
        public CameraComponent uiCamera;
        
        // Screen dimensions (updated by rendering system)
        internal int screenWidth = 1280;
        internal int screenHeight = 720;

        public Canvas()
        {
        }

        /// <summary>
        /// Convert screen position to canvas position
        /// </summary>
        public Vector2 ScreenToCanvasPoint(Vector2 screenPos)
        {
            return new Vector2(
                (screenPos.X / screenWidth) * 2 - 1,
                1 - (screenPos.Y / screenHeight) * 2
            );
        }
    }
}
