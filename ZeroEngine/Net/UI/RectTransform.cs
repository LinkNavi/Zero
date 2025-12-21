using OpenTK.Mathematics;
using EngineCore.ECS;

namespace EngineCore.UI
{
    /// <summary>
    /// RectTransform for UI elements (similar to Unity's RectTransform)
    /// Defines position, size, and anchors for UI
    /// </summary>
    public class RectTransform : Component
    {
        // Anchored position relative to anchors
        public Vector2 anchoredPosition = Vector2.Zero;
        
        // Size of the rect
        public Vector2 sizeDelta = new Vector2(100, 100);
        
        // Anchor points (0-1 range, relative to parent)
        public Vector2 anchorMin = new Vector2(0.5f, 0.5f);
        public Vector2 anchorMax = new Vector2(0.5f, 0.5f);
        
        // Pivot point (0-1 range, relative to rect)
        public Vector2 pivot = new Vector2(0.5f, 0.5f);
        
        // Rotation in degrees
        public float rotation = 0f;
        
        // Scale
        public Vector2 scale = Vector2.One;

        public RectTransform() { }

        /// <summary>
        /// Get the world rect bounds
        /// </summary>
        public (Vector2 min, Vector2 max) GetWorldRect(Canvas canvas)
        {
            if (canvas == null) return (Vector2.Zero, sizeDelta);

            float canvasWidth = canvas.screenWidth;
            float canvasHeight = canvas.screenHeight;

            // Calculate anchor position in pixels
            Vector2 anchorCenter = (anchorMin + anchorMax) * 0.5f;
            Vector2 anchorPos = new Vector2(
                anchorCenter.X * canvasWidth,
                anchorCenter.Y * canvasHeight
            );

            // Calculate final position
            Vector2 worldPos = anchorPos + anchoredPosition;

            // Calculate bounds
            Vector2 halfSize = sizeDelta * 0.5f;
            Vector2 min = worldPos - halfSize * pivot;
            Vector2 max = min + sizeDelta;

            return (min, max);
        }
    }
}
