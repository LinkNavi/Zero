using System;
using System.Collections.Generic;
using EngineCore.ECS;
using ECSVector3 = EngineCore.ECS.Vector3;

namespace EngineCore.Scripting
{
    /// <summary>
    /// Base class for all user scripts - Unity/PlayCanvas style
    /// Inherit from this to create gameplay scripts
    /// </summary>
    public abstract class ScriptBase
    {
        // Reference to the script component
        internal ScriptComponent _component;

        // Convenience properties
        public GameObject gameObject => _component?.gameObject;
        public Transform transform => _component?.transform;
        public bool enabled { get => _component?.enabled ?? false; set { if (_component != null) _component.enabled = value; } }

        // Lifecycle methods - override these in your script
        public virtual void OnStart() { }
        public virtual void OnUpdate() { }
        public virtual void OnLateUpdate() { }
        public virtual void OnDestroy() { }

        // Utility methods available to all scripts
        protected T GetComponent<T>() where T : Component => gameObject?.GetComponent<T>();
        protected GameObject FindGameObject(string name) => GameObject.Find(name);
        protected void Log(string message) => Console.WriteLine($"[{GetType().Name}] {message}");
        protected void LogWarning(string message) => Console.WriteLine($"[{GetType().Name}] WARNING: {message}");
        protected void LogError(string message) => Console.WriteLine($"[{GetType().Name}] ERROR: {message}");
    }

    /// <summary>
    /// Enhanced ScriptComponent that hosts user-defined script classes
    /// </summary>
    public class ScriptComponent : MonoBehaviour
    {
        private ScriptBase _scriptInstance;
        private Type _scriptType;

        public ScriptComponent() { }

        /// <summary>
        /// Set the script class type to use
        /// </summary>
        public void SetScriptType<T>() where T : ScriptBase, new()
        {
            _scriptType = typeof(T);
            _scriptInstance = new T();
            _scriptInstance._component = this;
        }

        /// <summary>
        /// Set script from type (for dynamic loading)
        /// </summary>
        public void SetScriptType(Type type)
        {
            if (!typeof(ScriptBase).IsAssignableFrom(type))
            {
                Console.WriteLine($"ERROR: {type.Name} does not inherit from ScriptBase");
                return;
            }

            _scriptType = type;
            _scriptInstance = (ScriptBase)Activator.CreateInstance(type);
            _scriptInstance._component = this;
        }

        /// <summary>
        /// Get the script instance
        /// </summary>
        public T GetScript<T>() where T : ScriptBase => _scriptInstance as T;

        protected override void Start()
        {
            _scriptInstance?.OnStart();
        }

        protected override void Update()
        {
            if (enabled)
                _scriptInstance?.OnUpdate();
        }

        protected override void LateUpdate()
        {
            if (enabled)
                _scriptInstance?.OnLateUpdate();
        }

        protected override void OnDestroy()
        {
            _scriptInstance?.OnDestroy();
        }
    }

    // ==================== EXAMPLE SCRIPTS ====================

    /// <summary>
    /// Example: Player controller script - Unity/PlayCanvas style
    /// </summary>
    public class PlayerController : ScriptBase
    {
        // Public fields (can be set from editor/code)
        public float moveSpeed = 5f;
        public float jumpForce = 8f;
        public float gravity = 20f;

        // Private state
        private float _velocityY = 0f;
        private bool _isGrounded = true;

        public override void OnStart()
        {
            Log("Player controller initialized!");
        }

        public override void OnUpdate()
        {
            // Get input
            float h = Input.Input.GetAxisHorizontal();
            float v = Input.Input.GetAxisVertical();

            // Move
            if (h != 0 || v != 0)
            {
                ECSVector3 movement = new ECSVector3(
                    h * moveSpeed * Time.DeltaTime,
                    0,
                    v * moveSpeed * Time.DeltaTime
                );
                transform.Translate(movement, false);
            }

            // Jump
            if (Input.Input.GetKeyDown(Input.KeyCode.Space) && _isGrounded)
            {
                _velocityY = jumpForce;
                _isGrounded = false;
                Log("Jump!");
            }

            // Apply gravity
            if (!_isGrounded)
            {
                _velocityY -= gravity * Time.DeltaTime;
                ECSVector3 pos = transform.position;
                pos.y += _velocityY * Time.DeltaTime;
                transform.position = pos;

                // Ground check
                if (pos.y <= 0.5f)
                {
                    pos.y = 0.5f;
                    transform.position = pos;
                    _velocityY = 0;
                    _isGrounded = true;
                }
            }
        }

        public override void OnDestroy()
        {
            Log("Player destroyed");
        }
    }

    /// <summary>
    /// Example: Enemy AI script
    /// </summary>
    public class EnemyAI : ScriptBase
    {
        public float detectRange = 8f;
        public float attackRange = 3f;
        public float moveSpeed = 2.5f;
        public Transform target;

        private enum State { Idle, Chase, Attack }
        private State _state = State.Idle;
        private float _attackTimer = 0f;

        public override void OnStart()
        {
            Log("Enemy AI started!");
            // Find player
            var playerObj = FindGameObject("Player");
            if (playerObj != null)
                target = playerObj.transform;
        }

        public override void OnUpdate()
        {
            if (target == null) return;

            float distance = Vector3Extensions.Distance(transform.position, target.position);

            switch (_state)
            {
                case State.Idle:
                    IdleBehavior();
                    if (distance < detectRange)
                    {
                        _state = State.Chase;
                        Log("Player detected!");
                    }
                    break;

                case State.Chase:
                    ChasePlayer(distance);
                    if (distance < attackRange)
                    {
                        _state = State.Attack;
                        Log("In attack range!");
                    }
                    else if (distance > detectRange * 1.5f)
                    {
                        _state = State.Idle;
                        Log("Lost player");
                    }
                    break;

                case State.Attack:
                    AttackPlayer();
                    if (distance > attackRange * 1.2f)
                    {
                        _state = State.Chase;
                    }
                    break;
            }
        }

        private void IdleBehavior()
        {
            // Bob up and down
            float bobAmount = MathF.Sin(Time.TotalTime * 2f) * 0.2f;
            ECSVector3 pos = transform.position;
            pos.y = 1f + bobAmount;
            transform.position = pos;
        }

        private void ChasePlayer(float distance)
        {
            if (distance > 0.1f)
            {
                // Move toward player
                ECSVector3 direction = new ECSVector3(
                    target.position.x - transform.position.x,
                    0,
                    target.position.z - transform.position.z
                );
                
                float magnitude = MathF.Sqrt(direction.x * direction.x + direction.z * direction.z);
                direction.x /= magnitude;
                direction.z /= magnitude;

                transform.Translate(new ECSVector3(
                    direction.x * moveSpeed * Time.DeltaTime,
                    0,
                    direction.z * moveSpeed * Time.DeltaTime
                ), false);
            }
        }

        private void AttackPlayer()
        {
            _attackTimer += Time.DeltaTime;
            if (_attackTimer >= 1f)
            {
                Log("Enemy attacks!");
                _attackTimer = 0f;
            }
        }
    }

    /// <summary>
    /// Example: Rotating object script
    /// </summary>
    public class Rotator : ScriptBase
    {
        public float rotationSpeed = 90f; // degrees per second
        public ECSVector3 axis = ECSVector3.Up;

        public override void OnUpdate()
        {
            float rotation = rotationSpeed * Time.DeltaTime;
            transform.Rotate(axis.x * rotation, axis.y * rotation, axis.z * rotation);
        }
    }

    /// <summary>
    /// Example: Follow camera script
    /// </summary>
    public class FollowCamera : ScriptBase
    {
        public Transform target;
        public ECSVector3 offset = new ECSVector3(0, 5, 10);
        public float smoothSpeed = 5f;

        public override void OnLateUpdate()
        {
            if (target == null) return;

            ECSVector3 desiredPosition = new ECSVector3(
                target.position.x + offset.x,
                target.position.y + offset.y,
                target.position.z + offset.z
            );

            transform.position = Vector3Extensions.Lerp(
                transform.position,
                desiredPosition,
                smoothSpeed * Time.DeltaTime
            );

            transform.LookAt(target.position);
        }
    }

    /// <summary>
    /// Example: Collectible item script
    /// </summary>
    public class Collectible : ScriptBase
    {
        public float rotationSpeed = 180f;
        public float bobSpeed = 2f;
        public float bobAmount = 0.3f;

        private float _startY;

        public override void OnStart()
        {
            _startY = transform.position.y;
        }

        public override void OnUpdate()
        {
            // Rotate
            transform.Rotate(0, rotationSpeed * Time.DeltaTime, 0);

            // Bob up and down
            float newY = _startY + MathF.Sin(Time.TotalTime * bobSpeed) * bobAmount;
            ECSVector3 pos = transform.position;
            pos.y = newY;
            transform.position = pos;
        }
    }
}
