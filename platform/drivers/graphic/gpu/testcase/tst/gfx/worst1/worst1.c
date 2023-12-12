#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <math.h>

#include "vg_lite.h"
#include "vg_lite_util.h"

#define RATEW(x) (128.0 * (x) / 640)
#define RATEH(x) (128.0 * (x) / 480)

static vg_lite_buffer_t buffer, image_buffer;
static vg_lite_buffer_t * fb;

#define CLOCK_RADIUS    10.0f
#define SEC_HAND_LEN    10.0f
#define MIN_HAND_LEN    8
#define HOU_HAND_LEN    6
#define SEC_HAND_WIDTH  2
#define MIN_HAND_WIDTH  3
#define HOU_HAND_WIDTH  4
#define BUTTON_WIDTH    6
#define BUTTON_HEIGHT   3

#define   OPCODE_END                                               0x00
#define   OPCODE_CLOSE                                             0x01
#define   OPCODE_MOVE                                              0x02
#define   OPCODE_MOVE_REL                                          0x03
#define   OPCODE_LINE                                              0x04
#define   OPCODE_LINE_REL                                          0x05
#define   OPCODE_QUADRATIC                                         0x06
#define   OPCODE_QUADRATIC_REL                                     0x07
#define   OPCODE_CUBIC                                             0x08
#define   OPCODE_CUBIC_REL                                         0x09
#define   OPCODE_BREAK                                             0x0A
#define   OPCODE_HLINE                                             0x0B
#define   OPCODE_HLINE_REL                                         0x0C
#define   OPCODE_VLINE                                             0x0D
#define   OPCODE_VLINE_REL                                         0x0E
#define   OPCODE_SQUAD                                             0x0F
#define   OPCODE_SQUAD_REL                                         0x10
#define   OPCODE_SCUBIC                                            0x11
#define   OPCODE_SCUBIC_REL                                        0x12

static vg_lite_path_t path_circle;
static vg_lite_path_t path_hand;
static vg_lite_path_t path_button;

static char path_data[] = {
    2, -10, -10,
    4, 10, -10,
    4, 10, 10,
    4, -10, 10,
    0,
};

static vg_lite_path_t path = {
    {-10, -10, /* left,top */
    10, 10}, /* right,bottom */
    VG_LITE_HIGH, /* quality */
    VG_LITE_S8, /* -128 to 127 coordinate range */
    {0}, /* uploaded */
    sizeof(path_data), /* path length */
    path_data, /* path data */
    1
};

void build_paths()
{
    char    *pchar;
    float   *pfloat;
    int32_t data_size;
    
    /* Path circle */
    /* MOV_TO 
     CUBIC_TO
     CUBIC_TO
     CUBIC_TO
     CUBIC_TO
     */
    data_size = 4 * 5 + 1 + 8 + 8 * 3 * 4;

    memset(&path_circle, 0, sizeof(path_circle));
    path_circle.quality = VG_LITE_HIGH;
    path_circle.format = VG_LITE_FP32;
    path_circle.bounding_box[0] = -CLOCK_RADIUS;
    path_circle.bounding_box[1] = -CLOCK_RADIUS;
    path_circle.bounding_box[2] = CLOCK_RADIUS;
    path_circle.bounding_box[3] = CLOCK_RADIUS;
    path_circle.path_length = data_size;
    path_circle.path = malloc(data_size);

    pchar = (char*)path_circle.path;
    pfloat = (float*)path_circle.path;
    *pchar = OPCODE_MOVE;
    pfloat++;
    *pfloat++ = CLOCK_RADIUS;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_CUBIC;
    pfloat++;
    *pfloat++ = CLOCK_RADIUS;
    *pfloat++ = CLOCK_RADIUS * 0.6f;
    *pfloat++ = CLOCK_RADIUS * 0.6f;
    *pfloat++ = CLOCK_RADIUS;
    *pfloat++ = 0.0f;
    *pfloat++ = CLOCK_RADIUS;

    pchar = (char*)pfloat;
    *pchar = OPCODE_CUBIC;
    pfloat++;
    *pfloat++ = -CLOCK_RADIUS * 0.6f;
    *pfloat++ = CLOCK_RADIUS;
    *pfloat++ = -CLOCK_RADIUS;
    *pfloat++ = CLOCK_RADIUS * 0.6f;
    *pfloat++ = -CLOCK_RADIUS;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_CUBIC;
    pfloat++;
    *pfloat++ = -CLOCK_RADIUS;
    *pfloat++ = -CLOCK_RADIUS * 0.6f;
    *pfloat++ = -CLOCK_RADIUS * 0.6f;
    *pfloat++ = -CLOCK_RADIUS;
    *pfloat++ = 0.0f;
    *pfloat++ = -CLOCK_RADIUS;

    pchar = (char*)pfloat;
    *pchar = OPCODE_CUBIC;
    pfloat++;
    *pfloat++ = CLOCK_RADIUS * 0.6f;
    *pfloat++ = -CLOCK_RADIUS;
    *pfloat++ = CLOCK_RADIUS;
    *pfloat++ = -CLOCK_RADIUS * 0.6f;
    *pfloat++ = CLOCK_RADIUS;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_END;

    /* Path hands. */
    /*
     MOV_TO
     VLINE
     HLINE
     VLINE
     CLOSE
     */
    data_size = 4 * 4 + 1 + 4 * 2 * 4;/*+ 4 * 3; */

    memset(&path_hand, 0, sizeof(path_hand));
    path_hand.quality = VG_LITE_HIGH;
    path_hand.format = VG_LITE_FP32;
    path_hand.bounding_box[0] = 0.0f;
    path_hand.bounding_box[1] = 0.0f;
    path_hand.bounding_box[2] = 1.0f;
    path_hand.bounding_box[3] = 1.0f;
    path_hand.path_length = data_size;
    path_hand.path = malloc(data_size);

    pchar = (char*)path_hand.path;
    pfloat = (float*)path_hand.path;
    *pchar = OPCODE_MOVE;
    pfloat++;
    *pfloat++ = 0.0f;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = 1.0f;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = 1.0f;
    *pfloat++ = 1.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = 0.0f;
    *pfloat++ = 1.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_END;

    /* Path button. */
    /*
     MOV_TO
     LINE
     QUAD
     LINE
     QUAD
     LINE
     QUAD
     LINE
     QUAD
     CLOSE
     */
    data_size = 4 * 9 + 1 + 4 * 2 * 5 + 4 * 4 * 4;/*+ 4 * 3; */

    memset(&path_button, 0, sizeof(path_button));
    path_button.quality = VG_LITE_HIGH;
    path_button.format = VG_LITE_FP32;
    path_button.bounding_box[0] = 0.0f;
    path_button.bounding_box[1] = 0.0f;
    path_button.bounding_box[2] = BUTTON_WIDTH;
    path_button.bounding_box[3] = BUTTON_HEIGHT;
    path_button.path_length = data_size;
    path_button.path = malloc(data_size);

    pchar = (char*)path_button.path;
    pfloat = (float*)path_button.path;
    *pchar = OPCODE_MOVE;
    pfloat++;
    *pfloat++ = BUTTON_WIDTH / 8.0f;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = BUTTON_WIDTH * 7.0f / 8.0f;
    *pfloat++ = 0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_QUADRATIC;
    pfloat++;
    *pfloat++ = BUTTON_WIDTH;
    *pfloat++ = 0.0f;
    *pfloat++ = BUTTON_WIDTH;
    *pfloat++ = BUTTON_HEIGHT / 4.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = BUTTON_WIDTH;
    *pfloat++ = BUTTON_HEIGHT * 3.0f / 4.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_QUADRATIC;
    pfloat++;
    *pfloat++ = BUTTON_WIDTH;
    *pfloat++ = BUTTON_HEIGHT;
    *pfloat++ = BUTTON_WIDTH * 7.0f / 8.0f;
    *pfloat++ = BUTTON_HEIGHT;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = BUTTON_WIDTH / 8.0f;
    *pfloat++ = BUTTON_HEIGHT;

    pchar = (char*)pfloat;
    *pchar = OPCODE_QUADRATIC;
    pfloat++;
    *pfloat++ = 0.0f;
    *pfloat++ = BUTTON_HEIGHT;
    *pfloat++ = 0.0f;
    *pfloat++ = BUTTON_HEIGHT * 3.0f / 4.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_LINE;
    pfloat++;
    *pfloat++ = 0.0f;
    *pfloat++ = BUTTON_HEIGHT / 4.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_QUADRATIC;
    pfloat++;
    *pfloat++ = 0.0f;
    *pfloat++ = 0.0f;
    *pfloat++ = BUTTON_WIDTH / 8.0f;
    *pfloat++ =0.0f;

    pchar = (char*)pfloat;
    *pchar = OPCODE_END;
}

void render_clock(float w, float h)
{
    vg_lite_matrix_t mat;
    int32_t i;
    float   x, y;
    float   angle;

    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
    vg_lite_scale(10.0f, 10.0f, &mat);
    vg_lite_scale(RATEW(w), RATEH(h), &mat);

    vg_lite_draw(fb, &path_circle, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);

    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
    vg_lite_scale(9.5f, 9.5f, &mat);
    vg_lite_scale(RATEW(w), RATEH(h), &mat);

    vg_lite_draw(fb, &path_circle, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0xffcccccc);

    /* Draw ticks. */
    for (i = 0; i < 12; i++)
    {
        angle = i * 3.14159265f / 6;
        x = 80.0f * cos(angle);
        y = 80.0f * sin(angle);
        vg_lite_identity(&mat);
        vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
        vg_lite_translate(x * RATEW(w), y * RATEH(h), &mat);
        vg_lite_rotate(i * 30, &mat);
        vg_lite_scale(2.0f, 1.0f, &mat);
        vg_lite_scale(RATEW(w), RATEH(h), &mat);
        vg_lite_draw(fb, &path_button, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);
    }

    /* Draw hands. */
    /* Hour */
    x = -0.1f;/* * cos(angle); */
    y = -0.5f;/* * sin(angle); */
    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
    vg_lite_rotate(7 * 30, &mat);
    vg_lite_translate(x * RATEW(w) * 40.0f, y * RATEH(h) * 8.0f, &mat);
    vg_lite_scale(40.0f, 8.0f, &mat);
    vg_lite_scale(RATEW(w), RATEH(h), &mat);
    vg_lite_draw(fb, &path_hand, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);

    /* Minute */
    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
    vg_lite_rotate(12 * 6, &mat);
    vg_lite_translate(x * RATEW(w) * 60.0f, y * RATEH(h) * 6.0f, &mat);
    vg_lite_scale(60.0f, 6.0f, &mat);
    vg_lite_scale(RATEW(w), RATEH(h), &mat);
    vg_lite_draw(fb, &path_hand, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);

    /* Second */
    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
    vg_lite_rotate(22 * 6, &mat);
    vg_lite_translate(x * RATEW(w) * 75.0f, y * RATEH(h) * 2.0f, &mat);
    vg_lite_scale(75.0f, 2.0f, &mat);
    vg_lite_scale(RATEW(w), RATEH(h), &mat);
    vg_lite_draw(fb, &path_hand, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);
    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2, fb->height / 2, &mat);
    vg_lite_scale(RATEW(w), RATEH(h), &mat);

    vg_lite_draw(fb, &path_circle, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);
}

void render_buttons(float w, float h)
{
    vg_lite_matrix_t mat;
    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2 - BUTTON_WIDTH * 10 * RATEW(w), fb->height / 2 + 35 * h, &mat);
    vg_lite_scale(10.0f * RATEW(w), 10.0f * RATEH(h), &mat);
    vg_lite_draw(fb, &path_button, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);

    vg_lite_identity(&mat);
    vg_lite_translate(fb->width / 2 + 3.0f, fb->height / 2 + 35 * h, &mat);
    vg_lite_scale(10.0f * RATEW(w), 10.0f * RATEH(h), &mat);
    vg_lite_draw(fb, &path_button, VG_LITE_FILL_EVEN_ODD, &mat, VG_LITE_BLEND_NONE, 0x78cc78ff);
}

void cleanup(void)
{
    if (image_buffer.handle != NULL)
    {
        vg_lite_free(&image_buffer);
    }

    if (buffer.handle != NULL) {
        /* Free the offscreen framebuffer memory. */
        vg_lite_free(&buffer);
    }
    if (path_button.path != NULL) {
        free(path_button.path);
    }
    if (path_circle.path != NULL) {
        free(path_circle.path);
    }
    if (path_hand.path != NULL) {
        free(path_hand.path);
    }
}

int main(int argc, const char * argv[])
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    vg_lite_filter_t filter;
    vg_lite_matrix_t matrix;
    int32_t frames = 1;
    int32_t i;
    float wtimes, htimes;
    long time;

    filter = VG_LITE_FILTER_POINT;

    /* Allocate the buffer. */
    if (argc == 3) {
        buffer.width = atoi(argv[1]);
        buffer.height = atoi(argv[2]);
    } else if (argc == 4){
        buffer.width = atoi(argv[1]);
        buffer.height = atoi(argv[2]);
        frames = atoi(argv[3]);
    } else {
        buffer.width  = RATEW(640);
        buffer.height = RATEH(480);
    }
    /* Initialize the draw. */
    error = vg_lite_init(buffer.width, buffer.height);
    if (error) {
        printf("vg_lite_draw_init() returned error %d\n", error);
        cleanup();
        return -1;
    }
    wtimes = buffer.width / 128.0;
    htimes = buffer.height / 128.0;
    buffer.format = VG_LITE_A8;
    error = vg_lite_allocate(&buffer);
    if (error) {
        printf("vg_lite_allocate() returned error %d\n", error);
        cleanup();
        return -1;
    }
    fb = &buffer;

    build_paths();
    if (!vg_lite_load_png(&image_buffer, "landscape1.png")) {
        printf("vg_lite_load_png() coud not load the image 'landscape1.png'\n");
        cleanup();
        return -1;
    }
    /* Draw the path using the matrix. */
    for (i = 0; i < frames; i++)
    {
        /* Clear the buffer. */
        vg_lite_clear(fb, NULL, 0x7fffffff);
        vg_lite_identity(&matrix);
        vg_lite_scale((vg_lite_float_t) fb->width / (vg_lite_float_t) image_buffer.width,
                      (vg_lite_float_t) fb->height / (vg_lite_float_t) image_buffer.height, &matrix);
        vg_lite_blit(fb, &image_buffer, &matrix, VG_LITE_BLEND_SRC_OVER, 0, filter);
        render_clock(wtimes, htimes);
        render_buttons(wtimes, htimes);
    }
    if (error) {
        printf("vg_lite_draw() returned error %d\n", error);
        cleanup();
        return -1;
    }
    vg_lite_finish();
    /* Save PNG file. */
    vg_lite_save_png("worst_1.png", fb);
    /* Cleanup. */
    cleanup();
    return 0;
}
