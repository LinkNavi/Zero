using System;
using System.Collections.Generic;
using System.Linq;

namespace EngineCore.ECS
{
    // ==================== COMPONENT BASE ====================

    /// <summary>
    /// Base class for all components attached to GameObjects
    /// </summary>
    public abstract class Component
    {
        public GameObject gameObject { get; internal set; }
        public Transform transform => gameObject.transform;
        public bool enabled { get; set; } = true;

        internal virtual void OnAttached() { }
        internal virtual void OnDetached() { }
    }

    // ==================== MONOBEHAVIOUR ====================

    /// <summary>
    /// Base class for user scripts with lifecycle methods (Unity-style)
    /// </summary>
    public abstract class MonoBehaviour : Component
    {
        private bool _started = false;

        // Lifecycle methods for user to override
        protected virtual void Awake() { }
        protected virtual void Start() { }
        protected virtual void Update() { }
        protected virtual void LateUpdate() { }
        protected virtual void OnDestroy() { }
        internal virtual void OnEnable() { }
        internal virtual void OnDisable() { }

        private static CoroutineManager _coroutineManager = new CoroutineManager();
        // Internal lifecycle hooks
        internal void InternalAwake() => Awake();

        internal void InternalStart()
        {
            if (!_started)
            {
                _started = true;
                Start();
            }
        }

        internal void InternalUpdate()
        {
            if (enabled && gameObject.activeInHierarchy)
                Update();
        }

        internal void InternalLateUpdate()
        {
            if (enabled && gameObject.activeInHierarchy)
                LateUpdate();
        }

        internal void InternalOnDestroy() => OnDestroy();

        internal override void OnAttached()
        {
            if (enabled) OnEnable();
        }

        internal override void OnDetached()
        {
            if (enabled) OnDisable();
        }


        public Coroutine StartCoroutine(IEnumerator routine)
        {
            return _coroutineManager.StartCoroutine(routine, this);
        }

        public void StopCoroutine(Coroutine coroutine)
        {
            _coroutineManager.StopCoroutine(coroutine);
        }

        public void StopAllCoroutines()
        {
            _coroutineManager.StopAllCoroutines(this);
        }

        internal static void UpdateCoroutines()
        {
            _coroutineManager.Update();
        }
    }

    // ==================== TRANSFORM ====================

    /// <summary>
    /// Transform component - position, rotation, scale and hierarchy
    /// </summary>
    public class Transform : Component
    {
        private Vector3 _localPosition = Vector3.Zero;
        private Quaternion _localRotation = Quaternion.Identity;
        private Vector3 _localScale = Vector3.One;

        private Transform _parent;
        private readonly List<Transform> _children = new List<Transform>();

        // Local space
        public Vector3 localPosition
        {
            get => _localPosition;
            set => _localPosition = value;
        }

        public Quaternion localRotation
        {
            get => _localRotation;
            set => _localRotation = value;
        }

        public Vector3 localScale
        {
            get => _localScale;
            set => _localScale = value;
        }

        // World space (simplified - would need proper matrix calculations)
        public Vector3 position
        {
            get => _parent == null ? _localPosition : _parent.position + _localPosition;
            set => _localPosition = _parent == null ? value : value - _parent.position;
        }

        public Quaternion rotation
        {
            get => _parent == null ? _localRotation : _parent.rotation * _localRotation;
            set => _localRotation = _parent == null ? value : Quaternion.Inverse(_parent.rotation) * value;
        }

        // Hierarchy
        public Transform parent
        {
            get => _parent;
            set => SetParent(value);
        }

        public int childCount => _children.Count;

        public Transform GetChild(int index) => _children[index];

        public void SetParent(Transform newParent, bool worldPositionStays = true)
        {
            if (_parent == newParent) return;

            Vector3 worldPos = position;
            Quaternion worldRot = rotation;

            _parent?._children.Remove(this);

            _parent = newParent;
            _parent?._children.Add(this);

            if (worldPositionStays && _parent != null)
            {
                position = worldPos;
                rotation = worldRot;
            }
        }
        /// <summary>
        /// Rotate the transform around a point by eulerAngles degrees
        /// </summary>
        public static void Rotate(this Transform transform, float xAngle, float yAngle, float zAngle)
        {
            Quaternion rotation = QuaternionExtensions.Euler(xAngle, yAngle, zAngle);
            transform.localRotation = transform.localRotation * rotation;
        }

        /// <summary>
        /// Rotate the transform by a Vector3 of Euler angles
        /// </summary>
        public static void Rotate(this Transform transform, Vector3 eulerAngles)
        {
            Rotate(transform, eulerAngles.x, eulerAngles.y, eulerAngles.z);
        }

        /// <summary>
        /// Rotate around the given axis by the specified angle in degrees
        /// </summary>
        public static void RotateAround(this Transform transform, Vector3 point, Vector3 axis, float angle)
        {
            Vector3 pos = transform.position;

            // Move to origin relative to point
            Vector3 diff = new Vector3(pos.x - point.x, pos.y - point.y, pos.z - point.z);

            // Create rotation quaternion
            float angleRad = angle * Mathf.Deg2Rad;
            float halfAngle = angleRad * 0.5f;
            float s = MathF.Sin(halfAngle);

            // Normalize axis
            float axisMag = MathF.Sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
            if (axisMag < Mathf.Epsilon) return;

            axis = new Vector3(axis.x / axisMag, axis.y / axisMag, axis.z / axisMag);

            Quaternion rotation = new Quaternion(
                axis.x * s,
                axis.y * s,
                axis.z * s,
                MathF.Cos(halfAngle)
            );

            // Rotate the difference vector
            Vector3 rotatedDiff = rotation * diff;

            // Move back
            transform.position = new Vector3(
                point.x + rotatedDiff.x,
                point.y + rotatedDiff.y,
                point.z + rotatedDiff.z
            );

            // Also rotate the object itself
            transform.rotation = transform.rotation * rotation;
        }

        /// <summary>
        /// Make the transform look at a target position
        /// </summary>
        public static void LookAt(this Transform transform, Vector3 target, Vector3 worldUp)
        {
            Vector3 forward = new Vector3(
                target.x - transform.position.x,
                target.y - transform.position.y,
                target.z - transform.position.z
            );

            transform.rotation = QuaternionExtensions.LookRotation(forward, worldUp);
        }

        /// <summary>
        /// Make the transform look at a target position (default up vector)
        /// </summary>
        public static void LookAt(this Transform transform, Vector3 target)
        {
            LookAt(transform, target, Vector3.Up);
        }

        /// <summary>
        /// Make the transform look at another transform
        /// </summary>
        public static void LookAt(this Transform transform, Transform target)
        {
            LookAt(transform, target.position, Vector3.Up);
        }

        /// <summary>
        /// Translate the transform in the direction and distance of translation
        /// </summary>
        public static void Translate(this Transform transform, Vector3 translation, bool relativeTo = true)
        {
            if (relativeTo)
            {
                // Local space translation
                Vector3 rotated = transform.rotation * translation;
                transform.position = new Vector3(
                    transform.position.x + rotated.x,
                    transform.position.y + rotated.y,
                    transform.position.z + rotated.z
                );
            }
            else
            {
                // World space translation
                transform.position = new Vector3(
                    transform.position.x + translation.x,
                    transform.position.y + translation.y,
                    transform.position.z + translation.z
                );
            }
        }

        /// <summary>
        /// Translate the transform in the direction and distance
        /// </summary>
        public static void Translate(this Transform transform, float x, float y, float z, bool relativeTo = true)
        {
            Translate(transform, new Vector3(x, y, z), relativeTo);
        }

        /// <summary>
        /// Transform a direction from local space to world space
        /// </summary>
        public static Vector3 TransformDirection(this Transform transform, Vector3 direction)
        {
            return transform.rotation * direction;
        }

        /// <summary>
        /// Transform a direction from world space to local space
        /// </summary>
        public static Vector3 InverseTransformDirection(this Transform transform, Vector3 direction)
        {
            return Quaternion.Inverse(transform.rotation) * direction;
        }

        /// <summary>
        /// Transform a point from local space to world space
        /// </summary>
        public static Vector3 TransformPoint(this Transform transform, Vector3 point)
        {
            Vector3 rotated = transform.rotation * new Vector3(
                point.x * transform.localScale.x,
                point.y * transform.localScale.y,
                point.z * transform.localScale.z
            );

            return new Vector3(
                transform.position.x + rotated.x,
                transform.position.y + rotated.y,
                transform.position.z + rotated.z
            );
        }

        /// <summary>
        /// Transform a point from world space to local space
        /// </summary>
        public static Vector3 InverseTransformPoint(this Transform transform, Vector3 point)
        {
            Vector3 diff = new Vector3(
                point.x - transform.position.x,
                point.y - transform.position.y,
                point.z - transform.position.z
            );

            Vector3 rotated = Quaternion.Inverse(transform.rotation) * diff;

            return new Vector3(
                rotated.x / transform.localScale.x,
                rotated.y / transform.localScale.y,
                rotated.z / transform.localScale.z
            );
        }

        /// <summary>
        /// Get the Euler angles of the rotation in degrees
        /// </summary>
        public static Vector3 GetEulerAngles(this Transform transform)
        {
            return QuaternionToEuler(transform.localRotation);
        }

        /// <summary>
        /// Set the rotation using Euler angles in degrees
        /// </summary>
        public static void SetEulerAngles(this Transform transform, float x, float y, float z)
        {
            transform.localRotation = QuaternionExtensions.Euler(x, y, z);
        }

        /// <summary>
        /// Set the rotation using Euler angles vector
        /// </summary>
        public static void SetEulerAngles(this Transform transform, Vector3 eulerAngles)
        {
            transform.localRotation = QuaternionExtensions.Euler(eulerAngles);
        }

        // Helper to convert quaternion to Euler angles
        private static Vector3 QuaternionToEuler(Quaternion q)
        {
            Vector3 angles = Vector3.Zero;

            // Roll (x-axis rotation)
            float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
            float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
            angles.x = MathF.Atan2(sinr_cosp, cosr_cosp);

            // Pitch (y-axis rotation)
            float sinp = 2 * (q.w * q.y - q.z * q.x);
            if (MathF.Abs(sinp) >= 1)
                angles.y = MathF.CopySign(MathF.PI / 2, sinp); // Use 90 degrees if out of range
            else
                angles.y = MathF.Asin(sinp);

            // Yaw (z-axis rotation)
            float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
            float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
            angles.z = MathF.Atan2(siny_cosp, cosy_cosp);

            // Convert to degrees
            angles.x *= Mathf.Rad2Deg;
            angles.y *= Mathf.Rad2Deg;
            angles.z *= Mathf.Rad2Deg;

            return angles;
        }

        // Direction vectors (simplified)
        public Vector3 forward => rotation * Vector3.Forward;
        public Vector3 right => rotation * Vector3.Right;
        public Vector3 up => rotation * Vector3.Up;
    }

    // ==================== GAMEOBJECT ====================

    /// <summary>
    /// GameObject - container for components (Unity-style)
    /// </summary>
    public class GameObject
    {
        private readonly List<Component> _components = new List<Component>();
        private readonly Dictionary<Type, Component> _componentCache = new Dictionary<Type, Component>();

        public string name { get; set; }
        public string tag { get; set; } = "Untagged";
        public int layer { get; set; } = 0;
        public Transform transform { get; private set; }

        private bool _activeSelf = true;
        public bool activeSelf
        {
            get => _activeSelf;
            set
            {
                if (_activeSelf != value)
                {
                    _activeSelf = value;
                    OnActiveChanged();
                }
            }
        }

        public bool activeInHierarchy
        {
            get
            {
                if (!_activeSelf) return false;
                return transform.parent == null || transform.parent.gameObject.activeInHierarchy;
            }
        }

        public GameObject(string name = "GameObject")
        {
            this.name = name;
            transform = AddComponent<Transform>();
        }

        // Component management
        public T AddComponent<T>() where T : Component, new()
        {
            var component = new T { gameObject = this };
            _components.Add(component);
            _componentCache[typeof(T)] = component;

            component.OnAttached();

            if (component is MonoBehaviour mono)
            {
                mono.InternalAwake();
                Scene.Active?.RegisterMonoBehaviour(mono);
            }

            return component;
        }

        public T GetComponent<T>() where T : Component
        {
            if (_componentCache.TryGetValue(typeof(T), out var cached))
                return cached as T;

            var component = _components.OfType<T>().FirstOrDefault();
            if (component != null)
                _componentCache[typeof(T)] = component;

            return component;
        }

        public T[] GetComponents<T>() where T : Component
        {
            return _components.OfType<T>().ToArray();
        }

        public Component GetComponent(Type type)
        {
            if (_componentCache.TryGetValue(type, out var cached))
                return cached;

            var component = _components.FirstOrDefault(c => type.IsInstanceOfType(c));
            if (component != null)
                _componentCache[type] = component;

            return component;
        }

        public T GetComponentInChildren<T>() where T : Component
        {
            var component = GetComponent<T>();
            if (component != null) return component;

            for (int i = 0; i < transform.childCount; i++)
            {
                component = transform.GetChild(i).gameObject.GetComponentInChildren<T>();
                if (component != null) return component;
            }

            return null;
        }

        public void RemoveComponent<T>() where T : Component
        {
            var component = GetComponent<T>();
            if (component != null)
            {
                component.OnDetached();
                _components.Remove(component);
                _componentCache.Remove(typeof(T));

                if (component is MonoBehaviour mono)
                {
                    mono.InternalOnDestroy();
                    Scene.Active?.UnregisterMonoBehaviour(mono);
                }
            }
        }

        // Static factory methods
        public static GameObject Find(string name)
        {
            return Scene.Active?.FindGameObject(name);
        }

        public static GameObject FindWithTag(string tag)
        {
            return Scene.Active?.FindGameObjectWithTag(tag);
        }

        public static GameObject[] FindGameObjectsWithTag(string tag)
        {
            return Scene.Active?.FindGameObjectsWithTag(tag) ?? Array.Empty<GameObject>();
        }

        public void Destroy()
        {
            Scene.Active?.DestroyGameObject(this);
        }

        private void OnActiveChanged()
        {
            foreach (var component in _components.OfType<MonoBehaviour>())
            {
                if (_activeSelf && component.enabled)
                    component.OnEnable();
                else if (!_activeSelf && component.enabled)
                    component.OnDisable();
            }

            // Propagate to children
            for (int i = 0; i < transform.childCount; i++)
            {
                transform.GetChild(i).gameObject.OnActiveChanged();
            }
        }
    }

    // ==================== SCENE ====================

    /// <summary>
    /// Scene - manages all GameObjects
    /// </summary>
    public class Scene
    {
        public static Scene Active { get; private set; }

        private readonly List<GameObject> _gameObjects = new List<GameObject>();
        private readonly List<MonoBehaviour> _behaviours = new List<MonoBehaviour>();
        private readonly List<MonoBehaviour> _behavioursToStart = new List<MonoBehaviour>();
        private readonly HashSet<GameObject> _objectsToDestroy = new HashSet<GameObject>();

        public string name { get; set; }

        public Scene(string name = "Untitled")
        {
            this.name = name;
        }

        public void SetActive()
        {
            Active = this;
        }

        public GameObject CreateGameObject(string name = "GameObject")
        {
            var go = new GameObject(name);
            _gameObjects.Add(go);
            return go;
        }

        public void DestroyGameObject(GameObject gameObject)
        {
            _objectsToDestroy.Add(gameObject);
        }

        internal void RegisterMonoBehaviour(MonoBehaviour behaviour)
        {
            _behaviours.Add(behaviour);
            _behavioursToStart.Add(behaviour);
        }

        internal void UnregisterMonoBehaviour(MonoBehaviour behaviour)
        {
            _behaviours.Remove(behaviour);
            _behavioursToStart.Remove(behaviour);
        }

        public void Update()
        {
            // Start newly added behaviours
            for (int i = _behavioursToStart.Count - 1; i >= 0; i--)
            {
                _behavioursToStart[i].InternalStart();
                _behavioursToStart.RemoveAt(i);
            }

            // Update all behaviours
            foreach (var behaviour in _behaviours)
            {
                behaviour.InternalUpdate();
            }
            MonoBehaviour.UpdateCoroutines();
        }

        public void LateUpdate()
        {
            foreach (var behaviour in _behaviours)
            {
                behaviour.InternalLateUpdate();
            }

            // Process destructions
            foreach (var go in _objectsToDestroy)
            {
                DestroyImmediate(go);
            }
            _objectsToDestroy.Clear();
        }

        private void DestroyImmediate(GameObject gameObject)
        {
            // Destroy all components
            var behaviours = gameObject.GetComponents<MonoBehaviour>();
            foreach (var behaviour in behaviours)
            {
                behaviour.InternalOnDestroy();
                UnregisterMonoBehaviour(behaviour);
            }

            // Remove from scene
            _gameObjects.Remove(gameObject);
        }

        public GameObject FindGameObject(string name)
        {
            return _gameObjects.FirstOrDefault(go => go.name == name);
        }

        public GameObject FindGameObjectWithTag(string tag)
        {
            return _gameObjects.FirstOrDefault(go => go.tag == tag);
        }

        public GameObject[] FindGameObjectsWithTag(string tag)
        {
            return _gameObjects.Where(go => go.tag == tag).ToArray();
        }

        public GameObject[] GetAllGameObjects()
        {
            return _gameObjects.ToArray();
        }
    }

    // ==================== MATH HELPERS (Simplified) ====================

    public struct Vector3
    {
        public float x, y, z;

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static Vector3 Zero => new Vector3(0, 0, 0);
        public static Vector3 One => new Vector3(1, 1, 1);
        public static Vector3 Forward => new Vector3(0, 0, 1);
        public static Vector3 Back => new Vector3(0, 0, -1);
        public static Vector3 Up => new Vector3(0, 1, 0);
        public static Vector3 Down => new Vector3(0, -1, 0);
        public static Vector3 Right => new Vector3(1, 0, 0);
        public static Vector3 Left => new Vector3(-1, 0, 0);

        public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
        public static Vector3 operator *(Vector3 a, float scalar) => new Vector3(a.x * scalar, a.y * scalar, a.z * scalar);
    }

    public struct Quaternion
    {
        public float x, y, z, w;

        public Quaternion(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public static Quaternion Identity => new Quaternion(0, 0, 0, 1);

        public static Quaternion operator *(Quaternion a, Quaternion b)
        {
            return new Quaternion(
                a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
                a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
            );
        }

        public static Vector3 operator *(Quaternion q, Vector3 v)
        {
            // Simplified rotation - proper implementation would be more complex
            return v;
        }

        public static Quaternion Inverse(Quaternion q)
        {
            float norm = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
            return new Quaternion(-q.x / norm, -q.y / norm, -q.z / norm, q.w / norm);
        }
    }
}
