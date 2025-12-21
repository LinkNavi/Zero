using System;
using System.Collections.Generic;
using System.Numerics;
using System.Runtime.InteropServices;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;
using MagicPhysX; // PhysX wrapper for C#
using static MagicPhysX.NativeMethods;

namespace EngineCore.Physics
{
    /// <summary>
    /// PhysX Physics System Manager
    /// </summary>
    public static unsafe class PhysXManager
    {
        private static PxFoundation* _foundation;
        private static PxPhysics* _physics;
        private static PxScene* _scene;
        private static PxDefaultCpuDispatcher* _dispatcher;
        private static PxMaterial* _defaultMaterial;
        private static bool _initialized = false;

        public static PxScene* Scene => _scene;
        public static PxPhysics* Physics => _physics;
        public static bool IsInitialized => _initialized;

        // Physics settings
        public static System.Numerics.Vector3 Gravity { get; set; } = new System.Numerics.Vector3(0, -9.81f, 0);
        public static float FixedTimeStep { get; set; } = 1.0f / 60.0f;
        private static float _accumulator = 0;

        /// <summary>
        /// Initialize PhysX
        /// </summary>
        public static void Initialize()
        {
            if (_initialized) return;

            try
            {
                // Create foundation
                _foundation = phys_PxCreateFoundation(PX_PHYSICS_VERSION, null, null);
                
                // Create physics with tolerances scale
                _physics = phys_PxCreatePhysics(PX_PHYSICS_VERSION, _foundation, null, false, null, null);

                // Create dispatcher
                _dispatcher = phys_PxDefaultCpuDispatcherCreate(2, null, PxDefaultCpuDispatcherWaitForWorkMode.WaitForWork, 0);

                // Create scene
                var sceneDesc = PxSceneDesc_new(PxPhysics_getTolerancesScale(_physics));
                sceneDesc->gravity = new PxVec3 { x = Gravity.X, y = Gravity.Y, z = Gravity.Z };
                sceneDesc->cpuDispatcher = (PxCpuDispatcher*)_dispatcher;
                sceneDesc->filterShader = PxDefaultSimulationFilterShader_new();

                _scene = PxPhysics_createScene_mut(_physics, sceneDesc);

                // Create default material
                _defaultMaterial = PxPhysics_createMaterial_mut(_physics, 0.5f, 0.5f, 0.1f);

                _initialized = true;
                Console.WriteLine("PhysX initialized successfully");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"ERROR initializing PhysX: {ex.Message}");
            }
        }

        /// <summary>
        /// Update physics simulation
        /// </summary>
        public static void Update(float deltaTime)
        {
            if (!_initialized) return;

            _accumulator += deltaTime;

            // Fixed timestep updates
            while (_accumulator >= FixedTimeStep)
            {
                PxScene_simulate_mut(_scene, FixedTimeStep, null, null, 0, true);
                uint error = 0;
                PxScene_fetchResults_mut(_scene, true, &error);
                _accumulator -= FixedTimeStep;
            }
        }

        /// <summary>
        /// Shutdown PhysX
        /// </summary>
        public static void Shutdown()
        {
            if (!_initialized) return;

            if (_scene != null) PxScene_release_mut(_scene);
            if (_defaultMaterial != null) PxMaterial_release_mut(_defaultMaterial);
            if (_dispatcher != null) PxDefaultCpuDispatcher_release_mut(_dispatcher);
            if (_physics != null) PxPhysics_release_mut(_physics);
            if (_foundation != null) PxFoundation_release_mut(_foundation);

            _initialized = false;
            Console.WriteLine("PhysX shut down");
        }

        public static PxMaterial* GetDefaultMaterial() => _defaultMaterial;

        /// <summary>
        /// Raycast against physics world
        /// </summary>
        public static bool Raycast(System.Numerics.Vector3 origin, System.Numerics.Vector3 direction, float maxDistance, out RaycastHit hit)
        {
            hit = default;
            if (!_initialized) return false;

            var pxOrigin = new PxVec3 { x = origin.X, y = origin.Y, z = origin.Z };
            var pxDir = System.Numerics.Vector3.Normalize(direction);
            var pxDirection = new PxVec3 { x = pxDir.X, y = pxDir.Y, z = pxDir.Z };

            PxRaycastBuffer hitBuffer;
            bool hasHit = PxScene_raycast(_scene, &pxOrigin, &pxDirection, maxDistance, &hitBuffer, 
                PxHitFlags.Default, null, null, null);

            if (hasHit && hitBuffer.hasBlock != 0)
            {
                var block = hitBuffer.block;
                hit = new RaycastHit
                {
                    Point = new System.Numerics.Vector3(block.position.x, block.position.y, block.position.z),
                    Normal = new System.Numerics.Vector3(block.normal.x, block.normal.y, block.normal.z),
                    Distance = block.distance,
                    Collider = null // Would need actor-to-collider mapping
                };
                return true;
            }

            return false;
        }
    }

    /// <summary>
    /// Raycast hit information
    /// </summary>
    public struct RaycastHit
    {
        public System.Numerics.Vector3 Point;
        public System.Numerics.Vector3 Normal;
        public float Distance;
        public PhysXCollider Collider;
    }

    /// <summary>
    /// PhysX Rigidbody Component
    /// </summary>
    public unsafe class PhysXRigidbody : MonoBehaviour
    {
        private PxRigidDynamic* _actor;
        private bool _initialized = false;

        // Properties
        public float Mass { get; set; } = 1.0f;
        public float LinearDamping { get; set; } = 0.1f;
        public float AngularDamping { get; set; } = 0.05f;
        public bool UseGravity { get; set; } = true;
        public bool IsKinematic { get; set; } = false;
        public bool FreezePositionX { get; set; } = false;
        public bool FreezePositionY { get; set; } = false;
        public bool FreezePositionZ { get; set; } = false;
        public bool FreezeRotationX { get; set; } = false;
        public bool FreezeRotationY { get; set; } = false;
        public bool FreezeRotationZ { get; set; } = false;

        public ECSVector3 LinearVelocity
        {
            get
            {
                if (_actor == null) return ECSVector3.Zero;
                var vel = PxRigidBody_getLinearVelocity((PxRigidBody*)_actor);
                return new ECSVector3(vel.x, vel.y, vel.z);
            }
            set
            {
                if (_actor != null)
                {
                    var vel = new PxVec3 { x = value.x, y = value.y, z = value.z };
                    PxRigidBody_setLinearVelocity_mut((PxRigidBody*)_actor, &vel, true);
                }
            }
        }

        public ECSVector3 AngularVelocity
        {
            get
            {
                if (_actor == null) return ECSVector3.Zero;
                var vel = PxRigidBody_getAngularVelocity((PxRigidBody*)_actor);
                return new ECSVector3(vel.x, vel.y, vel.z);
            }
            set
            {
                if (_actor != null)
                {
                    var vel = new PxVec3 { x = value.x, y = value.y, z = value.z };
                    PxRigidBody_setAngularVelocity_mut((PxRigidBody*)_actor, &vel, true);
                }
            }
        }

        protected override void Awake()
        {
            if (!PhysXManager.IsInitialized)
            {
                Console.WriteLine("WARNING: PhysXRigidbody created but PhysX not initialized");
                return;
            }

            InitializeActor();
        }

        private void InitializeActor()
        {
            var pos = transform.position;
            var rot = transform.localRotation;

            var pxTransform = new PxTransform
            {
                p = new PxVec3 { x = pos.x, y = pos.y, z = pos.z },
                q = new PxQuat { x = rot.x, y = rot.y, z = rot.z, w = rot.w }
            };

            _actor = PxPhysics_createRigidDynamic_mut(PhysXManager.Physics, &pxTransform);

            // Set properties
            PxRigidBody_setMass_mut((PxRigidBody*)_actor, Mass);
            PxRigidBody_setLinearDamping_mut((PxRigidBody*)_actor, LinearDamping);
            PxRigidBody_setAngularDamping_mut((PxRigidBody*)_actor, AngularDamping);
            PxActor_setActorFlag_mut((PxActor*)_actor, PxActorFlag.DisableGravity, !UseGravity);
            PxRigidBody_setRigidBodyFlag_mut((PxRigidBody*)_actor, PxRigidBodyFlag.Kinematic, IsKinematic);

            UpdateConstraints();

            PxScene_addActor_mut(PhysXManager.Scene, (PxActor*)_actor, null);
            _initialized = true;
        }

        protected override void Update()
        {
            if (!_initialized || _actor == null) return;

            // Sync transform from PhysX
            if (!IsKinematic)
            {
                var pxTransform = PxRigidActor_getGlobalPose((PxRigidActor*)_actor);
                var pos = pxTransform.p;
                var rot = pxTransform.q;

                transform.position = new ECSVector3(pos.x, pos.y, pos.z);
                transform.localRotation = new Quaternion(rot.x, rot.y, rot.z, rot.w);
            }
            else
            {
                // Sync to PhysX for kinematic bodies
                var pos = transform.position;
                var rot = transform.localRotation;
                var pxTransform = new PxTransform
                {
                    p = new PxVec3 { x = pos.x, y = pos.y, z = pos.z },
                    q = new PxQuat { x = rot.x, y = rot.y, z = rot.z, w = rot.w }
                };
                PxRigidDynamic_setKinematicTarget_mut(_actor, &pxTransform);
            }
        }

        protected override void OnDestroy()
        {
            if (_actor != null)
            {
                PxScene_removeActor_mut(PhysXManager.Scene, (PxActor*)_actor, true);
                PxRigidActor_release_mut((PxRigidActor*)_actor);
                _actor = null;
            }
        }

        /// <summary>
        /// Add force to the rigidbody
        /// </summary>
        public void AddForce(ECSVector3 force, ForceMode mode = ForceMode.Force)
        {
            if (_actor == null) return;

            var pxForce = new PxVec3 { x = force.x, y = force.y, z = force.z };
            PxRigidBody_addForce_mut((PxRigidBody*)_actor, &pxForce, ToPxForceMode(mode), true);
        }

        /// <summary>
        /// Add torque to the rigidbody
        /// </summary>
        public void AddTorque(ECSVector3 torque, ForceMode mode = ForceMode.Force)
        {
            if (_actor == null) return;

            var pxTorque = new PxVec3 { x = torque.x, y = torque.y, z = torque.z };
            PxRigidBody_addTorque_mut((PxRigidBody*)_actor, &pxTorque, ToPxForceMode(mode), true);
        }

        /// <summary>
        /// Add force at a specific position
        /// </summary>
        public void AddForceAtPosition(ECSVector3 force, ECSVector3 position, ForceMode mode = ForceMode.Force)
        {
            if (_actor == null) return;

            var pxForce = new PxVec3 { x = force.x, y = force.y, z = force.z };
            var pxPos = new PxVec3 { x = position.x, y = position.y, z = position.z };
            
            // Calculate torque from force at position
            var centerOfMass = PxRigidBody_getCMassLocalPose((PxRigidBody*)_actor).p;
            var r = new PxVec3 
            { 
                x = pxPos.x - centerOfMass.x, 
                y = pxPos.y - centerOfMass.y, 
                z = pxPos.z - centerOfMass.z 
            };
            
            // Cross product for torque
            var torque = new PxVec3
            {
                x = r.y * pxForce.z - r.z * pxForce.y,
                y = r.z * pxForce.x - r.x * pxForce.z,
                z = r.x * pxForce.y - r.y * pxForce.x
            };
            
            PxRigidBody_addForce_mut((PxRigidBody*)_actor, &pxForce, ToPxForceMode(mode), true);
            PxRigidBody_addTorque_mut((PxRigidBody*)_actor, &torque, ToPxForceMode(mode), true);
        }

        /// <summary>
        /// Add explosion force
        /// </summary>
        public void AddExplosionForce(float explosionForce, ECSVector3 explosionPosition, float explosionRadius)
        {
            if (_actor == null) return;

            var pos = transform.position;
            var dir = new ECSVector3(
                pos.x - explosionPosition.x,
                pos.y - explosionPosition.y,
                pos.z - explosionPosition.z
            );

            float dist = MathF.Sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            if (dist < explosionRadius)
            {
                float magnitude = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
                if (magnitude > 0.0001f)
                {
                    magnitude = MathF.Sqrt(magnitude);
                    dir.x /= magnitude;
                    dir.y /= magnitude;
                    dir.z /= magnitude;
                }

                float force = explosionForce * (1 - dist / explosionRadius);
                AddForce(new ECSVector3(dir.x * force, dir.y * force, dir.z * force), ForceMode.Impulse);
            }
        }

        private void UpdateConstraints()
        {
            if (_actor == null) return;

            var flags = PxRigidDynamicLockFlag.None;
            if (FreezePositionX) flags |= PxRigidDynamicLockFlag.LockLinearX;
            if (FreezePositionY) flags |= PxRigidDynamicLockFlag.LockLinearY;
            if (FreezePositionZ) flags |= PxRigidDynamicLockFlag.LockLinearZ;
            if (FreezeRotationX) flags |= PxRigidDynamicLockFlag.LockAngularX;
            if (FreezeRotationY) flags |= PxRigidDynamicLockFlag.LockAngularY;
            if (FreezeRotationZ) flags |= PxRigidDynamicLockFlag.LockAngularZ;

            PxRigidDynamic_setRigidDynamicLockFlags_mut(_actor, flags);
        }

        private PxForceMode ToPxForceMode(ForceMode mode)
        {
            return mode switch
            {
                ForceMode.Force => PxForceMode.Force,
                ForceMode.Impulse => PxForceMode.Impulse,
                ForceMode.VelocityChange => PxForceMode.VelocityChange,
                ForceMode.Acceleration => PxForceMode.Acceleration,
                _ => PxForceMode.Force
            };
        }

        public PxRigidDynamic* GetActor() => _actor;
    }

    public enum ForceMode
    {
        Force,
        Impulse,
        VelocityChange,
        Acceleration
    }

    /// <summary>
    /// Base PhysX Collider Component
    /// </summary>
    public abstract unsafe class PhysXCollider : Component
    {
        protected PxShape* _shape;
        protected PhysXRigidbody _rigidbody;

        public bool IsTrigger { get; set; } = false;
        public PxMaterial* Material { get; set; }

        public ECSVector3 Center { get; set; } = ECSVector3.Zero;

        internal override void OnAttached()
        {
            _rigidbody = gameObject.GetComponent<PhysXRigidbody>();
            if (Material == null)
                Material = PhysXManager.GetDefaultMaterial();
        }

        protected void AttachShape(PxShape* shape)
        {
            _shape = shape;
            PxShape_setFlag_mut(_shape, PxShapeFlag.SimulationShape, !IsTrigger);
            PxShape_setFlag_mut(_shape, PxShapeFlag.TriggerShape, IsTrigger);

            if (_rigidbody != null)
            {
                var actor = _rigidbody.GetActor();
                if (actor != null)
                    PxRigidActor_attachShape_mut((PxRigidActor*)actor, _shape);
            }
        }

        internal override void OnDetached()
        {
            if (_shape != null)
            {
                if (_rigidbody != null)
                {
                    var actor = _rigidbody.GetActor();
                    if (actor != null)
                        PxRigidActor_detachShape_mut((PxRigidActor*)actor, _shape, true);
                }
                PxShape_release_mut(_shape);
                _shape = null;
            }
        }
    }

    /// <summary>
    /// Box Collider
    /// </summary>
    public unsafe class PhysXBoxCollider : PhysXCollider
    {
        public ECSVector3 Size { get; set; } = ECSVector3.One;

        internal override void OnAttached()
        {
            base.OnAttached();
            CreateShape();
        }

        private void CreateShape()
        {
            var halfExtents = new PxVec3 { x = Size.x * 0.5f, y = Size.y * 0.5f, z = Size.z * 0.5f };
            var geometry = PxBoxGeometry_new(&halfExtents);
            var shape = PxPhysics_createShape_mut(PhysXManager.Physics, &geometry, Material, true, PxShapeFlags.SceneQueryShape | PxShapeFlags.SimulationShape);
            
            AttachShape(shape);
        }
    }

    /// <summary>
    /// Sphere Collider
    /// </summary>
    public unsafe class PhysXSphereCollider : PhysXCollider
    {
        public float Radius { get; set; } = 0.5f;

        internal override void OnAttached()
        {
            base.OnAttached();
            CreateShape();
        }

        private void CreateShape()
        {
            var geometry = PxSphereGeometry_new(Radius);
            var shape = PxPhysics_createShape_mut(PhysXManager.Physics, &geometry, Material, true, PxShapeFlags.SceneQueryShape | PxShapeFlags.SimulationShape);
            
            AttachShape(shape);
        }
    }

    /// <summary>
    /// Capsule Collider
    /// </summary>
    public unsafe class PhysXCapsuleCollider : PhysXCollider
    {
        public float Radius { get; set; } = 0.5f;
        public float Height { get; set; } = 2.0f;

        internal override void OnAttached()
        {
            base.OnAttached();
            CreateShape();
        }

        private void CreateShape()
        {
            var halfHeight = Height * 0.5f - Radius;
            var geometry = PxCapsuleGeometry_new(Radius, halfHeight);
            var shape = PxPhysics_createShape_mut(PhysXManager.Physics, &geometry, Material, true, PxShapeFlags.SceneQueryShape | PxShapeFlags.SimulationShape);
            
            AttachShape(shape);
        }
    }

    /// <summary>
    /// Plane Collider (for ground planes)
    /// </summary>
    public unsafe class PhysXPlaneCollider : PhysXCollider
    {
        internal override void OnAttached()
        {
            base.OnAttached();
            CreateShape();
        }

        private void CreateShape()
        {
            var geometry = PxPlaneGeometry_new();
            var shape = PxPhysics_createShape_mut(PhysXManager.Physics, &geometry, Material, true, PxShapeFlags.SceneQueryShape | PxShapeFlags.SimulationShape);
            
            AttachShape(shape);
        }
    }

    /// <summary>
    /// Character Controller for player movement
    /// </summary>
    public unsafe class PhysXCharacterController : MonoBehaviour
    {
        private PxController* _controller;
        private PxControllerManager* _manager;

        public float Height { get; set; } = 2.0f;
        public float Radius { get; set; } = 0.5f;
        public float StepOffset { get; set; } = 0.5f;
        public float SlopeLimit { get; set; } = 45.0f;

        protected override void Awake()
        {
            if (!PhysXManager.IsInitialized) return;

            _manager = PxScene_createControllerManager_mut(PhysXManager.Scene, false);
            CreateController();
        }

        private void CreateController()
        {
            var desc = PxCapsuleControllerDesc_new();
            desc->height = Height;
            desc->radius = Radius;
            desc->stepOffset = StepOffset;
            desc->slopeLimit = MathF.Cos(SlopeLimit * MathF.PI / 180f);
            desc->material = PhysXManager.GetDefaultMaterial();
            desc->position.x = transform.position.x;
            desc->position.y = transform.position.y;
            desc->position.z = transform.position.z;

            _controller = PxControllerManager_createController_mut(_manager, (PxControllerDesc*)desc);
        }

        /// <summary>
        /// Move the character controller
        /// </summary>
        public void Move(ECSVector3 motion)
        {
            if (_controller == null) return;

            var displacement = new PxVec3 { x = motion.x, y = motion.y, z = motion.z };
            PxController_move_mut(_controller, &displacement, 0.001f, Time.DeltaTime, null, null);

            // Update transform
            var pos = PxController_getPosition(_controller);
            transform.position = new ECSVector3((float)pos.x, (float)pos.y, (float)pos.z);
        }

        protected override void OnDestroy()
        {
            if (_controller != null) PxController_release_mut(_controller);
            if (_manager != null) PxControllerManager_release_mut(_manager);
        }
    }
}
