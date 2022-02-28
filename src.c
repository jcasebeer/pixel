#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <string.h>

#include <GLFW/glfw3.h>

#define LEN(a) (sizeof(a)/(sizeof(*a)))

static uint32_t display_width;
static uint32_t display_height;

enum {
    INPUT_UP    = 1,
    INPUT_DOWN  = 2,
    INPUT_LEFT  = 4,
    INPUT_RIGHT = 8,
    INPUT_A     = 16,
    INPUT_B     = 32,
    INPUT_QUIT  = 64
};

static int input_key_map[] = {
    GLFW_KEY_UP,
    GLFW_KEY_DOWN,
    GLFW_KEY_LEFT,
    GLFW_KEY_RIGHT,
    GLFW_KEY_Z,
    GLFW_KEY_X,
    GLFW_KEY_ESCAPE,
    GLFW_KEY_F4
};

struct input {
    uint16_t current;
    uint16_t previous;
};

static struct input _input;

struct input update_input_states(GLFWwindow *window, struct input input)
{
    int i;
    input.previous = input.current; 
    input.current = 0;
    for(i = 0; i<LEN(input_key_map); i++) {
        int key = input_key_map[i];
        if (glfwGetKey(window, key)) {
            input.current |= (1<<i); 
        }
    }
    return input;
}

int key_down(int key)
{
    return _input.current & key;
}

int key_up(int key)
{
    return !(_input.current & key);
}

int key_pressed(int key)
{
    return !(_input.previous & key) && (_input.current & key);
}

int key_released(int key)
{
    return (_input.previous & key) && !(_input.current & key);
}

enum {
    V_WIDTH = 480,
    V_HEIGHT = 270
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    display_width = width;
    display_height = height;
}

GLFWwindow *window_create()
{
    GLFWwindow *window=NULL;

    if (!window) {
        GLFWmonitor *Monitor = glfwGetPrimaryMonitor();

        display_width = V_WIDTH*2;
        display_height = V_HEIGHT*2;
        window = glfwCreateWindow(display_width, display_height, "pixel", NULL, NULL);

        if (!window)
            goto end;

        glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

        glfwSetWindowSizeLimits(window, V_WIDTH, V_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE); 
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        glfwMakeContextCurrent(window);
    }
end:
    return window; 
}

static uint8_t pixels[V_WIDTH*V_HEIGHT*4] = {0};

static void screen_quad_draw(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(V_WIDTH,V_HEIGHT);
    glVertex3f(x+width,y,0);
    glTexCoord2f(0.f,V_HEIGHT);
    glVertex3f(x,y,0);
    glTexCoord2f(0.f,0.f);
    glVertex3f(x,y+height,0);
    glTexCoord2f(V_WIDTH,0.f);
    glVertex3f(x+width,y+height,0);
    glEnd();
}

int main(void)
{
    GLFWwindow* window;
    srand(time(NULL));

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = window_create();
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Set up the pixel buffer */
    GLuint pixel_buffer_id = 0;
    glGenTextures(1, &pixel_buffer_id);
    glBindTexture(GL_TEXTURE_RECTANGLE, pixel_buffer_id);
    glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_RGBA, V_WIDTH, V_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, NULL);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_RECTANGLE,0);

    memset(pixels, -1, sizeof(pixels));

    double start_time = glfwGetTime();
    uint32_t frames = 0;

    int board[V_WIDTH][V_HEIGHT] = {0};
    int count[V_WIDTH][V_HEIGHT] = {0};
    int x,y;

    for(y = 0; y<V_HEIGHT; y++)
        for(x = 0; x<V_WIDTH; x++) 
            board[x][y] = rand() & 1;

    glfwSwapInterval(1);
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        _input = update_input_states(window, _input);
        if (key_released(INPUT_QUIT)) {
            break;
        }

        int x, y;
        for(y=0; y<V_HEIGHT; y++)
            for(x=0; x<V_WIDTH; x++) {
                int xo, yo;
                int sum = 0;
                for(xo = -1; xo<2; xo++)
                    for(yo = -1; yo<2; yo++)
                        if( xo || yo) {
                            if (board[(x+xo+V_WIDTH) % V_WIDTH][(y+yo+V_HEIGHT) % V_HEIGHT])
                                sum++;
                        }
                count[x][y] = sum;
            }

        for(y=0; y<V_HEIGHT; y++)
            for(x=0; x<V_WIDTH; x++) {
                int neighbors = count[x][y];
                int alive = board[x][y];
                int state = 0;
                if (alive) {
                    if (neighbors == 2 || neighbors == 3)
                        state = 1;
                } else {
                    if (neighbors == 3)
                        state = 1;
                }
                board[x][y] = state;
            }

        for(y = 0; y<V_HEIGHT; y++)
            for(x = 0; x<V_WIDTH; x++) {
                int p = (y*V_WIDTH+x)*4;
                if (board[x][y]) {
                    pixels[p++] = 0xcc;
                    pixels[p++] = 0xcc;
                    pixels[p++] = 0xcc;
                } else {
                    pixels[p++] = 0x1f;
                    pixels[p++] = 0x1f;
                    pixels[p++] = 0x1f;
                }
            }

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* upload pixels to texture */
        glBindTexture(GL_TEXTURE_RECTANGLE, pixel_buffer_id);
        glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, V_WIDTH, V_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        /* Draw Pixels */
        glViewport(0, 0, display_width, display_height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, display_width, display_height, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        float width_ratio = (float)display_width / V_WIDTH; 
        float height_ratio = (float)display_height / V_HEIGHT;
        float ratio = width_ratio <= height_ratio ? width_ratio: height_ratio;

        int width = V_WIDTH*ratio; 
        int height= V_HEIGHT*ratio;
        int xc = display_width/2 - width/2;
        int yc= display_height/2 - height/2;

        /*
        glAlphaFunc(GL_GREATER,0.f);
        glEnable(GL_ALPHA_TEST);
        */
        glEnable(GL_TEXTURE_RECTANGLE);
        glBegin(GL_TRIANGLE_FAN);
        screen_quad_draw(xc,yc,width,height); 
        glDisable(GL_TEXTURE_RECTANGLE);
        glBindTexture(GL_TEXTURE_RECTANGLE, 0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        frames++;
        if (frames > 60) {
            double fps = frames / (glfwGetTime() - start_time);
            frames = 0;
            start_time = glfwGetTime();
            printf("%f\n", fps);
        }

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
