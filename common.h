#ifndef COMMON_H
#define COMMON_H

/* Grid coefficient */
#define H 20
#define NUM_SQUARES (window_w * window_h / (H * H))
#define NUM_SQUARES_X (window_w / H)
#define NUM_SQUARES_Y (window_h / H)

/* Translates screen coordinates (x, y) into H-index */
#define index2f(x, y) \
   ((int)((y) / H) * NUM_SQUARES_X + (int)((x) / H))

/* Translates H-coordinates (x, y) into H-index */
#define index2i(x, y) \
    (y) * NUM_SQUARES_X + (x)

/* Returns the model element at screen coordinates (x, y) */
#define model2f(x, y) \
    model[(int)((y) / H) * NUM_SQUARES_X + (int)((x) / H)]

/* Returns the model element at screen H-coordinates (x, y) */
#define model2i(x, y) \
    model[(y) * NUM_SQUARES_X + (x)]

/* Returns the node element at screen H-coordinates (x, y) */
#define node2i(x, y) \
    nodes[(y) * NUM_SQUARES_X + (x)]

#define PI (float)3.141592654
#define NIL -1

typedef struct Node Node;
typedef struct Bucket Bucket;
typedef struct List List;

struct Node {
    float w;
    float f;
    float h;
    float g;
    int s;
    int i;
    Node *parent;
};

/* List orientation: tail -> 0 <-> 1 <-> ... <-> N <- head */
struct List {
    Bucket *head;
    Bucket *tail;
    int size;
} open;

struct Bucket {
    Node *node;
    Bucket *next;
    Bucket *prev;
    int id;
};

extern Node *nodes;
extern int *path;
extern int index_i; /* Source */
extern int index_j; /* Sink */
extern int window_h;
extern int window_w;

enum {
    N_OPEN,
    N_CLOSED,
    N_EXPANDABLE,
    N_OBSTACLE
};

enum {
    T_WHITE,
    T_BLACK,
    T_SOURCE,
    T_SINK,
};

#endif /* COMMON_H */
