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

#include "astar.h"
#include "list.h"

/* Extern definitions */
Node *nodes = NULL;
int *path = NULL;
int index_i = 0;
int index_j = 0;
int window_h = 1000;
int window_w = 1500;

/* Draw square at index */
#define rect2i(i) do { \
    float x = ((i) % NUM_SQUARES_X) * H; \
    float y = ((i) / NUM_SQUARES_X) * H; \
    glRectf(x, y, x + H, y + H); \
} while (0)

/* Draw circle at index */
#define circle2i(i) do { \
    float x = ((i) % NUM_SQUARES_X) * H + H / 2; \
    float y = ((i) / NUM_SQUARES_X) * H + H / 2; \
    glCirclef(x, y, 9); \
} while (0)

enum {
    MOUSE_LEFT,
    MOUSE_RIGHT
};

enum {
    S_EDIT,
    S_RUN,
    S_PAUSE,
};

int state = S_EDIT;

int mouse_down[2];
int mouse_released[2];
int mouse_pressed[2];

float mx = 0.0,
      my = 0.0;

GLFWwindow *window = NULL;

int *model = NULL;

void clear_obstacles(void);
void clear_search(void);
void init_nodes(int *model);

void glCirclef(float x, float y, float r){
    int n = 20; /* Number of triangles */
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y); /* Circle center */
    for(int i = 0; i <= n; i++) {
        glVertex2f(
            x + (r * cos(i * 2 * PI / n)),
            y + (r * sin(i * 2 * PI / n))
        );
    }
    glEnd();
}

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
        case GLFW_KEY_C:
            if (GLFW_MOD_SHIFT & mods)
                clear_obstacles();
            clear_search();
            state = S_EDIT;
            break;
        case GLFW_KEY_R:
            init_nodes(model);
            state = S_RUN;
            break;
        case GLFW_KEY_P:
            if (state == S_RUN || state == S_PAUSE)
                state = (state == S_PAUSE ? S_RUN : S_PAUSE);
            break;
        case GLFW_KEY_Q:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
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


void clear_obstacles(void)
{
    for (int i = 0; i < NUM_SQUARES; i++)
    {
        int q = model[i];
        if (q != T_SOURCE && q != T_SINK)
            model[i] = T_WHITE;
    }

}

void clear_search(void)
{
    free(path);
    path = NULL;
}

void update_model(void)
{
    static int j = 0;
    static int k = NIL;

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
        k = NIL;
    }

    /* Move fixture index */
    if (k != NIL)
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
    /* Draw blocks */
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


void draw_path(void)
{
    glColor3f(1, 1, 0);
    glLineWidth(2.5f);

    int k = 0;

    glBegin(GL_LINES);

    while (1)
    {
        int i = path[k];
        int j = path[k + 1];
        if (i == NIL || j == NIL)
            break;
        k++;
        float sx = (i % NUM_SQUARES_X) * H + H / 2;
        float sy = (i / NUM_SQUARES_X) * H + H / 2;
        float tx = (j % NUM_SQUARES_X) * H + H / 2;
        float ty = (j / NUM_SQUARES_X) * H + H / 2;
        glVertex2f(sx, sy);
        glVertex2f(tx, ty);
    }

    glEnd();
}

void draw_view(void)
{
    for (int i = 0; i < NUM_SQUARES; i++)
    {
        int tile_type = model[i];

        /* Draw model */
        if (tile_type == T_BLACK)
        {
            glColor3f(0.2, 0.2, 0.2);
            rect2i(i);
        }
        else if (tile_type == T_SOURCE)
        {
            glColor3f(1.0, 0.0, 0.0);
            rect2i(i);
        }
        else if (tile_type == T_SINK)
        {
            glColor3f(0.0, 1.0, 0.0);
            rect2i(i);
        }
        else if (tile_type == T_WHITE)
        {
            glColor3f(1.0, 1.0, 1.0);
            rect2i(i);
        }

        if (tile_type == T_SOURCE || tile_type == T_SINK)
            continue;

        /* Draw search */
        if (state == S_RUN || state == S_PAUSE)
        {
            switch (nodes[i].s)
            {
            case N_OPEN:
                glColor3f(0.0, 0.6, 0.5);
                rect2i(i);
                break;
            case N_CLOSED:
                glColor3f(0.5, 1.0, 0.8);
                rect2i(i);
                break;
            }
        }
    }
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

    /* Allocate model and node memory and place source and sink */

    model = (int *)calloc(NUM_SQUARES, sizeof(int));

    int x, y;
    x = NUM_SQUARES_X / 2;
    y = NUM_SQUARES_Y / 2;
    model2i(x - 4, y) = T_SOURCE;
    model2i(x + 4, y) = T_SINK;
    index_i = index2i(x - 4, y);
    index_j = index2i(x + 4, y);

    nodes = (Node *)malloc(NUM_SQUARES * sizeof(Node));
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    printf(
        "Keys:\n"
        "     'left mouse'  Remove obstacles\n"
        "     'rigth mouse' Place obstacles\n"
        "     'ctrl+c'      Clear everything\n"
        "     'c'           Clear search\n"
        "     'q'           Quit\n"
        "     'r'           Run\n"
        "     'p'           Pause\n"
    );


    initialize();

    while (!glfwWindowShouldClose(window))
    {
        switch (state)
        {
        case S_EDIT:
            update_model();
            break;
        case S_RUN:
            if (open.size > 0)
            {
                Node *u = extract(&open);
                if (model[u->i] == T_SINK)
                {
                    build_path(u);
                    state = S_PAUSE;
                    break;
                }
                expand(u);
            }
            break;
        }

        glfwGetFramebufferSize(window, &window_w, &window_h);
        glViewport(0, 0, window_w, window_h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, window_w, window_h, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_view();
        if (path != NULL)
            draw_path();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
