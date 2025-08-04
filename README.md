# gimp-vtf

A GIMP 3.0 plugin that reads and writes Valve Texture Format (VTF) files.

Internally, it uses [sourcepp](https://github.com/craftablescience/sourcepp/). Many thanks to the author of sourcepp for making this plugin possible.

The project is currently in a **beta** state; the plugin works, but some features are missing. There is no working Windows build yet (see #1). 

## Installation

Go to **[Releases](https://github.com/chev2/gimp-vtf/releases)** on the right of the page.

Download the zip file according to your operating system.

### Windows

Currently not supported :\/


### Linux

1. Go to your GIMP plug-ins folder, usually found in `~/.config/GIMP/3.0/plug-ins/`
2. Create a `file-vtf` directory in there, and put the `file-vtf` executable inside that new directory.
3. The final path of the executable should be something like `~/.config/GIMP/3.0/plug-ins/file-vtf/file-vtf`
4. Launch GIMP. Now you can import and export VTF files.

### MacOS

Currently not supported :\/


## Build

This project requires GIMP 3.0 development headers to be present. It's tested and confirmed working on GIMP 3.0.2.

Install those development headers however you normally would for whatever operating system or distro you use. You can take a look at the GitHub actions workflow file for how it does this for each operating system.

Then, once that's complete:

1. Clone this repo: `git clone https://github.com/chev2/gimp-vtf gimp-vtf`
2. Change directory into the repo: `cd gimp-vtf`
3. Initialize cmake: `cmake -B build`
4. Compile the executable: `cmake --build build`

It will create the `file-vtf` executable in the `build` directory.
