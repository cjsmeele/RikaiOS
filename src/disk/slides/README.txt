This directory contains a slideshow presentation (its textual source, and a
tool to generate slide images from it).

The .rli files are run-length-encoded 1920x1080 32bpp images.
See the source of the "view" utility for more information on the file format.

In short: A RLI file is a flat array of packed little-endian RLE entries.
An RLE entry consists of a 16-bit count and a 32-bit RGBA value (color octets
are in the same order as is usually accepted by the framebuffer).
To decode it, simply repeat each color the given amount of times.

To encode PNG images as RLI, you must first produce a RGBA image, which can be
done as follows (on your host OS, not within RikaiOS):

    ./png2rgba.pl < YOUR_PNG_FILE | ./rlencode.pl > RLI_FILE

(script filenames were chosen with FAT's 8.3 in mind)

This uses ImageMagick under the hood, so make sure you have that installed
first :-)
