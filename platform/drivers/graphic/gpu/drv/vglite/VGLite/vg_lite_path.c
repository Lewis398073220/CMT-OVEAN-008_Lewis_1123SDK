/****************************************************************************
*
*    Copyright 2012 - 2022 Vivante Corporation, Santa Clara, California.
*    All Rights Reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining
*    a copy of this software and associated documentation files (the
*    'Software'), to deal in the Software without restriction, including
*    without limitation the rights to use, copy, modify, merge, publish,
*    distribute, sub license, and/or sell copies of the Software, and to
*    permit persons to whom the Software is furnished to do so, subject
*    to the following conditions:
*
*    The above copyright notice and this permission notice (including the
*    next paragraph) shall be included in all copies or substantial
*    portions of the Software.
*
*    THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
*    IN NO EVENT SHALL VIVANTE AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
*    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#include "vg_lite_context.h"

/* Path data operations. */
#define CDALIGN(value, by) (((value) + (by) - 1) & ~((by) - 1))
#define CDMIN(x, y) ((x) > (y) ? (y) : (x))
#define CDMAX(x, y) ((x) > (y) ? (x) : (y))

static int32_t get_data_count(uint8_t cmd)
{
    static int32_t count[] = {
        0,
        0,
        2,
        2,
        2,
        2,
        4,
        4,
        6,
        6,
        5,
        5,
        5,
        5,
        5,
        5,
        5,
        5
    };

    if (cmd > VLC_OP_LCWARC_REL) {
        return -1;
    }
    else {
        return count[cmd];
    }
}

static void compute_pathbounds(float* xmin, float* ymin, float* xmax, float* ymax, float x, float y)
{
    if (xmin != NULL)
    {
        *xmin = *xmin < x ? *xmin : x;
    }

    if (xmax != NULL)
    {
        *xmax = *xmax > x ? *xmax : x;
    }

    if (ymin != NULL)
    {
        *ymin = *ymin < y ? *ymin : y;
    }

    if (ymax != NULL)
    {
        *ymax = *ymax > y ? *ymax : y;
    }
}

static int32_t get_data_size(vg_lite_format_t format)
{
    int32_t data_size = 0;

    switch (format) {
    case VG_LITE_S8:
        data_size = sizeof(int8_t);
        break;

    case VG_LITE_S16:
        data_size = sizeof(int16_t);
        break;

    case VG_LITE_S32:
        data_size = sizeof(int32_t);
        break;

    default:
        data_size = sizeof(vg_lite_float_t);
        break;
    }

    return data_size;
}

vg_lite_error_t vg_lite_init_path(vg_lite_path_t* path,
    vg_lite_format_t data_format,
    vg_lite_quality_t quality,
    vg_lite_uint32_t path_length,
    vg_lite_pointer path_data,
    vg_lite_float_t min_x, vg_lite_float_t min_y,
    vg_lite_float_t max_x, vg_lite_float_t max_y)
{
    if (path == NULL)
        return VG_LITE_INVALID_ARGUMENT;

    memset(path, 0, sizeof(*path));
    path->format = data_format;
    path->quality = quality;
    path->bounding_box[0] = min_x;
    path->bounding_box[1] = min_y;
    path->bounding_box[2] = max_x;
    path->bounding_box[3] = max_y;

    path->path_length = path_length;
    path->path = path_data;

    path->path_changed = 1;
    path->uploaded.address = 0;
    path->uploaded.bytes = 0;
    path->uploaded.handle = NULL;
    path->uploaded.memory = NULL;
    path->pdata_internal = 0;

    /* Default FILL path type*/
    path->path_type = VG_LITE_DRAW_FILL_PATH;

    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_set_path_type(vg_lite_path_t* path, vg_lite_path_type_t path_type)
{
    if (!path ||
        (path_type != VG_LITE_DRAW_FILL_PATH &&
         path_type != VG_LITE_DRAW_STROKE_PATH &&
         path_type != VG_LITE_DRAW_FILL_STROKE_PATH)
       )
        return VG_LITE_INVALID_ARGUMENT;

    path->path_type = path_type;

    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_clear_path(vg_lite_path_t* path)
{
    vg_lite_error_t error;
    if (path->uploaded.handle != NULL)
    {
        vg_lite_kernel_free_t free_cmd;
        free_cmd.memory_handle = path->uploaded.handle;
        error = vg_lite_kernel(VG_LITE_FREE, &free_cmd);
        if (error != VG_LITE_SUCCESS)
            return error;
    }

    path->uploaded.address = 0;
    path->uploaded.bytes = 0;
    path->uploaded.handle = NULL;
    path->uploaded.memory = NULL;

    if (path->pdata_internal == 1 && path->path != NULL) {
        vg_lite_os_free(path->path);
    }
    path->path = NULL;

    if (path->stroke_path) {
        vg_lite_os_free(path->stroke_path);
        path->stroke_path = NULL;
    }

#if gcFEATURE_VG_STROKE_PATH
    if (path->stroke) {
        if (path->stroke->path_points) {
            vg_lite_path_point_ptr temp_point;
            while (path->stroke->path_points) {
                temp_point = path->stroke->path_points->next;
                vg_lite_os_free(path->stroke->path_points);
                path->stroke->path_points = temp_point;
            }
            temp_point = NULL;
        }

        if (path->stroke->stroke_paths) {
            vg_lite_sub_path_ptr temp_sub_path;
            while (path->stroke->stroke_paths) {
                temp_sub_path = path->stroke->stroke_paths->next;
                if (path->stroke->stroke_paths->point_list) {
                    vg_lite_path_point_ptr temp_point;
                    while (path->stroke->stroke_paths->point_list) {
                        temp_point = path->stroke->stroke_paths->point_list->next;
                        vg_lite_os_free(path->stroke->stroke_paths->point_list);
                        path->stroke->stroke_paths->point_list = temp_point;
                    }
                    temp_point = NULL;
                }
                vg_lite_os_free(path->stroke->stroke_paths);
                path->stroke->stroke_paths = temp_sub_path;
            }
            temp_sub_path = NULL;
        }

        vg_lite_os_free(path->stroke);
        path->stroke = NULL;
    }
#endif

    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_upload_path(vg_lite_path_t * path)
{
    uint32_t bytes;
    vg_lite_buffer_t Buf, *buffer;
    buffer = &Buf;

    /* Compute the number of bytes required for path + command buffer prefix/postfix. */
    bytes = (8 + path->path_length + 7 + 8) & ~7;

    /* Allocate GPU memory. */
    buffer->width  = bytes;
    buffer->height = 1;
    buffer->stride = 0;
    buffer->format = VG_LITE_A8;
    if (vg_lite_allocate(buffer) != VG_LITE_SUCCESS) {
        return VG_LITE_OUT_OF_MEMORY;
    }

    /* Initialize command buffer prefix. */
    ((uint32_t *) buffer->memory)[0] = 0x40000000 | ((path->path_length + 7) / 8);
    ((uint32_t *) buffer->memory)[1] = 0;

    /* Copy the path data. */
    memcpy((uint32_t *) buffer->memory + 2, path->path, path->path_length);

    /* Initialize command buffer postfix. */
    ((uint32_t *) buffer->memory)[bytes / 4 - 2] = 0x70000000;
    ((uint32_t *) buffer->memory)[bytes / 4 - 1] = 0;

    /* Mark path as uploaded. */
    path->path = buffer->memory;
    path->uploaded.handle = buffer->handle;
    path->uploaded.address = buffer->address;
    path->uploaded.memory = buffer->memory;
    path->uploaded.bytes = bytes;
    path->path_changed = 0;
    VLM_PATH_ENABLE_UPLOAD(*path);      /* Implicitly enable path uploading. */

    /* Return pointer to vg_lite_buffer structure. */
    return VG_LITE_SUCCESS;
}

vg_lite_uint32_t vg_lite_get_path_length(vg_lite_uint8_t *cmd, vg_lite_uint32_t count, vg_lite_format_t format)
{
    uint32_t size = 0;
    int32_t dCount = 0;
    uint32_t i = 0;
    int32_t data_size = 0;

    data_size = get_data_size(format);

    for (i = 0; i < count; i++) {
        size++;     /* OP CODE. */

        dCount = get_data_count(cmd[i]);
        if (dCount > 0) {
            size = CDALIGN(size, data_size);
            size += dCount * data_size;
        }
    }

    return size;
}

vg_lite_error_t vg_lite_append_path(vg_lite_path_t *path,
                                    vg_lite_uint8_t *cmd,
                                    vg_lite_pointer data,
                                    vg_lite_uint32_t seg_count)
{
    vg_lite_error_t error = VG_LITE_SUCCESS;
    uint32_t i;
    int32_t j;
    int32_t offset = 0;
    int32_t dataCount = 0;
    float *dataf = (float*) data;
    float *pathf = NULL;
    int32_t *data_s32 = (int32_t*) data;
    int32_t *path_s32 = NULL;
    int16_t *data_s16 = (int16_t*) data;
    int16_t *path_s16 = NULL;
    int8_t *data_s8 = (int8_t*) data;
    int8_t *path_s8 = NULL;
    uint8_t *pathc = NULL;
    int32_t data_size;
    uint8_t arc_path = 0;
    float px = 0.0f, py = 0.0f, cx = 0.0f, cy = 0.0f;
    int rel = 0;

    if (cmd == NULL || data == NULL || path == NULL)
        return VG_LITE_INVALID_ARGUMENT;

    for(i = 0; i < seg_count; i++) {
        if (cmd[i] > VLC_OP_LCWARC_REL)
            return VG_LITE_INVALID_ARGUMENT;
    }

    /* Support NULL path->path case for OpenVG */
    if (!path->path) {
        data_size = vg_lite_get_path_length(cmd, seg_count, path->format);
        path->path = (vg_lite_pointer)malloc(data_size);
    }

    data_size = get_data_size(path->format);
    path->path_changed= 1;
    pathf = (float *)path->path;
    path_s32 = (int32_t *)path->path;
    path_s16 = (int16_t *)path->path;
    path_s8 = (int8_t *)path->path;
    pathc = (uint8_t *)path->path;

    /* Loop to fill path data. */
    for (i = 0; i < seg_count; i++) {
        *(pathc + offset) = cmd[i];
        offset++;

        dataCount = get_data_count(cmd[i]);
        if (dataCount > 0) {
            offset = CDALIGN(offset, data_size);
            if ((cmd[i] > VLC_OP_CLOSE) &&
                ((cmd[i] & 0x01) == 1)) {
                rel = 1;
            }
            else {
                rel = 0;
            }
            if (cmd[i] < VLC_OP_SCCWARC) {
                for (j = 0; j < dataCount / 2; j++) {
                    switch (path->format) {
                    case VG_LITE_S8:
                        path_s8 = (int8_t *)(pathc + offset);
                        path_s8[j * 2] = *data_s8++;
                        path_s8[j * 2 + 1] = *data_s8++;

                        if (rel) {
                            cx = px + path_s8[j * 2];
                            cy = py + path_s8[j * 2 + 1];
                        }
                        else {
                            cx = path_s8[j * 2];
                            cy = path_s8[j * 2 + 1];
                        }
                        break;
                    case VG_LITE_S16:
                        path_s16 = (int16_t *)(pathc + offset);
                        path_s16[j * 2] = *data_s16++;
                        path_s16[j * 2 + 1] = *data_s16++;

                        if (rel) {
                            cx = px + path_s16[j * 2];
                            cy = py + path_s16[j * 2 + 1];
                        }
                        else {
                            cx = path_s16[j * 2];
                            cy = path_s16[j * 2 + 1];
                        }
                        break;
                    case VG_LITE_S32:
                        path_s32 = (int32_t *)(pathc + offset);
                        path_s32[j * 2] = *data_s32++;
                        path_s32[j * 2 + 1] = *data_s32++;

                        if (rel) {
                            cx = px + path_s32[j * 2];
                            cy = py + path_s32[j * 2 + 1];
                        }
                        else {
                            cx = (float)path_s32[j * 2];
                            cy = (float)path_s32[j * 2 + 1];
                        }
                        break;
                    case VG_LITE_FP32:
                        pathf = (float *)(pathc + offset);
                        pathf[j * 2] = *dataf++;
                        pathf[j * 2 + 1] = *dataf++;

                        if (rel) {
                            cx = px + pathf[j * 2];
                            cy = py + pathf[j * 2 + 1];
                        }
                        else {
                            cx = pathf[j * 2];
                            cy = pathf[j * 2 + 1];
                        }
                        break;

                    default:
                        return VG_LITE_INVALID_ARGUMENT;
                    }

                    /* Update path bounds. */
                    path->bounding_box[0] = CDMIN(path->bounding_box[0], cx);
                    path->bounding_box[2] = CDMAX(path->bounding_box[2], cx);
                    path->bounding_box[1] = CDMIN(path->bounding_box[1], cy);
                    path->bounding_box[3] = CDMAX(path->bounding_box[3], cy);
                }
            }
#if gcFEATURE_VG_ARC_PATH
            else {
                    arc_path = 1;
                    pathf = (float *)(pathc + offset);
                    pathf[0] = *dataf++;
                    pathf[1] = *dataf++;
                    pathf[2] = *dataf++;
                    pathf[3] = *dataf++;
                    pathf[4] = *dataf++;

                    if (rel) {
                        cx = px + pathf[3];
                        cy = py + pathf[4];
                    }
                    else {
                        cx = pathf[3];
                        cy = pathf[4];
                    }

                    /* Update path bounds. */
                    compute_pathbounds(&path->bounding_box[0], &path->bounding_box[1], &path->bounding_box[2], &path->bounding_box[3],cx + 2 * pathf[0],cy + 2 * pathf[1]);
                    compute_pathbounds(&path->bounding_box[0], &path->bounding_box[1], &path->bounding_box[2], &path->bounding_box[3],px + 2 * pathf[1],py + 2 * pathf[1]);
                    compute_pathbounds(&path->bounding_box[0], &path->bounding_box[1], &path->bounding_box[2], &path->bounding_box[3],cx - 2 * pathf[0],cy - 2 * pathf[1]);
                    compute_pathbounds(&path->bounding_box[0], &path->bounding_box[1], &path->bounding_box[2], &path->bounding_box[3],px - 2 * pathf[1],py - 2 * pathf[1]);
            }
#endif
            px = cx;
            py = cy;

            offset += dataCount * data_size;
        }
    }

    path->path_length = offset;

#if gcFEATURE_VG_ARC_PATH
    if (arc_path) {
        error = vg_lite_init_arc_path(path,
                    VG_LITE_FP32,
                    path->quality,
                    path->path_length,
                    path->path,
                    path->bounding_box[0], path->bounding_box[1],
                    path->bounding_box[2], path->bounding_box[3]);
    }
#endif

    return error;
}
