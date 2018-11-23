/***************************************************************************
 *
 * Copyright (c) 2018 Mathias Thore
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ***************************************************************************/
#include <stdint.h>

#ifndef _JBF_H
#define _JBF_H

#define JBFSUCCESS     0
#define JBFEARGS      -1
#define JBFEMEM       -2
#define JBFECORRUPT   -3

typedef enum {
    JbfFiletypeE_raw  = 0x00,
    JbfFiletypeE_bmp  = 0x01,
    JbfFiletypeE_clp  = 0x03,
    JbfFiletypeE_cut  = 0x04,
    JbfFiletypeE_dib  = 0x06,
    JbfFiletypeE_emf  = 0x07,
    JbfFiletypeE_eps  = 0x08,
    JbfFiletypeE_fpx  = 0x09,
    JbfFiletypeE_gif  = 0x0a,
    JbfFiletypeE_iff  = 0x0b,
    JbfFiletypeE_img  = 0x0c,
    JbfFiletypeE_jpg  = 0x11,
    JbfFiletypeE_lbm  = 0x13,
    JbfFiletypeE_mac  = 0x14,
    JbfFiletypeE_msp  = 0x15,
    JbfFiletypeE_pbm  = 0x16,
    JbfFiletypeE_pcx  = 0x18,
    JbfFiletypeE_pgm  = 0x19,
    JbfFiletypeE_pic  = 0x1a,
    JbfFiletypeE_pct  = 0x1b,
    JbfFiletypeE_png  = 0x1c,
    JbfFiletypeE_ppm  = 0x1d,
    JbfFiletypeE_psd  = 0x1e,
    JbfFiletypeE_psp  = 0x1f,
    JbfFiletypeE_ras  = 0x20,
    JbfFiletypeE_rle  = 0x21,
    JbfFiletypeE_sct  = 0x22,
    JbfFiletypeE_tga  = 0x23,
    JbfFiletypeE_tif  = 0x24,
    JbfFiletypeE_wmf  = 0x25,
    JbfFiletypeE_wpg  = 0x26,
    JbfFiletypeE_rgb  = 0x27,
}   JbfFiletypeE;

typedef struct {
    uint32_t      filenamelength;
    char         *filename;
    uint64_t      filetime;
    uint32_t      filetype;
    uint32_t      width;
    uint32_t      height;
    uint32_t      bpp;
    uint32_t      bufsize;
    uint32_t      filesize;
    struct {
        uint32_t  size;
        uint8_t  *data;
    }             thumbnail;
} jbf_entry;

typedef struct {
    uint32_t      entrycount;
    char         *dirname;
    jbf_entry    *entries;
    void        *_handle;
} jbf_file;

int jbf_open(char *filename, jbf_file **jbf);
int jbf_close(jbf_file *jbf);

#endif // _JBF_H
