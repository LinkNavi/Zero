using OpenTK.Mathematics;
using EngineCore.ECS;
using EngineCore.Rendering;

namespace EngineCore.Components
{
    /// <summary>
    /// Skybox component for background rendering
    /// </summary>
    public class Skybox : Component
    {
        public Material material;
        public Color4 tintColor = Color4.White;

        public Skybox() { }

        public Skybox(Material material)
        {
            this.material = material;
        }
    }
}
