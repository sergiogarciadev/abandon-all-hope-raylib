# Chapter 2: Creating an Empty World

## 2.1 The 3D Camera

In *Abandon All Hope*, the gameplay camera sits high above and slightly south of the survivor (`(0, 30, 13)`), looking down at a 55° field of view:

```c
gScene.camera.position = (Vector3){ 0.0f, 30.0f, 13.0f };
gScene.camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
gScene.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
gScene.camera.fovy = 55.0f;
gScene.camera.projection = CAMERA_PERSPECTIVE;
```

Every frame, we call `BeginMode3D(gScene.camera)` ... `EndMode3D()` inside `BeginDrawing()` / `EndDrawing()`.

## 2.2 Custom Lighting & Distance Fog Shader

Raylib's default 3D pipeline is unlit. To reproduce the atmospheric moonlight, warm porch lantern, muzzle-flash point light, and distance fog (`near = 32`, `far = 62`, color `#05060a`), we load a compact GLSL 330 shader directly from memory via `LoadShaderFromMemory(LIT_VS, LIT_FS)`.

The fragment shader computes:
1. **Base color**: `vertexColor * colDiffuse` (multiplied by `texture(texture0, fragTexCoord)` when `useTex == 1`).
2. **Directional moonlight + ambient**: `max(dot(N, moonDir), 0.0) * moonColor + ambientColor`.
3. **Two dynamic point lights**: light `0` for the porch lantern and light `1` for the gun's muzzle flash, with smooth quadratic distance falloff.
4. **Linear distance fog**: `mix(lit, fogColor, clamp((dist - fogNear) / (fogFar - fogNear), 0.0, 1.0))`.

## 2.3 Procedural Forest Floor

Rather than loading external PNGs, we generate a `128x128` pixel-art moss/needle/dirt texture at startup using a deterministic LCG (`SeededRng`) and `ImageDrawRectangle()`:

- `SetTextureWrap(tex, TEXTURE_WRAP_REPEAT)` tiles the texture.
- `SetTextureFilter(tex, TEXTURE_FILTER_POINT)` keeps the crisp blocky aesthetic.
- We scale the `200x200` plane's UVs by `25.0f` and upload the modified texcoord buffer with `UpdateMeshBuffer()`.

With `ClearBackground(gScene.fogColor)` and our fog shader, the ground fades seamlessly into the night!
