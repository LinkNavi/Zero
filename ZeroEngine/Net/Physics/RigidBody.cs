using System;
using EngineCore.ECS;

namespace EngineCore.Physics
{
    /// <summary>
    /// Simple rigidbody component for basic physics
    /// </summary>
    public class Rigidbody : MonoBehaviour
    {
        public Vector3 velocity = Vector3.Zero;
        public float mass = 1.0f;
        public float drag = 0.1f;
        public bool useGravity = true;
        public bool isKinematic = false;

        private const float GRAVITY = -9.81f;

        protected override void Update()
        {
            if (isKinematic) return;

            float dt = Time.DeltaTime;

            // Apply gravity
            if (useGravity)
            {
                velocity.y += GRAVITY * dt;
            }

            // Apply drag
            velocity.x *= (1 - drag * dt);
            velocity.y *= (1 - drag * dt);
            velocity.z *= (1 - drag * dt);

            // Update position
            var pos = transform.position;
            pos.x += velocity.x * dt;
            pos.y += velocity.y * dt;
            pos.z += velocity.z * dt;
            transform.position = pos;
        }

        public void AddForce(Vector3 force)
        {
            velocity.x += force.x / mass;
            velocity.y += force.y / mass;
            velocity.z += force.z / mass;
        }
    }
}
