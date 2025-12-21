using System;
using System.Collections;
using System.Collections.Generic;
using EngineCore.ECS;

namespace EngineCore
{
    /// <summary>
    /// Coroutine system - allows for delayed and timed code execution
    /// </summary>
    public class CoroutineManager
    {
        private class CoroutineInstance
        {
            public IEnumerator Enumerator;
            public MonoBehaviour Owner;
            public object Current;
        }

        private List<CoroutineInstance> _coroutines = new List<CoroutineInstance>();
        private List<CoroutineInstance> _toAdd = new List<CoroutineInstance>();
        private List<CoroutineInstance> _toRemove = new List<CoroutineInstance>();

        /// <summary>
        /// Start a new coroutine
        /// </summary>
        public Coroutine StartCoroutine(IEnumerator routine, MonoBehaviour owner)
        {
            var instance = new CoroutineInstance
            {
                Enumerator = routine,
                Owner = owner,
                Current = null
            };

            _toAdd.Add(instance);
            return new Coroutine(instance);
        }

        /// <summary>
        /// Stop a running coroutine
        /// </summary>
        public void StopCoroutine(Coroutine coroutine)
        {
            if (coroutine?.Instance != null)
            {
                _toRemove.Add(coroutine.Instance as CoroutineInstance);
            }
        }

        /// <summary>
        /// Stop all coroutines owned by a MonoBehaviour
        /// </summary>
        public void StopAllCoroutines(MonoBehaviour owner)
        {
            for (int i = _coroutines.Count - 1; i >= 0; i--)
            {
                if (_coroutines[i].Owner == owner)
                {
                    _toRemove.Add(_coroutines[i]);
                }
            }
        }

        /// <summary>
        /// Update all running coroutines
        /// </summary>
        public void Update()
        {
            // Add new coroutines
            if (_toAdd.Count > 0)
            {
                _coroutines.AddRange(_toAdd);
                _toAdd.Clear();
            }

            // Update existing coroutines
            for (int i = _coroutines.Count - 1; i >= 0; i--)
            {
                var routine = _coroutines[i];

                // Skip if owner is destroyed or disabled
                if (routine.Owner == null || !routine.Owner.enabled || !routine.Owner.gameObject.activeInHierarchy)
                {
                    _toRemove.Add(routine);
                    continue;
                }

                // Check if we're waiting for something
                if (routine.Current != null)
                {
                    if (routine.Current is WaitForSeconds waitSeconds)
                    {
                        waitSeconds.TimeRemaining -= Time.DeltaTime;
                        if (waitSeconds.TimeRemaining > 0)
                            continue; // Still waiting
                    }
                    else if (routine.Current is WaitUntil waitUntil)
                    {
                        if (!waitUntil.Predicate())
                            continue; // Condition not met
                    }
                    else if (routine.Current is WaitWhile waitWhile)
                    {
                        if (waitWhile.Predicate())
                            continue; // Still waiting
                    }
                    else if (routine.Current is Coroutine nestedCoroutine)
                    {
                        // Wait for nested coroutine
                        if (nestedCoroutine.IsRunning)
                            continue;
                    }
                }

                // Move to next step
                bool hasNext = routine.Enumerator.MoveNext();
                if (hasNext)
                {
                    routine.Current = routine.Enumerator.Current;
                }
                else
                {
                    // Coroutine finished
                    _toRemove.Add(routine);
                }
            }

            // Remove finished coroutines
            if (_toRemove.Count > 0)
            {
                foreach (var routine in _toRemove)
                {
                    _coroutines.Remove(routine);
                }
                _toRemove.Clear();
            }
        }
    }

    /// <summary>
    /// Coroutine handle returned by StartCoroutine
    /// </summary>
    public class Coroutine
    {
        internal object Instance { get; private set; }
        public bool IsRunning { get; internal set; } = true;

        internal Coroutine(object instance)
        {
            Instance = instance;
        }
    }

    /// <summary>
    /// Wait for a specified number of seconds
    /// </summary>
    public class WaitForSeconds
    {
        public float Duration { get; private set; }
        internal float TimeRemaining { get; set; }

        public WaitForSeconds(float seconds)
        {
            Duration = seconds;
            TimeRemaining = seconds;
        }
    }

    /// <summary>
    /// Wait until a condition is true
    /// </summary>
    public class WaitUntil
    {
        public Func<bool> Predicate { get; private set; }

        public WaitUntil(Func<bool> predicate)
        {
            Predicate = predicate;
        }
    }

    /// <summary>
    /// Wait while a condition is true
    /// </summary>
    public class WaitWhile
    {
        public Func<bool> Predicate { get; private set; }

        public WaitWhile(Func<bool> predicate)
        {
            Predicate = predicate;
        }
    }

    /// <summary>
    /// Wait for the end of the frame (essentially yield return null)
    /// </summary>
    public class WaitForEndOfFrame { }

    // Add these methods to MonoBehaviour class in ECS.cs:
    /*
    
    private static CoroutineManager _coroutineManager = new CoroutineManager();
    
    /// <summary>
    /// Start a coroutine
    /// </summary>
    public Coroutine StartCoroutine(IEnumerator routine)
    {
        return _coroutineManager.StartCoroutine(routine, this);
    }
    
    /// <summary>
    /// Stop a specific coroutine
    /// </summary>
    public void StopCoroutine(Coroutine coroutine)
    {
        _coroutineManager.StopCoroutine(coroutine);
    }
    
    /// <summary>
    /// Stop all coroutines on this MonoBehaviour
    /// </summary>
    public void StopAllCoroutines()
    {
        _coroutineManager.StopAllCoroutines(this);
    }
    
    /// <summary>
    /// Update coroutines (call from Scene.Update)
    /// </summary>
    internal static void UpdateCoroutines()
    {
        _coroutineManager.Update();
    }
    
    */
}

// ==================== EXAMPLE USAGE ====================
/*

Example MonoBehaviour using coroutines:

public class CoroutineExample : MonoBehaviour
{
    private Coroutine _blinkCoroutine;
    
    protected override void Start()
    {
        // Start a simple coroutine
        StartCoroutine(PrintMessages());
        
        // Start a blinking coroutine
        _blinkCoroutine = StartCoroutine(BlinkObject());
        
        // Start a coroutine that waits for a condition
        StartCoroutine(WaitForSpaceBar());
    }
    
    private IEnumerator PrintMessages()
    {
        Debug.Log("Starting coroutine...");
        
        yield return new WaitForSeconds(1.0f);
        Debug.Log("1 second passed");
        
        yield return new WaitForSeconds(2.0f);
        Debug.Log("3 seconds passed");
        
        yield return new WaitForSeconds(1.0f);
        Debug.Log("Coroutine finished!");
    }
    
    private IEnumerator BlinkObject()
    {
        while (true)
        {
            gameObject.activeSelf = false;
            yield return new WaitForSeconds(0.5f);
            
            gameObject.activeSelf = true;
            yield return new WaitForSeconds(0.5f);
        }
    }
    
    private IEnumerator WaitForSpaceBar()
    {
        Debug.Log("Press Space to continue...");
        
        yield return new WaitUntil(() => Input.GetKeyDown(KeyCode.Space));
        
        Debug.Log("Space was pressed!");
        
        // Stop the blinking
        StopCoroutine(_blinkCoroutine);
    }
    
    private IEnumerator MoveOverTime(Vector3 target, float duration)
    {
        Vector3 startPos = transform.position;
        float elapsed = 0f;
        
        while (elapsed < duration)
        {
            elapsed += Time.DeltaTime;
            float t = elapsed / duration;
            
            transform.position = Vector3Extensions.Lerp(startPos, target, t);
            
            yield return null; // Wait one frame
        }
        
        transform.position = target; // Ensure we end exactly at target
    }
    
    private IEnumerator ChainedCoroutines()
    {
        Debug.Log("Starting movement sequence...");
        
        // Move to first position
        yield return StartCoroutine(MoveOverTime(new Vector3(5, 0, 0), 2.0f));
        Debug.Log("Reached first position");
        
        // Wait a bit
        yield return new WaitForSeconds(1.0f);
        
        // Move to second position
        yield return StartCoroutine(MoveOverTime(new Vector3(5, 0, 5), 2.0f));
        Debug.Log("Reached second position");
        
        Debug.Log("Movement sequence complete!");
    }
}

*/
