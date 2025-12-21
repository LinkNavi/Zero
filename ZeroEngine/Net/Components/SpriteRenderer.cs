
using System;
using OpenTK.Mathematics;
using EngineCore.ECS;
using EngineCore.Rendering;

namespace EngineCore.Components
{
    /// <summary>
    /// Renders 2D sprites (textured quads)
    /// Useful for 2D games and UI elements
    /// </summary>
    public class SpriteRenderer : Component
    {
        public Texture sprite;
        public Color4 color = Color4.White;
        public Vector2 size = Vector2.One;
        public bool flipX = false;
        public bool flipY = false;
        
        // Sprite rendering order (higher = rendered on top)
        public int sortingOrder = 0;

        public SpriteRenderer() { }

        public SpriteRenderer(Texture sprite)
        {
            this.sprite = sprite;
        }

        public Matrix4 GetModelMatrix()
        {
            var t = transform;
            var translation = Matrix4.CreateTranslation(t.position.x, t.position.y, t.position.z);
            var scale = Matrix4.CreateScale(size.X, size.Y, 1);
            return scale * translation;
        }
    }
}
