
We are building a voxel editor. Users will use this program to build
3D voxel objects, then select vertexes, and then from those vertexes,
generate a smooth 3D object.

This will eventually be done in VR, but I would like the majority of
the code to be built in a way that can be run on the desktop.

For VR, this will run on the Meta quest 3, and use the hands library.

We want close looped testing on the desktop, so I would like you to
use straight OpenGL and have a command line tool to render at any
point in time.

We want this to be reasonably high performant. It does not need to
have fancy rendering.

We should break down the project into logical parts and validate with
unit tests.

There will be three applications:
1) A command line application with a simple rendering window.
2) A desktop Qt application
3) A VR application

All three should use a shared library for doing the bulk of the
work. The shared library can either render to an OpenGL surface or to
a local image.



