using System;
using System.Numerics;
using System.Runtime.CompilerServices;
using BepuPhysics;
using BepuPhysics.Collidables;
using BepuPhysics.CollisionDetection;
using BepuPhysics.Constraints;
using BepuUtilities;
using BepuUtilities.Memory;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;
using SysVector3 = System.Numerics.Vector3;

namespace EngineCore.Physics
{
    public struct NarrowPhaseCallbacks : INarrowPhaseCallbacks
    {
        public void Initialize(Simulation simulation) { }
        
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool AllowContactGeneration(int workerIndex, CollidableReference a, CollidableReference b, ref float speculativeMargin) => true;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool AllowContactGeneration(int workerIndex, CollidablePair pair, int childIndexA, int childIndexB) => true;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool ConfigureContactManifold<TManifold>(int workerIndex, CollidablePair pair, ref TManifold manifold, out PairMaterialProperties pairMaterial) where TManifold : unmanaged, IContactManifold<TManifold>
        {
            pairMaterial.FrictionCoefficient = 0.5f;
            pairMaterial.MaximumRecoveryVelocity = 2f;
            pairMaterial.SpringSettings = new SpringSettings(30, 1);
            return true;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool ConfigureContactManifold(int workerIndex, CollidablePair pair, int childIndexA, int childIndexB, ref ConvexContactManifold manifold) => true;

        public void Dispose() { }
    }

    public struct PoseIntegratorCallbacks : IPoseIntegratorCallbacks
    {
        public SysVector3 Gravity;
        public float LinearDamping;
        public float AngularDamping;

        public readonly AngularIntegrationMode AngularIntegrationMode => AngularIntegrationMode.Nonconserving;
        public readonly bool AllowSubstepsForUnconstrainedBodies => false;
        public readonly bool IntegrateVelocityForKinematics => false;

        public void Initialize(Simulation simulation) { }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void PrepareForIntegration(float dt) { }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void IntegrateVelocity(Vector<int> bodyIndices, Vector3Wide position, QuaternionWide orientation, BodyInertiaWide localInertia, Vector<int> integrationMask, int workerIndex, Vector<float> dt, ref BodyVelocityWide velocity)
        {
            var gravityDt = Gravity * dt[0];
            var linearDamp = new Vector<float>(MathF.Pow(1f - LinearDamping, dt[0]));
            var angularDamp = new Vector<float>(MathF.Pow(1f - AngularDamping, dt[0]));
            
            velocity.Linear.X = velocity.Linear.X + new Vector<float>(gravityDt.X);
            velocity.Linear.Y = velocity.Linear.Y + new Vector<float>(gravityDt.Y);
            velocity.Linear.Z = velocity.Linear.Z + new Vector<float>(gravityDt.Z);
            
            velocity.Linear.X *= linearDamp;
            velocity.Linear.Y *= linearDamp;
            velocity.Linear.Z *= linearDamp;
            velocity.Angular.X *= angularDamp;
            velocity.Angular.Y *= angularDamp;
            velocity.Angular.Z *= angularDamp;
        }
    }

    public static class PhysXManager
    {
        private static BufferPool _bufferPool;
        private static Simulation _simulation;
        private static NarrowPhaseCallbacks _narrowPhaseCallbacks;
        private static PoseIntegratorCallbacks _poseCallbacks;
        private static bool _initialized;

        public static SysVector3 Gravity = new SysVector3(0, -9.81f, 0);
        public static float FixedTimeStep = 1.0f / 60.0f;
        public static bool IsInitialized => _initialized;

        public static void Initialize()
        {
            if (_initialized) return;

            _bufferPool = new BufferPool();
            _narrowPhaseCallbacks = new NarrowPhaseCallbacks();
            _poseCallbacks = new PoseIntegratorCallbacks
            {
                Gravity = Gravity,
                LinearDamping = 0.03f,
                AngularDamping = 0.03f
            };

            _simulation = Simulation.Create(_bufferPool, _narrowPhaseCallbacks, _poseCallbacks, new SolveDescription(8, 1));
            _initialized = true;
        }

        public static void Update(float deltaTime)
        {
            if (!_initialized) return;
            _simulation.Timestep(deltaTime);
        }

        public static void Shutdown()
        {
            if (!_initialized) return;
            _simulation.Dispose();
            _bufferPool.Clear();
            _initialized = false;
        }

        public static Simulation GetSimulation() => _simulation;
        public static BufferPool GetBufferPool() => _bufferPool;
    }

    public struct RaycastHit
    {
        public SysVector3 Point;
        public SysVector3 Normal;
        public float Distance;
        public PhysXCollider Collider;
    }

    public enum ForceMode
    {
        Force,
        Impulse,
        VelocityChange,
        Acceleration
    }

    public class PhysXRigidbody : MonoBehaviour
    {
        private BodyHandle _handle;
        private bool _initialized;

        public float Mass { get; set; } = 1.0f;
        public float LinearDamping { get; set; } = 0.1f;
        public float AngularDamping { get; set; } = 0.05f;
        public bool UseGravity { get; set; } = true;
        public bool IsKinematic { get; set; }

        public ECSVector3 LinearVelocity
        {
            get
            {
                if (!_initialized) return ECSVector3.Zero;
                var vel = PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle).Velocity.Linear;
                return new ECSVector3(vel.X, vel.Y, vel.Z);
            }
            set
            {
                if (_initialized)
                {
                    PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle).Velocity.Linear = new SysVector3(value.x, value.y, value.z);
                }
            }
        }

        public ECSVector3 AngularVelocity
        {
            get
            {
                if (!_initialized) return ECSVector3.Zero;
                var vel = PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle).Velocity.Angular;
                return new ECSVector3(vel.X, vel.Y, vel.Z);
            }
            set
            {
                if (_initialized)
                {
                    PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle).Velocity.Angular = new SysVector3(value.x, value.y, value.z);
                }
            }
        }

        protected override void Awake()
        {
            if (!PhysXManager.IsInitialized) return;
            InitializeBody();
        }

        private void InitializeBody()
        {
            var pos = transform.position;
            var shape = new Sphere(0.5f);
            var shapeIndex = PhysXManager.GetSimulation().Shapes.Add(shape);
            
            var inertia = shape.ComputeInertia(Mass);
            var collidable = new CollidableDescription(shapeIndex, 0.1f);
            var bodyDesc = BodyDescription.CreateDynamic(new SysVector3(pos.x, pos.y, pos.z), inertia, collidable, 0.01f);
            
            _handle = PhysXManager.GetSimulation().Bodies.Add(bodyDesc);
            _initialized = true;
        }

        protected override void Update()
        {
            if (!_initialized) return;

            var body = PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle);
            var p = body.Pose.Position;
            var q = body.Pose.Orientation;

            transform.position = new ECSVector3(p.X, p.Y, p.Z);
            transform.localRotation = new EngineCore.ECS.Quaternion(q.X, q.Y, q.Z, q.W);
        }

        protected override void OnDestroy()
        {
            if (_initialized)
            {
                PhysXManager.GetSimulation().Bodies.Remove(_handle);
                _initialized = false;
            }
        }

        public void AddForce(ECSVector3 force, ForceMode mode = ForceMode.Force)
        {
            if (!_initialized) return;

            var body = PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle);
            var f = new SysVector3(force.x, force.y, force.z);

            if (mode == ForceMode.Impulse)
                body.Velocity.Linear += f / Mass;
            else
                body.Velocity.Linear += f * PhysXManager.FixedTimeStep / Mass;
        }

        public void AddTorque(ECSVector3 torque, ForceMode mode = ForceMode.Force)
        {
            if (!_initialized) return;
            
            var body = PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle);
            var t = new SysVector3(torque.x, torque.y, torque.z);
            
            if (mode == ForceMode.Impulse)
                body.Velocity.Angular += t;
            else
                body.Velocity.Angular += t * PhysXManager.FixedTimeStep;
        }

        public void AddForceAtPosition(ECSVector3 force, ECSVector3 position, ForceMode mode = ForceMode.Force)
        {
            if (!_initialized) return;

            var body = PhysXManager.GetSimulation().Bodies.GetBodyReference(_handle);
            var f = new SysVector3(force.x, force.y, force.z);
            var p = new SysVector3(position.x, position.y, position.z);
            var center = body.Pose.Position;
            
            var r = p - center;
            var torque = SysVector3.Cross(r, f);
            
            if (mode == ForceMode.Impulse)
            {
                body.Velocity.Linear += f / Mass;
                body.Velocity.Angular += torque;
            }
            else
            {
                body.Velocity.Linear += f * PhysXManager.FixedTimeStep / Mass;
                body.Velocity.Angular += torque * PhysXManager.FixedTimeStep;
            }
        }

        public void AddExplosionForce(float explosionForce, ECSVector3 explosionPosition, float explosionRadius)
        {
            if (!_initialized) return;

            var pos = transform.position;
            var dir = new ECSVector3(pos.x - explosionPosition.x, pos.y - explosionPosition.y, pos.z - explosionPosition.z);
            var dist = MathF.Sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            
            if (dist < explosionRadius)
            {
                var mag = MathF.Sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                if (mag > 0.0001f)
                {
                    dir.x /= mag;
                    dir.y /= mag;
                    dir.z /= mag;
                }

                var force = explosionForce * (1 - dist / explosionRadius);
                AddForce(new ECSVector3(dir.x * force, dir.y * force, dir.z * force), ForceMode.Impulse);
            }
        }

        public BodyHandle GetHandle() => _handle;
    }

    public abstract class PhysXCollider : Component
    {
        protected PhysXRigidbody _rigidbody;
        public bool IsTrigger { get; set; }
        public ECSVector3 Center { get; set; } = ECSVector3.Zero;

        internal override void OnAttached()
        {
            _rigidbody = gameObject.GetComponent<PhysXRigidbody>();
        }
    }

    public class PhysXBoxCollider : PhysXCollider
    {
        public ECSVector3 Size { get; set; } = ECSVector3.One;
    }

    public class PhysXSphereCollider : PhysXCollider
    {
        public float Radius { get; set; } = 0.5f;
    }

    public class PhysXCapsuleCollider : PhysXCollider
    {
        public float Radius { get; set; } = 0.5f;
        public float Height { get; set; } = 2.0f;
    }

    public class PhysXPlaneCollider : PhysXCollider
    {
    }

    public class PhysXCharacterController : MonoBehaviour
    {
        public float Height { get; set; } = 2.0f;
        public float Radius { get; set; } = 0.5f;
        public float StepOffset { get; set; } = 0.5f;
        public float SlopeLimit { get; set; } = 45.0f;

        public void Move(ECSVector3 motion)
        {
        }
    }
}
