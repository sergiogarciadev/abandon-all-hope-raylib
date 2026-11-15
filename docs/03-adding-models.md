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

### 1. Procedural 45° Chamfered Block: [`ex03_chamfered_mesh.c`](../examples/ex03_chamfered_mesh.c)
```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex03_chamfered_mesh && cmake --build build --target ex03_chamfered_mesh
./build/ex03_chamfered_mesh
```
Shows how `src/core/blocks.c` builds crisp chamfered geometry in memory without `.obj` files:
```c
Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(pb, pa), Vector3Subtract(pc, pa)));
m.vertices[v * 3 + 0] = pts[k].x; /* ... */
m.normals[v * 3 + 0]  = n.x;      /* flat-shaded face normal */
m.colors[v * 4 + 0]   = col.r;    /* baked per-vertex color */
```
After filling 6 main faces, 12 edge chamfers, and 8 corner triangles (44 triangles total), `UploadMesh(&m, false)` sends the mesh to the GPU once.

### 2. Dijkstra Waypoint Routing: [`ex03_dijkstra_grid.c`](../examples/ex03_dijkstra_grid.c)
```bash
cmake -B build -DBUILD_LESSON_EXAMPLE=ex03_dijkstra_grid && cmake --build build --target ex03_dijkstra_grid
./build/ex03_dijkstra_grid
```
Demonstrates the exact waypoint-graph pattern from `src/world/solids.c`:
- Places 4 padded corner nodes around each rectangular obstacle.
- Connects any two nodes where `clear_line(pts[u], pts[v])` does not intersect any box.
- Runs Dijkstra from `start` to `goal` (Left-click to move Goal, Right-click to move Start) and draws the shortest unobstructed path.
