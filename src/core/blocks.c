#include "blocks.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void batch_init(BlockBatch *b) {
    memset(b, 0, sizeof(*b));
}

void batch_free(BlockBatch *b) {
    free(b->pos);
    free(b->nrm);
    free(b->uv);
    free(b->col);
    memset(b, 0, sizeof(*b));
}

static void batch_reserve(BlockBatch *b, int addVerts) {
    if (b->count + addVerts <= b->capacity) return;
    int newCap = b->capacity ? b->capacity * 2 : 1024;
    while (newCap < b->count + addVerts) newCap *= 2;
    b->pos = realloc(b->pos, (size_t)newCap * 3 * sizeof(float));
    b->nrm = realloc(b->nrm, (size_t)newCap * 3 * sizeof(float));
    b->uv  = realloc(b->uv,  (size_t)newCap * 2 * sizeof(float));
    b->col = realloc(b->col, (size_t)newCap * 4 * sizeof(unsigned char));
    b->capacity = newCap;
}

static void push_vert(BlockBatch *b, Vector3 p, Vector3 n, float u, float v, Color c) {
    batch_reserve(b, 1);
    int i = b->count++;
    b->pos[i * 3 + 0] = p.x;
    b->pos[i * 3 + 1] = p.y;
    b->pos[i * 3 + 2] = p.z;
    b->nrm[i * 3 + 0] = n.x;
    b->nrm[i * 3 + 1] = n.y;
    b->nrm[i * 3 + 2] = n.z;
    b->uv[i * 2 + 0]  = u;
    b->uv[i * 2 + 1]  = v;
    b->col[i * 4 + 0] = c.r;
    b->col[i * 4 + 1] = c.g;
    b->col[i * 4 + 2] = c.b;
    b->col[i * 4 + 3] = c.a;
}

static Vector3 xform_point(Vector3 p, const Matrix *m) {
    return (Vector3){
        m->m0 * p.x + m->m4 * p.y + m->m8  * p.z + m->m12,
        m->m1 * p.x + m->m5 * p.y + m->m9  * p.z + m->m13,
        m->m2 * p.x + m->m6 * p.y + m->m10 * p.z + m->m14
    };
}

static Vector3 xform_dir(Vector3 n, const Matrix *m) {
    Vector3 r = {
        m->m0 * n.x + m->m4 * n.y + m->m8  * n.z,
        m->m1 * n.x + m->m5 * n.y + m->m9  * n.z,
        m->m2 * n.x + m->m6 * n.y + m->m10 * n.z
    };
    return Vector3Normalize(r);
}

static void emit_face(BlockBatch *b, const Vector3 *pts, int npts, const Matrix *m, Color c) {
    Vector3 p0 = pts[0];
    Vector3 e1 = Vector3Subtract(pts[1], p0);
    Vector3 e2 = Vector3Subtract(pts[2], p0);
    Vector3 n = Vector3Normalize(Vector3CrossProduct(e1, e2));

    Vector3 localPts[4];
    for (int i = 0; i < npts; i++) localPts[i] = pts[i];

    // Wind outward (the box is convex)
    if (Vector3DotProduct(n, p0) < 0.0f) {
        for (int i = 0; i < npts / 2; i++) {
            Vector3 tmp = localPts[i];
            localPts[i] = localPts[npts - 1 - i];
            localPts[npts - 1 - i] = tmp;
        }
        n = Vector3Negate(n);
    }

    float ax = fabsf(n.x), ay = fabsf(n.y), az = fabsf(n.z);
    int uIdx, vIdx;
    if (ax >= ay && ax >= az) { uIdx = 2; vIdx = 1; }
    else if (ay >= az) { uIdx = 0; vIdx = 2; }
    else { uIdx = 0; vIdx = 1; }

    Vector3 wn = xform_dir(n, m);
    for (int i = 1; i < npts - 1; i++) {
        Vector3 tri[3] = { localPts[0], localPts[i], localPts[i + 1] };
        for (int k = 0; k < 3; k++) {
            float coords[3] = { tri[k].x, tri[k].y, tri[k].z };
            Vector3 wp = xform_point(tri[k], m);
            push_vert(b, wp, wn, coords[uIdx] / UV_UNIT, coords[vIdx] / UV_UNIT, c);
        }
    }
}

static Vector3 bevel_vert(const float e[3], float r, const int s[3], int a) {
    float v[3];
    for (int i = 0; i < 3; i++) {
        v[i] = (float)s[i] * (i == a ? e[i] : e[i] - r);
    }
    return (Vector3){ v[0], v[1], v[2] };
}

static void add_bevel_box(BlockBatch *b, float w, float h, float d, float r, const Matrix *m, Color c) {
    float e[3] = { w * 0.5f, h * 0.5f, d * 0.5f };
    float mr = e[0] * 0.45f;
    if (e[1] * 0.45f < mr) mr = e[1] * 0.45f;
    if (e[2] * 0.45f < mr) mr = e[2] * 0.45f;
    if (r > mr) r = mr;

    const int signs[2] = { 1, -1 };
    const int quad[4][2] = { {1, 1}, {1, -1}, {-1, -1}, {-1, 1} };

    for (int a = 0; a < 3; a++) {
        int bb = (a + 1) % 3;
        int cc = (a + 2) % 3;
        for (int si = 0; si < 2; si++) {
            int sa = signs[si];
            Vector3 fpts[4];
            for (int q = 0; q < 4; q++) {
                int s[3];
                s[a] = sa;
                s[bb] = quad[q][0];
                s[cc] = quad[q][1];
                fpts[q] = bevel_vert(e, r, s, a);
            }
            emit_face(b, fpts, 4, m, c);

            for (int sbi = 0; sbi < 2; sbi++) {
                int sb = signs[sbi];
                int s1[3], s2[3];
                s1[a] = sa; s1[bb] = sb; s1[cc] = 1;
                s2[a] = sa; s2[bb] = sb; s2[cc] = -1;
                Vector3 bpts[4] = {
                    bevel_vert(e, r, s1, a),
                    bevel_vert(e, r, s2, a),
                    bevel_vert(e, r, s2, bb),
                    bevel_vert(e, r, s1, bb)
                };
                emit_face(b, bpts, 4, m, c);
            }
        }
    }

    for (int xi = 0; xi < 2; xi++) {
        for (int yi = 0; yi < 2; yi++) {
            for (int zi = 0; zi < 2; zi++) {
                int s[3] = { signs[xi], signs[yi], signs[zi] };
                Vector3 cpts[3] = {
                    bevel_vert(e, r, s, 0),
                    bevel_vert(e, r, s, 1),
                    bevel_vert(e, r, s, 2)
                };
                emit_face(b, cpts, 3, m, c);
            }
        }
    }
}

static void add_plain_box(BlockBatch *b, float w, float h, float d, const Matrix *m, Color c) {
    float hx = w * 0.5f, hy = h * 0.5f, hz = d * 0.5f;
    const Vector3 faces[6][4] = {
        { { hx, -hy,  hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, { hx,  hy,  hz} },
        { {-hx, -hy, -hz}, {-hx, -hy,  hz}, {-hx,  hy,  hz}, {-hx,  hy, -hz} },
        { {-hx,  hy,  hz}, { hx,  hy,  hz}, { hx,  hy, -hz}, {-hx,  hy, -hz} },
        { {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx, -hy,  hz}, {-hx, -hy,  hz} },
        { {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz} },
        { { hx, -hy, -hz}, {-hx, -hy, -hz}, {-hx,  hy, -hz}, { hx,  hy, -hz} },
    };
    for (int f = 0; f < 6; f++) {
        emit_face(b, faces[f], 4, m, c);
    }
}

Matrix make_transform_yxz(float x, float y, float z, float rx, float ry, float rz) {
    // YXZ Euler order: R = Ry * Rx * Rz
    Matrix mr = MatrixMultiply(MatrixMultiply(MatrixRotateZ(rz), MatrixRotateX(rx)), MatrixRotateY(ry));
    Matrix mt = MatrixTranslate(x, y, z);
    return MatrixMultiply(mr, mt);
}

void batch_add(BlockBatch *b, const BlockSpec *blocks, int n, SeededRng *rng, const Matrix *parent, bool flat) {
    for (int i = 0; i < n; i++) {
        const BlockSpec *blk = &blocks[i];
        Matrix local = make_transform_yxz(blk->x, blk->y, blk->z, blk->rx, blk->ry, blk->rz);
        Matrix m = parent ? MatrixMultiply(local, *parent) : local;
        float shade = rng ? (0.9f + rng_next(rng) * 0.2f) : 1.0f;
        Color c = scale_color(hex_color(blk->color, 255), shade);
        if (flat || blk->bevel <= 0.0f) {
            add_plain_box(b, blk->w, blk->h, blk->d, &m, c);
        } else {
            add_bevel_box(b, blk->w, blk->h, blk->d, blk->bevel, &m, c);
        }
    }
}

Mesh batch_build_mesh(BlockBatch *b) {
    Mesh mesh = { 0 };
    mesh.vertexCount = b->count;
    mesh.triangleCount = b->count / 3;
    mesh.vertices = (float *)MemAlloc((unsigned int)(b->count * 3 * sizeof(float)));
    mesh.normals  = (float *)MemAlloc((unsigned int)(b->count * 3 * sizeof(float)));
    mesh.texcoords = (float *)MemAlloc((unsigned int)(b->count * 2 * sizeof(float)));
    mesh.colors   = (unsigned char *)MemAlloc((unsigned int)(b->count * 4 * sizeof(unsigned char)));
    memcpy(mesh.vertices, b->pos, (size_t)b->count * 3 * sizeof(float));
    memcpy(mesh.normals,  b->nrm, (size_t)b->count * 3 * sizeof(float));
    memcpy(mesh.texcoords, b->uv, (size_t)b->count * 2 * sizeof(float));
    memcpy(mesh.colors,   b->col, (size_t)b->count * 4 * sizeof(unsigned char));
    UploadMesh(&mesh, false);
    return mesh;
}

Mesh mesh_from_blocks(const BlockSpec *blocks, int n, SeededRng *rng, bool flat) {
    BlockBatch b;
    batch_init(&b);
    batch_add(&b, blocks, n, rng, NULL, flat);
    Mesh m = batch_build_mesh(&b);
    batch_free(&b);
    return m;
}

Model model_from_blocks(const BlockSpec *blocks, int n, SeededRng *rng, bool flat, Shader shader) {
    Mesh mesh = mesh_from_blocks(blocks, n, rng, flat);
    Model model = LoadModelFromMesh(mesh);
    model.materials[0].shader = shader;
    return model;
}
