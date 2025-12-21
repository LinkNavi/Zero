using System;
using System.Collections.Generic;
using OpenTK.Mathematics;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Effects
{
    /// <summary>
    /// Creates a trail behind moving objects
    /// </summary>
    public class TrailRenderer : MonoBehaviour
    {
        public struct TrailPoint
        {
            public ECSVector3 position;
            public float age;
        }

        public float time = 1f; // How long the trail lasts
        public float minVertexDistance = 0.1f;
        public Color4 startColor = Color4.White;
        public Color4 endColor = new Color4(1, 1, 1, 0);
        public float startWidth = 1f;
        public float endWidth = 0.1f;

        private List<TrailPoint> _points = new List<TrailPoint>();

        protected override void Update()
        {
            float dt = EngineCore.Time.DeltaTime;

            // Add new point if moved enough
            var currentPos = new ECSVector3(transform.position.x, transform.position.y, transform.position.z);
            
            if (_points.Count == 0 || Vector3Distance(_points[0].position, currentPos) > minVertexDistance)
            {
                _points.Insert(0, new TrailPoint { position = currentPos, age = 0 });
            }

            // Age and remove old points
            for (int i = _points.Count - 1; i >= 0; i--)
            {
                var point = _points[i];
                point.age += dt;
                _points[i] = point;

                if (point.age > time)
                {
                    _points.RemoveAt(i);
                }
            }
        }

        private float Vector3Distance(ECSVector3 a, ECSVector3 b)
        {
            float dx = a.x - b.x;
            float dy = a.y - b.y;
            float dz = a.z - b.z;
            return MathF.Sqrt(dx * dx + dy * dy + dz * dz);
        }

        public List<TrailPoint> GetPoints() => _points;
    }
}
