# Navary Material System
Navary develop to bring nostalgic feeling of Legacy Material System look and feel.
All features are still in experimental mode and WIP.

## 1. Shader Effect and Pass Management

- Shader Effect: Manages shader stages (vertex, fragment) into a cohesive effect.
- Shader Pass: Represents a rendering pass with a specific shader effect.

## 2. Effect Template

- Gathers Shader Passes for different rendering passes (e.g., forward, shadow).

## 3. Material Properties

- Diffuse Color: Base color.
- Specular Color: Specular highlight color.
- Shininess: Controls the specular highlight size.
- Ambient Color: Ambient lighting color.
- Emissive Color: Emissive color.
- Transparency: Controls the material's transparency.

## 4. Texture Management

Supports a comprehensive set of textures: 
- Diffuse 
- Specular 
- Normal 
- Emissive 
- Ambient Occlusion 
- Roughness 
- Metallic 
- Height 
- Opacity 
- Lightmap.


## 5. Layered Materials

Supports multiple material layers with dynamic blending.

## 6. Dynamic Lighting Enhancements

- Dynamic Shadow Mapping: Can be enabled or disabled.

## 7. Material Caching

- Ensures efficient reuse of materials to optimize performance.


## 8. Dynamic Property and Texture Setting

- Allows selective setting of properties and blending of textures.
- Supports alpha blending for transparency control.

## 9. Shader Compatibility

