#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <omp.h>
#include <unistd.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>

enum { N_OPEN, N_CLOSED, N_IDLE };

enum {
    MOUSE_LEFT,
    MOUSE_RIGHT
};

typedef struct Node Node;

struct Node {
    int w;
    int f;
    int h;
    int g;
    int t;
    int s;
    int i;
    Node *parent;
};

Node *nodes;

int keys[256] = { 0 };

int mouse_down[2];
int mouse_released[2];
int mouse_pressed[2];

float mx = 0.0,
      my = 0.0;

int window_w = 1500,
    window_h = 1000;

enum {
    S_MODEL,
    S_PREPARE,
    S_PLAY,
    S_PAUSE,
    S_DONE
};

int state = S_MODEL;

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

/* Draw square at index */
#define rect2i(i) do { \
    float x = ((i) % NUM_SQUARES_X) * H; \
    float y = ((i) / NUM_SQUARES_X) * H; \
    glRectf(x, y, x + H, y + H); \
} while (0)

enum {
    T_WHITE,
    T_BLACK,
    T_SOURCE,
    T_SINK,
};

GLFWwindow* window;

int *model = NULL;

int index_j,
    index_i;

void clear_model(void);

double walltime(void)
{
    static struct timeval t;
    gettimeofday(&t, NULL);
    return ((double)t.tv_sec + (double)t.tv_usec * 1.0e-06);
}


/*******************************************************************************
 *
 * I/O
 *
 ******************************************************************************/


static void key_callback(GLFWwindow* window, int key, int scancode, int action,
    int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_Q:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_C:
            clear_model();
            break;
        case GLFW_KEY_R:
            state = S_PREPARE;
            break;
        }
    }
}

static void cursor_position_callback(GLFWwindow *window, double x, double y)
{
    mx = x;
    my = y;

    if (mx > window_w)
        mx = window_w - 1;
    else if (mx < 0)
        mx = 0;
    if (my > window_h)
        my = window_h - 1;
    else if (my < 0)
        my = 0;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouse_down[MOUSE_RIGHT] = 1;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse_down[MOUSE_LEFT] = 1;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse_pressed[MOUSE_LEFT] = 1;
    }
    if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouse_down[MOUSE_RIGHT] = 0;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse_down[MOUSE_LEFT] = 0;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse_released[MOUSE_LEFT] = 1;
    }
}

/* Returns 1 once when 'key' is pressed, 0 otherwise */
int is_mouse_pressed(int key)
{
    int key_state = 0;
    switch (key)
    {
    case MOUSE_LEFT:
        key_state = mouse_pressed[MOUSE_LEFT];
        mouse_pressed[MOUSE_LEFT] = 0;
        return key_state;
    }

    return -1;
}

/* Returns 1 once when 'key' is released, 0 otherwise */
int is_mouse_released(int key)
{
    int key_state = 0;
    switch (key)
    {
    case MOUSE_LEFT:
        key_state = mouse_released[MOUSE_LEFT];
        mouse_released[MOUSE_LEFT] = 0;
        return key_state;
    }

    return -1;
}

/* Returns 1 as long as 'key' is down, 0 otherwise */
int is_mouse_down(int key)
{
    switch (key)
    {
    case MOUSE_RIGHT:
        return mouse_down[MOUSE_RIGHT];
    case MOUSE_LEFT:
        return mouse_down[MOUSE_LEFT];
    }

    return -1;
}


/*******************************************************************************
 *
 * Model
 *
 ******************************************************************************/


enum { MODEL_DRAW, MODEL_MOVE };

void clear_model(void)
{
    for (int i = 0; i < NUM_SQUARES; i++)
    {
        int q = model[i];
        if (q != T_SOURCE && q != T_SINK)
            model[i] = T_WHITE;
    }
}

void update_model(void)
{
    static int j = 0;
    static int k = -1;

    j = index2f(mx, my);

    int *q = &model[j];

    /* Capture fixture index */
    if (is_mouse_pressed(MOUSE_LEFT))
    {
        if (*q == T_SOURCE || *q == T_SINK)
            k = j;
    }
    /* Release fixture index */
    else if (is_mouse_released(MOUSE_LEFT))
    {
        k = -1;
    }

    /* Move fixture index */
    if (k >= 0)
    {
        if (*q == T_WHITE)
        {
            *q = model[k];
            model[k] = T_WHITE;
            k = j;

            if (*q == T_SOURCE)
                index_i = k;
            if (*q == T_SINK)
                index_j = k;
        }
    }
    /* Draw/clear blocks */
    else
    {
        if (is_mouse_down(MOUSE_LEFT))
        {
            if (*q == T_SOURCE || *q == T_SINK)
                return;
            *q = T_BLACK;
        }
        else if (is_mouse_down(MOUSE_RIGHT))
        {
            if (*q == T_SOURCE || *q == T_SINK)
                return;
            *q = T_WHITE;
        }
    }
}


/*******************************************************************************
 *
 * View
 *
 ******************************************************************************/


void draw_view(void)
{
    for (int i = 0; i < NUM_SQUARES; i++)
    {
        if (model[i] == T_BLACK)
        {
            glColor3f(0.2, 0.2, 0.2);
            rect2i(i);
        }
        else if (model[i] == T_SOURCE)
        {
            glColor3f(1.0, 0.0, 0.0);
            rect2i(i);
        }
        else if (model[i] == T_SINK)
        {
            glColor3f(0.0, 1.0, 0.0);
            rect2i(i);
        }
        else if (model[i] == T_WHITE)
        {
            glColor3f(1.0, 1.0, 1.0);
            rect2i(i);
        }
        if (state == S_PLAY)
        {
            if (nodes[i].s == N_CLOSED)
            {
                glColor3f(0.0, 0.6, 0.5);
                rect2i(i);
            }
            if (nodes[i].s == N_OPEN)
            {
                glColor3f(0.5, 1.0, 0.5);
                rect2i(i);
            }
        }
    }
}


/*******************************************************************************
 *
 * List
 *
 ******************************************************************************/


typedef struct Bucket Bucket;

typedef struct List List;

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

/* Appends a node to the list */
void append(List *list, Node *node)
{
    static int i = 0;
    Bucket *bucket = (Bucket *)malloc(sizeof(Bucket));
    bucket->node = node;
    bucket->next = NULL;
    bucket->prev = NULL;
    bucket->id = i++;
    if (list->head == NULL)
    {
        list->head = bucket;
        list->tail = bucket;
    }
    else
    {
        list->head->next = bucket;
        bucket->prev = list->head;
        list->head = bucket;
    }
    list->size++;
}

/* Returns the node indetified by 'index' and removes the bucket from the list */
Node * slice_index(List *list, int index)
{
    Bucket *bucket = list->tail;

    int i = 0;

    /* Fetch bucket */
    while (bucket != NULL)
    {
        if (index == i++)
            break;
        bucket = bucket->next;
    }

    if (list->head == NULL || bucket == NULL)
        return NULL;

    list->size--;

    /* Remove bucket */
    if (list->head == bucket)
        list->head = bucket->prev;
    if (list->tail == bucket)
        list->tail = bucket->next;
    if (bucket->next != NULL)
        bucket->next->prev = bucket->prev;
    if (bucket->prev != NULL)
        bucket->prev->next = bucket->next;

    Node *node = bucket->node;

    free(bucket);

    return node;
}

/*  Returns the node identified by 'bucket' and removes the bucket from the list */
Node * slice_bucket(List *list, Bucket *bucket)
{
    if (list->head == NULL || bucket == NULL)
        return NULL;

    list->size--;

    /* Remove bucket */
    if (list->head == bucket)
        list->head = bucket->prev;
    if (list->tail == bucket)
        list->tail = bucket->next;
    if (bucket->next != NULL)
        bucket->next->prev = bucket->prev;
    if (bucket->prev != NULL)
        bucket->prev->next = bucket->next;

    Node *node = bucket->node;

    free(bucket);

    return node;
}

int contains(List *list, Node *node)
{
    Bucket *bucket = list->tail;
    while (bucket != NULL)
    {
        if (bucket->node->i == node->i)
            return 1;
        bucket = bucket->next;
    }
    return 0;
}

void print_list(List *list)
{
    Bucket *bucket = list->tail;
    while (bucket != NULL)
    {
        printf("%d\n", bucket->id);
        bucket = bucket->next;
    }
}

void test_list(void)
{
    for (int i = 0; i < 3; i++)
        append(&open, (Node *)malloc(sizeof(Node)));

    printf("[LIST TEST: DELETE]\n");

    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    slice_index(&open, 0);
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    printf("[LIST TEST: INSERT]\n");

    append(&open, (Node *)malloc(sizeof(Node)));
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    append(&open, (Node *)malloc(sizeof(Node)));
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");

    append(&open, (Node *)malloc(sizeof(Node)));
    print_list(&open);
    printf("%p %p %d\n", open.head, open.tail, open.size); printf("---\n");
}


/*******************************************************************************
 *
 * A*
 *
 ******************************************************************************/


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
    if (u->t == T_WHITE)
        u->s = N_CLOSED;

    Node *neighbors[9] = { NULL };
    get_neighbors(u, neighbors);

    Node *v;
    int i = 0;
    while ((v = neighbors[i++]) != NULL)
    {
        if (v->t != T_WHITE)
             continue;
        if (v->s == N_CLOSED)
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

void init_nodes(void)
{
    for (int i = 0; i < NUM_SQUARES; i++)
    {
        int sx = i % NUM_SQUARES_X;
        int sy = i / NUM_SQUARES_X;
        int tx = index_j % NUM_SQUARES_X;
        int ty = index_j / NUM_SQUARES_X;
        nodes[i] = (Node) {
            .w = 1,
            .f = INT_MAX,
            .g = INT_MAX,
            .h = manhattan(tx - sx, ty - sy),
            .i = i,
            .t = model[i],
            .s = N_IDLE,
            .parent = NULL
        };
    }

    Node *u = &nodes[index_i];
    u->g = 0;
    expand(u);
}


/*******************************************************************************
 *
 * Utils
 *
 ******************************************************************************/


void center_window(GLFWwindow *window)
{
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();

    if (!monitor)
        return;

    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode)
        return;

    int monitor_x, monitor_y;
    glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    glfwSetWindowPos(
        window,
        monitor_x + (mode->width - window_width) / 2,
        monitor_y + (mode->height - window_height) / 2
    );
}

void initialize(void)
{
    /* Configure GLFW */

    if (!glfwInit())
    {
        printf("[ERROR] Failed to initialize glfw\n");
        exit(1);
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window = glfwCreateWindow(window_w, window_h, "GLFW Window",
        NULL, NULL);

    if (!window)
    {
        printf("[ERROR] Failed to initialize window\n");
        exit(1);
    }
    center_window(window);
    glfwSwapInterval(1);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    /* Configure OpenGL */

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    /* Configure model */
    model = (int *)calloc(NUM_SQUARES, sizeof(int));

    int x, y;
    x = NUM_SQUARES_X / 2;
    y = NUM_SQUARES_Y / 2;
    model2i(x - 4, y) = T_SOURCE;
    model2i(x + 4, y) = T_SINK;
    index_i = index2i(x - 4, y);
    index_j = index2i(x + 4, y);

    /* Configure nodes */
    nodes = (Node *)malloc(NUM_SQUARES * sizeof(Node));
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    initialize();

    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &window_w, &window_h);
        glViewport(0, 0, window_w, window_h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, window_w, window_h, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        switch (state)
        {
        case S_MODEL:
            update_model();
            draw_view();
            break;
        case S_PREPARE:
            init_nodes();
            state = S_PLAY;
            break;
        case S_PLAY:
            if (open.size > 0)
            {
                Node *u = extract(&open);
                expand(u);
            }
            draw_view();
            break;
        case S_PAUSE:
            break;
        case S_DONE:
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
