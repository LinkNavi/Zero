
using OpenTK.Mathematics;
using EngineCore.ECS;

namespace EngineCore.UI
{
    /// <summary>
    /// Text rendering component for UI
    /// Note: This is a simplified version - full text rendering requires a font system
    /// </summary>
    public class UIText : Component
    {
        public string text = "Text";
        public Color4 color = Color4.White;
        public int fontSize = 14;
        
        public enum TextAlignment { Left, Center, Right }
        public TextAlignment alignment = TextAlignment.Center;
        
        public bool wordWrap = false;

        public UIText() { }

        public UIText(string text)
        {
            this.text = text;
        }
    }
}
