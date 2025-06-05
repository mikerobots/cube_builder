I would like to enhance the way this voxel editor works. The first
thing I would like to do is to add a work space to start placing
voxels. This will be a grid that is 32cm squares. This should be the
ground plane. You will be able to point at the grid and click. This
will add voxels to that location. The voxels can be offset from the
32cm markings.

Voxels should be able to be placed at an offset of 1cm.

There will be a outline of where the voxel will be placed on the
ground plane when hovering over the ground plane.

The ground plane should be partially transparent.

When voxels are being placed on other voxels (side, top, bottom, etc),
the other voxel will highlight the side that the mouse is pointing at.

If the voxel is smaller than the voxel it is being placed on, it will
create a shadow of where it will be placed, much like the ground plane. 

If the voxel is the same size as the voxel being placed on, it will default being aligned.

Bigger voxels can be placed on smaller voxels. If the mouse points at
a smaller voxel, the editor will show a placement highlight at the
plane of the smaller voxel. If the user moves away from that voxel,
the editor will stay that the same plane until the entire shadow is
off of the plane of the smaller voxel. Any time the mouse points at a
plane that is either perpendicular to the current plane of the shadow
or taller than the current plane of the shadow, it will shift to the
new plane.
