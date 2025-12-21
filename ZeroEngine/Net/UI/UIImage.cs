using OpenTK.Mathematics;
using EngineCore.ECS;
using EngineCore.Rendering;

namespace EngineCore.UI
{
    /// <summary>
    /// Displays a texture on a UI element
    /// </summary>
    public class UIImage : Component
    {
        public Texture texture;
        public Color4 color = Color4.White;
        
        // How the image fills the rect
        public enum ImageType { Simple, Sliced, Tiled, Filled }
        public ImageType imageType = ImageType.Simple;
        
        // For filled type
        public float fillAmount = 1.0f;

        public UIImage() { }

        public UIImage(Texture texture)
        {
            this.texture = texture;
        }
    }
}
