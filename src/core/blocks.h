#ifndef BLOCKS_H
#define BLOCKS_H

#include "raylib.h"
#include "math_util.h"
#include <stdbool.h>
#include <stdint.h>

#define UV_UNIT 0.32f

typedef struct {
    float w, h, d;
    float x, y, z;
    uint32_t color;
    float rx, ry, rz;
    float bevel; // <= 0 means plain 12-triangle box
} BlockSpec;

#define BLK(w, h, d, x, y, z, col) (BlockSpec){ (w), (h), (d), (x), (y), (z), (col), 0, 0, 0, 0.025f }
#define BLKR(w, h, d, x, y, z, col, rx, ry, rz) (BlockSpec){ (w), (h), (d), (x), (y), (z), (col), (rx), (ry), (rz), 0.025f }
#define BLKB(w, h, d, x, y, z, col, rx, ry, rz, bev) (BlockSpec){ (w), (h), (d), (x), (y), (z), (col), (rx), (ry), (rz), (bev) }

typedef struct {
    float *pos;
    float *nrm;
    float *uv;
    unsigned char *col;
    int count;    // vertex count
    int capacity; // vertex capacity
} BlockBatch;

void batch_init(BlockBatch *b);
void batch_free(BlockBatch *b);
void batch_add(BlockBatch *b, const BlockSpec *blocks, int n, SeededRng *rng, const Matrix *parent, bool flat);
Mesh batch_build_mesh(BlockBatch *b);
Model model_from_blocks(const BlockSpec *blocks, int n, SeededRng *rng, bool flat, Shader shader);
Mesh mesh_from_blocks(const BlockSpec *blocks, int n, SeededRng *rng, bool flat);

Matrix make_transform_yxz(float x, float y, float z, float rx, float ry, float rz);

#endif // BLOCKS_H
