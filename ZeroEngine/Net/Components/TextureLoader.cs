using System;
using System.IO;
using OpenTK.Mathematics;
using OpenTK.Graphics.OpenGL4;
using EngineCore.Rendering;
using StbImageSharp;

namespace EngineCore.Components
{
    /// <summary>
    /// Static utility class for loading textures from files
    /// Supports: PNG, JPG, BMP, TGA, GIF
    /// </summary>
    public static class TextureLoader
    {
        public static Texture LoadFromFile(string path)
        {
            if (!File.Exists(path))
                throw new FileNotFoundException($"Texture file not found: {path}");

            // Load image using StbImageSharp
            StbImage.stbi_set_flip_vertically_on_load(1);
            
            using var stream = File.OpenRead(path);
            ImageResult image = ImageResult.FromStream(stream, ColorComponents.RedGreenBlueAlpha);
            
            if (image == null)
                throw new Exception($"Failed to load texture: {path}");

            // Create OpenGL texture
            int handle = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, handle);

            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba,
                image.Width, image.Height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, image.Data);

            // Set texture parameters
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.LinearMipmapLinear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)TextureWrapMode.Repeat);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)TextureWrapMode.Repeat);

            // Generate mipmaps
            GL.GenerateMipmap(GenerateMipmapTarget.Texture2D);

            GL.BindTexture(TextureTarget.Texture2D, 0);

            Console.WriteLine($"Loaded texture: {path} ({image.Width}x{image.Height})");
            return new Texture(handle, image.Width, image.Height);
        }

        public static Texture CreateSolidColor(Color4 color, int width = 1, int height = 1)
        {
            byte[] pixels = new byte[width * height * 4];
            for (int i = 0; i < width * height; i++)
            {
                pixels[i * 4 + 0] = (byte)(color.R * 255);
                pixels[i * 4 + 1] = (byte)(color.G * 255);
                pixels[i * 4 + 2] = (byte)(color.B * 255);
                pixels[i * 4 + 3] = (byte)(color.A * 255);
            }

            int handle = GL.GenTexture();
            GL.BindTexture(TextureTarget.Texture2D, handle);
            GL.TexImage2D(TextureTarget.Texture2D, 0, PixelInternalFormat.Rgba,
                width, height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, pixels);
            
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)TextureMinFilter.Linear);
            GL.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)TextureMagFilter.Linear);
            
            GL.BindTexture(TextureTarget.Texture2D, 0);

            return new Texture(handle, width, height);
        }
    }
}
