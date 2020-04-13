# mija-framegrabber

Sample program to grab the video streams on a Xiaomi MJSXJ02CM camera.

The code was obtained by using [Ghidra](https://ghidra-sre.org/) to analyze the miio-stream binary.

To build this binary you need the Mstar sdk and some libs from the camera to compile it (libshbf, libshbfev, libpthread-2.25, libc-2.25) and install the package "libev-dev". 

## How to use

1. Copy the library files to you lib folder in the SDK directory.
2. Clone this repository to the MI/sample folder in the SDK directory.
3. Build the binary using make.
4. Run the binary like this:

    ```
    mija_framegrab -m mainStreamPipeName1 -s substreamPipeName1
    ```
    
    It will create two named pipes with the names given as the arguments which then can be accessed by other tools.
    
    You can create up to 10 pipes per stream by default (MAX_PIPES_PER_STREAM) by giving more arguments:
    
    Use -m pipeName to create a pipe for the main stream and -s pipeName to create a pipe for the sub stream.
    
    "mija_framegrab -m mainstreamPipe1 -m mainstreamPipe2 -s substreamPipe1 -s substreamPipe2" will create 2 pipes per stream.

