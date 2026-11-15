# Chapter 3: Adding Models

## 3.1 Procedural Chamfered Blocks (`blocks.c`)

Everything in *Abandon All Hope*—the log cabin, the rusted pickup truck, the trees, the survivor, and the zombies—is built from **blocks** (`BlockSpec`) batched into Raylib `Mesh` objects.

Why not just call `DrawCube()`?
1. **Chamfered edges**: Every block's edges are cut at 45° (`bevelBox`), creating 44 triangles per block instead of 12. Those thin 45° bevels catch moonlight and muzzle-flash highlights so adjacent dark blocks remain readable from the top-down camera.
2. **Batching**: `BlockBatch` collects hundreds of blocks (walls, floorboards, truck parts) into a single `Mesh` with `vertices`, `normals`, `texcoords`, and per-vertex `colors`, turning an entire log cabin into **one draw call**.
3. **Hand-painted variation**: Each block's color is multiplied by a seeded random shade (`0.9 + rng * 0.2`) so logs and planks look organic rather than flat.

```c
Mesh batch_build_mesh(BlockBatch *b) {
    Mesh mesh = { 0 };
    mesh.vertexCount = b->count;
    mesh.triangleCount = b->count / 3;
    mesh.vertices  = MemAlloc(b->count * 3 * sizeof(float));
    mesh.normals   = MemAlloc(b->count * 3 * sizeof(float));
    mesh.texcoords = MemAlloc(b->count * 2 * sizeof(float));
    mesh.colors    = MemAlloc(b->count * 4 * sizeof(unsigned char));
    /* copy buffers... */
    UploadMesh(&mesh, false);
    return mesh;
}
```

## 3.2 Building the Cabin, Yard, and Forest (`world.c`)

Using `BlockBatch`, `world_init()` constructs:
- **Cabin Floor**: Staggered planks (`0.42` wide), an oval braided rug, loose pages, a tin can, and old bloodstains.
- **Log Walls & Openings**: Interleaved log courses with mortar chinking, end-grain caps, fieldstone footings, two hinged doors, and three paned windows (`openings.c`), plus interior details (pantry shelf with jars/books, deer skull trophy, calendar, coat & hat, mirror).
- **The Yard**: Stacked firewood, chopping block with embedded axe, rusted pickup truck (registered in `solids.c` for collision/pathfinding), flickering porch lantern, broken fence, stumps, and a fallen trunk.
- **The Forest**: 900 trees (pines, spruces, oaks, dead snags) and 1,000 brush props (grass tufts, ferns, rocks) scattered in an annulus around the clearings. Near trees use chamfered blocks; far trees use 12-triangle plain boxes.


---

## Hands-On Examples

- [`examples/ex03_chamfered_mesh.c`](../examples/ex03_chamfered_mesh.c) — Procedural 45° chamfered cuboid mesh with baked vertex colors (`-DBUILD_LESSON_EXAMPLE=ex03_chamfered_mesh`).
- [`examples/ex03_dijkstra_grid.c`](../examples/ex03_dijkstra_grid.c) — Interactive Dijkstra shortest-path routing around rectangular obstacles (`-DBUILD_LESSON_EXAMPLE=ex03_dijkstra_grid`).
