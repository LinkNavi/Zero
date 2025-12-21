using System;
using EngineCore.ECS;

namespace EngineCore.Physics
{
    /// <summary>
    /// Box collider for simple collision detection
    /// </summary>
    public class BoxCollider : Component
    {
        public Vector3 center = Vector3.Zero;
        public Vector3 size = Vector3.One;
        public bool isTrigger = false;

        public BoxCollider() { }

        public BoxCollider(Vector3 size)
        {
            this.size = size;
        }

        /// <summary>
        /// Check if this collider intersects with another
        /// </summary>
        public bool Intersects(BoxCollider other)
        {
            var thisMin = GetMin();
            var thisMax = GetMax();
            var otherMin = other.GetMin();
            var otherMax = other.GetMax();

            return (thisMin.x <= otherMax.x && thisMax.x >= otherMin.x) &&
                   (thisMin.y <= otherMax.y && thisMax.y >= otherMin.y) &&
                   (thisMin.z <= otherMax.z && thisMax.z >= otherMin.z);
        }

        private Vector3 GetMin()
        {
            var pos = transform.position;
            return new Vector3(
                pos.x + center.x - size.x * 0.5f,
                pos.y + center.y - size.y * 0.5f,
                pos.z + center.z - size.z * 0.5f
            );
        }

        private Vector3 GetMax()
        {
            var pos = transform.position;
            return new Vector3(
                pos.x + center.x + size.x * 0.5f,
                pos.y + center.y + size.y * 0.5f,
                pos.z + center.z + size.z * 0.5f
            );
        }
    }
}
