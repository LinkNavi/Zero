using System;
using System.Collections.Generic;
using OpenTK.Mathematics;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Effects
{
    /// <summary>
    /// Simple particle system component
    /// </summary>
    public class ParticleSystem : MonoBehaviour
    {
        public struct Particle
        {
            public ECSVector3 position;
            public ECSVector3 velocity;
            public Color4 color;
            public float lifetime;
            public float age;
            public float size;
        }

        public int maxParticles = 100;
        public float emissionRate = 10f; // particles per second
        public float startLifetime = 2f;
        public float startSize = 1f;
        public ECSVector3 startVelocity = new ECSVector3(0, 1, 0);
        public Color4 startColor = Color4.White;
        public bool loop = true;

        private List<Particle> _particles = new List<Particle>();
        private float _emissionTimer = 0f;

        protected override void Update()
        {
            float dt = EngineCore.Time.DeltaTime;

            // Emit particles
            if (loop || _particles.Count < maxParticles)
            {
                _emissionTimer += dt;
                int particlesToEmit = (int)(_emissionTimer * emissionRate);
                _emissionTimer -= particlesToEmit / emissionRate;

                for (int i = 0; i < particlesToEmit && _particles.Count < maxParticles; i++)
                {
                    EmitParticle();
                }
            }

            // Update particles
            for (int i = _particles.Count - 1; i >= 0; i--)
            {
                var p = _particles[i];
                p.age += dt;

                if (p.age >= p.lifetime)
                {
                    _particles.RemoveAt(i);
                    continue;
                }

                // Update position
                p.position.x += p.velocity.x * dt;
                p.position.y += p.velocity.y * dt;
                p.position.z += p.velocity.z * dt;

                // Apply gravity
                p.velocity.y -= 9.81f * dt;

                _particles[i] = p;
            }
        }

        private void EmitParticle()
        {
            var particle = new Particle
            {
                position = new ECSVector3(transform.position.x, transform.position.y, transform.position.z),
                velocity = startVelocity,
                color = startColor,
                lifetime = startLifetime,
                age = 0,
                size = startSize
            };
            _particles.Add(particle);
        }

        public List<Particle> GetParticles() => _particles;
    }
}
