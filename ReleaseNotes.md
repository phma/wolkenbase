# Version 0.1.2
- You can now enter the cloud thickness, and Wolkenbase takes thickness into account when classifying points as ground.
- You can save a point cloud as is. This is useful when you have a point cloud already classified and want to separate different classes of points into different files.
- Reading multiple files at once is faster.
- Opening a point cloud, classifying or saving it, clearing it, and opening another point cloud now works correctly.
- It's confirmed with test data that Wolkenbase loses a few points.

# Version 0.1.1
- There's a new program LASify, which reads a point cloud in XYZ or PLY format and converts it to LAS. This is needed, because the octree has to start with a bounding cube, and only LAS, of these formats, has bounds in the header.

# Version 0.1.0
- Wolkenbase does reasonably well with forests, as long as some laser shots make it to the floor, but does not yet understand buildings, and often gets stuck on walls.
- When reading files, it spends a lot of time in unlockCube waiting for cubeMutex.
- It seems to lose a few points, less than one in a million.
