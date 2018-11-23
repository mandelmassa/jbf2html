# jbf2html

## Introduction

jbf2html is a program which converts Jasc Paint Shop Pro 7 browse files
(.jbf) to html with embedded thumbnails.

The style sheet in the html file emulates the look of the file browser in
Paint Shop Pro 7 as it appears in flat style in Windows 10.

The thumbnails link to local file names, allowing browsing an image
collection locally in a web browser.

## Using jbf2html

jbf2html operates on jbf files, and outputs html data. By default, the
current working directory is searched for a file named pspbrwse.jbf, and
html data is output to a file called index.html. The following command line
options can be used to tune this behavior:

Option  | Argument  | Description
:---    | :---      | :---
-o      | filename  | Direct output to the named file rather than index.html.
-z      |           | Include images with no thumbnail data in the output.
-h      |           | Shows the built-in help and exits.
input   |           | jbf file or directory containing pspbrwse.jbf file to operate on.

 * jbf2html in itself does not access the images listed in the jbf file -
thumbnails are extracted directly from the jbf file itself.
 * There is no way to influence image sorting in the html file after it has
been created.

## Building jbf2html

There are no special external dependencies. The included Makefile compiles
all source files and links the binary. Invoke `make` to build jbf2html.

## The JBF File Format

I could not find any previous documentation on the jbf file format so I had
a go at decoding it myself. I haven't been able to decode every single byte
of information in the file at this point, but certainly enough for the
requirements of this project. The method employed was to look at hexdumps
of jbf files for different collections of image files, and deduce the file
structure that way.

Fortunately, the structure of the jbf file is very simple. There are only 2
structural components:

1. A file header
2. An array of thumbnail objects.

Most numeric fields are 32-bit, little endian unsigned integers. Data
alignment is not guaranteed by the apparent jbf specification, however. The
jbf parser implementation in this project assumes that unaligned accesses
to 32-bit and 64-bit integers are safe.

There is no padding between thumbnail objects - one follows immediately
after the other in the file.

### File Header

The file header, 1 KB in total, contains a magic file marker, a counter for
the number of thumbnail objects included, and a path where the file was
originally created. The rest of this header looks to be largely
uninitialized junk.

```c
struct filehdr {
    char          magic[16];    // NULL terminated string "JASC BROWS FILE"
    uint8_t       data1[3];     // Ignored for now
    uint32_t      count;        // Number of thumbnail objects in the file
    uint8_t       data2[0x3e9]; // The rest is ignored for now
} __attribute__ ((packed));
```

### Thumbnail Object

Each thumbnail object contains a variable length filename stored as ASCII
text, a fixed header describing the image and thumbnail data, and a
thumbnail. The actual thumbnail image data is a complete, embedded JPEG
file. Thumbnail size seems to be limited to 150 by 150 pixels regardless of
the thumbnail size setting in PSP7.

Filename data comes first: a non-NULL terminated ASCII string preceded by
its length.

```c
struct filenamehdr {
    uint32_t      namelength;   // number of bytes in the filename
    uint8_t       filename[];   // filename data, not NULL terminated,
                                // namelength bytes long.
} __attribute__ ((packed));
```

After this follows the fixed header and thumbnail data.

```c
struct entryhdr {
    uint64_t      filetime;     // Win32 FILETIME, last time the image was modified
    uint32_t      filetype;     // Image type, represented by the JbfFileTypeE enum
    uint32_t      width;        // Image width in pixels
    uint32_t      height;       // Image height in pixels
    uint32_t      bpp;          // Image bits per pixel. 1, 4, 8, or 24.
    uint32_t      bufsize;      // Seems to contain width*height*bpp, buffer size
                                // required to hold the uncompressed image data?
    uint32_t      filesize;     // Image file size
    uint32_t      data1[2];     // data1[0] = 2, unknown interpretation
                                // data1[1] = 1, #of planes?
    uint32_t      thumbmagic;   // 0xffffffff
    uint32_t      thumbsize;    // number of bytes of thumbnail data
    uint8_t       jpghdr[];     // actual thumbnail data, thumbsize bytes long
} __attribute__ ((packed));
```

Raw files are supported by PSP7, but don't get a thumbnail. Some valid
information may be included in the jbf file however; filename, filetime,
and filetype. Other fields seem to be set to 0. The header ends after
data1[0].  Currently, this is detected by checking for thumbmagic !=
0xffffffff, but this is probably not correct. data1[0] may be a more
correct indicator, but what does the value 2 indicate?

## Acknowledgements

jbf2html uses Jouni Malinen's base64 encoder which is distributed under a
BSD license. These files, base64.c and base64.h, have been slightly
modified to suit the jbf2html project.

<http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c>
