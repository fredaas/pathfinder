#ifndef ASTAR_H
#define ASTAR_H

#include "common.h"

#include "list.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

float manhattan(int dx, int dy)
{
    return abs(dx) + abs(dy);
}

float euclidean(int dx, int dy)
{
    return sqrt(dx * dx + dy * dy);
}

float chebyshev(int dx, int dy)
{
    return max(dx, dy);
}

Node * extract(List *list)
{
    Bucket *u = list->tail;
    Bucket *v = list->tail;
    while (v != NULL)
    {
        if (v->node->f < u->node->f)
            u = v;
        v = v->next;
    }
    return slice_bucket(list, u);
}

void get_neighbors(Node *node, Node **neighbors)
{
    int x = node->i % NUM_SQUARES_X;
    int y = node->i / NUM_SQUARES_X;
    int j = 0;
    for (int i = 0; i < 9; i++)
    {
        if (i == 4)
            continue;
        int dy = (i / 3) - 1;
        int dx = (i % 3) - 1;
        if (x + dx < 0 || x + dx > NUM_SQUARES_X - 1)
            continue;
        if (y + dy < 0 || y + dy > NUM_SQUARES_Y - 1)
            continue;
        neighbors[j++] = &node2i(x + dx, y + dy);
    }
}

void expand(Node *u)
{
    u->s = N_CLOSED;

    Node *neighbors[9] = { NULL };
    get_neighbors(u, neighbors);

    Node *v;
    int i = 0;
    while ((v = neighbors[i++]) != NULL)
    {
        if (v->s == N_OBSTACLE || v->s == N_CLOSED)
            continue;
        if (u->g + v->w < v->g)
        {
            v->g = u->g + v->w;
            v->f = v->g + v->h;
            v->parent = u;
        }
        if (v->s != N_OPEN)
        {
            v->s = N_OPEN;
            append(&open, v);
        }
    }
}

void init_nodes(int *model)
{
    destroy(&open);

    for (int i = 0; i < NUM_SQUARES; i++)
    {
        int sx = i % NUM_SQUARES_X;
        int sy = i / NUM_SQUARES_X;
        int tx = index_j % NUM_SQUARES_X;
        int ty = index_j / NUM_SQUARES_X;
        int s = (model[i] == T_BLACK ? N_OBSTACLE : N_EXPANDABLE);
        nodes[i] = (Node) {
            .w = 1,
            .f = INT_MAX,
            .g = INT_MAX,
            .h = euclidean(tx - sx, ty - sy),
            .i = i,
            .s = s,
            .parent = NULL
        };
    }

    Node *u = &nodes[index_i];
    u->g = 0;
    expand(u);
}

void build_path(Node *sink)
{
    path = (int *)malloc(NUM_SQUARES * sizeof(int));
    int i = 0;

    Node *node = sink;
    while (node != NULL)
    {
        path[i++] = node->i;
        node = node->parent;
    }

    path[i] = NIL;

    path = (int *)realloc(path, (i + 1) * sizeof(int));
}

#endif /* ASTAR_H */
