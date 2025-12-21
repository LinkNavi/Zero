

using System;
using OpenTK.Mathematics;
using EngineCore.ECS;

namespace EngineCore.UI
{
    /// <summary>
    /// Interactive button component
    /// </summary>
    public class UIButton : MonoBehaviour
    {
        public Color4 normalColor = new Color4(1, 1, 1, 1);
        public Color4 hoverColor = new Color4(0.9f, 0.9f, 0.9f, 1);
        public Color4 pressedColor = new Color4(0.7f, 0.7f, 0.7f, 1);
        public Color4 disabledColor = new Color4(0.5f, 0.5f, 0.5f, 0.5f);

        private UIImage _image;
        private bool _isHovered = false;
        private bool _isPressed = false;

        // Events
        public Action OnClick;
        public Action OnHoverEnter;
        public Action OnHoverExit;

        protected override void Start()
        {
            _image = gameObject.GetComponent<UIImage>();
        }

        protected override void Update()
        {
            if (!enabled) return;

            // Update visual state
            if (_image != null)
            {
                if (!enabled)
                    _image.color = disabledColor;
                else if (_isPressed)
                    _image.color = pressedColor;
                else if (_isHovered)
                    _image.color = hoverColor;
                else
                    _image.color = normalColor;
            }
        }

        public void SetHovered(bool hovered)
        {
            if (_isHovered != hovered)
            {
                _isHovered = hovered;
                if (hovered)
                    OnHoverEnter?.Invoke();
                else
                    OnHoverExit?.Invoke();
            }
        }

        public void SetPressed(bool pressed)
        {
            _isPressed = pressed;
            if (!pressed && _isHovered && enabled)
            {
                OnClick?.Invoke();
            }
        }
    }
}
