# gimp-vtf

A GIMP 3.0 plugin that reads and writes Valve Texture Format (VTF) files.

Internally, it uses [sourcepp](https://github.com/craftablescience/sourcepp/). Many thanks to the author of sourcepp for making this plugin possible.

## Installation

Go to **[Releases](./releases/)** on the right of the page.

Download the zip file according to your operating system.

### Windows

\-\- todo \-\-

### Linux

1. Go to your GIMP plug-ins folder, usually found in `~/.config/GIMP/3.0/plug-ins/`
2. Create a `file-vtf` directory in there, and put the `file-vtf` executable inside that new directory.
3. The final path of the executable should be something like `~/.config/GIMP/3.0/plug-ins/file-vtf/file-vtf`
4. Launch GIMP. Now you can import and export VTF files.

### MacOS

\-\- todo \-\-

## Build

This project requires GIMP 3.0 development headers to be present.

Install them however you normally would for whatever operating system or distro you use.

Then, once that's complete:

1. Initialize cmake: `cmake -B build`
2. Compile the executable: `cmake --build build`

It will create the `file-vtf` executable in the `build` directory.
