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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "jbf.h"
#include "base64.h"

static void printcss(FILE *out);
static void printentry(FILE *out, jbf_entry *entry);
static const char *JbfFiletypeES(jbf_entry *entry);
static const char *BppS(jbf_entry *entry);
static char *filetimeS(jbf_entry *entry);
static char *filesizeS(jbf_entry *entry);

static void printhelp(void)
{
    printf("jbf2html [-h|-z|-o <file>] input\n"
           "\n"
           "Transforms Paint Shop Pro 7 jbf files to complete html pages\n"
           "with embedded thumbnails.\n"
           "\n"
           " -h          show this help\n"
           " -z          include entries with 0-byte thumbnails\n"
           "             (skipped by default)\n"
           " -o <file>   direct output to <file>\n"
           "             If not supplied, index.html is used.\n"
           " input       jbf file or directory where a jbf file is stored.\n"
           "             If none is given, current working directory is\n"
           "             searched for a file named pspbrwse.jbf\n");
}

int main(int argc, char *argv[])
{
    jbf_file *jbf;
    int ret = !JBFSUCCESS;
    int i;
    char *infile = NULL;
    char *outfile = NULL;
    int opt;
    FILE *out;
    uint32_t skip_zero_thumbs = 1;

    /***********************************************************************
     * parse command line
     */
    while ((opt = getopt(argc, argv, "hzo:")) != -1) {
        switch (opt) {
        case 'o':
            outfile = optarg;
            break;

        case 'h':
            printhelp();
            return 0;

        case 'z':
            skip_zero_thumbs = 0;
            break;
        }
    }

    // input?
    if (optind < argc) {
        infile = argv[optind];
    }

    /***********************************************************************
     * open jbf file
     *
     * if we get an argument, we assume it's
     * 1. a complete file name or
     * 2. a path
     * otherwise, look for pspbrwse.jbf in cwd
     */

    if (infile != NULL) {
        // 1. a complete file name
        ret = jbf_open(infile, &jbf);

        // 2. a path
        if (ret != JBFSUCCESS) {
            char *infile2 = calloc(1, strlen(infile) + 25);
            strcat(infile2, infile);
            strcat(infile2, "/pspbrwse.jbf");
            ret = jbf_open(infile2, &jbf);
            free(infile2);
        }
    }

    // 3. look in cwd
    if (ret != JBFSUCCESS) {
        ret = jbf_open("pspbrwse.jbf", &jbf);
    }

    if (ret != JBFSUCCESS) {
        fprintf(stderr, "error: jbf file not opened\n");
        return -1;
    }

    /***********************************************************************
     * open output file
     */

    if (outfile == NULL) {
        outfile = "index.html";
        out = fopen(outfile, "r");
        if (out != NULL) {
            fprintf(stderr, "error: index.html exists\n");
            return -1;
        }
    }
    out = fopen(outfile, "w");
    if (out == NULL) {
        fprintf(stderr, "error: can not open %s\n", outfile);
        return -1;
    }

    /***********************************************************************
     * output html document
     */

    fprintf(out,
            "<!DOCTYPE html>\n"
            "<!-- Created by jbf2html -->\n"
            "<html>\n"
            "<head>\n"
            "<title>Browse</title>\n"
            "<style>\n");
    printcss(out);
    fprintf(out,
            "</style>\n"
            "</head>\n"
            "<body>\n"
            "\n");

    // print entries
    for (i = 0; i < jbf->entrycount; i++) {
        if (jbf->entries[i].thumbnail.size == 0 && skip_zero_thumbs) {
            continue;
        }
        printentry(out, &jbf->entries[i]);
    }

    // close html
    fprintf(out,
            "</body>\n"
            "</html>\n");

    jbf_close(jbf);

    return 0;
}

static void printentry(FILE *out, jbf_entry *entry)
{
    unsigned char *imgdata;
    char *filetime;
    char *filesize;

    imgdata  = base64_encode(entry->thumbnail.data, entry->thumbnail.size, NULL);
    filetime = filetimeS(entry);
    filesize = filesizeS(entry);

    fprintf(out,
            "<div class=\"object\">\n"
            "<a href=\"%s\"\n"
            "title=\"%s\n"
            "%u x %u x %s, %s\n"
            "%s\n"
            "%s\">\n"
            "<span class=\"container\">\n"
            "<img class=\"thumbnail\" src=\"data:image/jpeg;charset=utf-8;base64,\n%s\" />\n"
            "</span>\n"
            "<span class=\"filename\">%s</span>\n"
            "</a>\n"
            "</div>\n"
            "\n",
            entry->filename,
            entry->filename, entry->width, entry->height, BppS(entry), filesize,
            JbfFiletypeES(entry),
            filetime,
            imgdata,
            entry->filename);

    free(imgdata);
    free(filetime);
    free(filesize);
}

static void printcss(FILE *out)
{
    fprintf(out,
            ".object {\n"
            "  border: 2px solid transparent;\n"
            "  margin: 3px;\n"
            "  margin-top: 5px;\n"
            "  padding: 2px;\n"
            "  float: left;\n"
            "}\n"
            ".object:hover {\n"
            "  border: 2px solid #A0A0A0;\n"
            "}\n"
            ".object:focus-within {\n"
            "  outline: 1px dotted #212121;\n"
            "}\n");
    fprintf(out,
            "a {\n"
            "  color: black;\n"
            "  text-decoration: none;\n"
            "}\n"
            "a:focus {\n"
            "  outline: none;\n"
            "}\n");
    fprintf(out,
            ".container {\n"
            "  display: block;\n"
            "  width: 150px;\n"
            "  height: 150px;\n"
            "}\n");
    fprintf(out,
            ".thumbnail {\n"
            "  display: block;\n"
            "  margin-left: auto;\n"
            "  margin-right: auto;\n"
            "  position: relative;\n"
            "  top: 50%%;\n"
            "  transform: translateY(-50%%);\n"
            "}\n");
    fprintf(out,
            ".filename {\n"
            "  display: block;\n"
            "  font-size: 0.7em;\n"
            "  text-align: center;\n"
            "  max-width: 150px;\n"
            "  overflow: hidden;\n"
            "  white-space: nowrap;\n"
            "  text-overflow: ellipsis;\n"
            "}\n");
}

/* Convert bpp to string as displayed in PSP7.
 *
 * Returned pointer must not be freed.
 */
static const char *BppS(jbf_entry *entry)
{
    switch (entry->bpp) {
    case 1:  return "2";
    case 4:  return "16";
    case 8:  return "256";
    case 24: return "16 Million";
    default: return "X bpp";
    }
}

/* Convert file type enum to file type string as displayed in PSP7.
 *
 * Returned pointer must not be freed.
 */
static const char *JbfFiletypeES(jbf_entry *entry)
{
    switch (entry->filetype) {
    case JbfFiletypeE_raw: return "Raw File Format";
    case JbfFiletypeE_bmp: return "Windows or OS/2 Bitmap";
    case JbfFiletypeE_clp: return "Windows Clipboard";
    case JbfFiletypeE_cut: return "Dr. Halo";
    case JbfFiletypeE_dib: return "OS/2 or Windows DIB";
    case JbfFiletypeE_emf: return "Windows Enhanced Meta File";
    case JbfFiletypeE_eps: return "Encapsulated PostScript";
    case JbfFiletypeE_fpx: return "FlashPix";
    case JbfFiletypeE_gif: return "CompuServe GIF";
    case JbfFiletypeE_iff: return "Amiga Interchange Format";
    case JbfFiletypeE_img: return "GEM Paint";
    case JbfFiletypeE_jpg: return "JPEG - JFIF Compliant";
    case JbfFiletypeE_lbm: return "Deluxe Paint";
    case JbfFiletypeE_mac: return "MacPaint";
    case JbfFiletypeE_msp: return "Microsoft Paint";
    case JbfFiletypeE_pbm: return "Portable Bitmap";
    case JbfFiletypeE_pcx: return "Zsoft Paintbrush";
    case JbfFiletypeE_pgm: return "Portable Greymap";
    case JbfFiletypeE_pic: return "PC Paint";
    case JbfFiletypeE_pct: return "Macintosh PICT";
    case JbfFiletypeE_png: return "Portable Network Graphics";
    case JbfFiletypeE_ppm: return "Portable Pixelmap";
    case JbfFiletypeE_psd: return "Photoshop 2.5";
    case JbfFiletypeE_psp: return "Paint Shop Pro";
    case JbfFiletypeE_ras: return "SUN Raster Images";
    case JbfFiletypeE_rle: return "Compressed Bitmap";
    case JbfFiletypeE_sct: return "SciTex Continuous Tone";
    case JbfFiletypeE_tga: return "Truevision Targa";
    case JbfFiletypeE_tif: return "Tagged Image File Format";
    case JbfFiletypeE_wmf: return "Windows Meta File";
    case JbfFiletypeE_wpg: return "Word Perfect";
    case JbfFiletypeE_rgb: return "SGI Image File";
    default:               return "Unknown type";
    }
}

/* Convert Win32 FILETIME to readable timestamp using common algorithm
 * found online. Not clean but works OK.
 *
 * Returned pointer must be freed by caller.
 */
static char *filetimeS(jbf_entry *entry)
{
    uint64_t filetime;
    struct tm tf_tm;
    char buf[256];

#define TICKS_PER_SECOND 10000000
#define EPOCH_DIFFERENCE 11644473600LL

    filetime  = entry->filetime;
    filetime /= TICKS_PER_SECOND;
    filetime -= EPOCH_DIFFERENCE;

    // this cast isn't guaranteed
    localtime_r((time_t *) &filetime, &tf_tm);

    snprintf(buf, sizeof(buf),
             "%04d-%02d-%02d %02d:%02d:%02d",
             1900 + tf_tm.tm_year, 1 + tf_tm.tm_mon, tf_tm.tm_mday,
             tf_tm.tm_hour, tf_tm.tm_min, tf_tm.tm_sec);
    return strdup(buf);
}

/* Convert byte count to file size as displayed in PSP7. GB case is
 * untested/unknown.
 *
 * Returned pointer must be freed by caller.
 */
static char *filesizeS(jbf_entry *entry)
{
    char buf[256];

    if (entry->filesize < 1024) {
        snprintf(buf, sizeof(buf),
                 "%u bytes",
                 entry->filesize);
    }
    else if (entry->filesize < (1024 * 1024)) {
        snprintf(buf, sizeof(buf),
                 "%.1f KB",
                 entry->filesize / 1024.0);
    }
    else if (entry->filesize < (1024 * 1024 * 1024)) {
        snprintf(buf, sizeof(buf),
                 "%.2f MB",
                 entry->filesize / (1024.0 * 1024.0));
    }
    else  {
        snprintf(buf, sizeof(buf),
                 "%.2f GB",
                 entry->filesize / (1024.0 * 1024.0 * 1024.0));
    }
    return strdup(buf);
}
