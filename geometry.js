/*
    mc4, a web voxel building game
    Copyright (C) 2019-2021 kholland4

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//A node is a cube with numbered faces 0, 1, 2, 3, 4, 5 each facing the OpenGL -x, +x, -y, +y, -z, and +z axes respectively.

var stdFaces = [
  {x: -1, y: 0, z: 0},
  {x: 1, y: 0, z: 0},
  {x: 0, y: -1, z: 0},
  {x: 0, y: 1, z: 0},
  {x: 0, y: 0, z: -1},
  {x: 0, y: 0, z: 1}
];

//axis: the face of the block that will be at the top; 0=+y, 1=-y, 2=+x, 3=+z, 4=-x, 5=-z
//face: the number of 90 degree clockwise (from the perspective above the top of the block) rotations around the axis
var stdRotAxis = [
  {x: 0, y: 1, z: 0},
  {x: 0, y: -1, z: 0},
  {x: 1, y: 0, z: 0},
  {x: 0, y: 0, z: 1},
  {x: -1, y: 0, z: 0},
  {x: 0, y: 0, z: -1}
];

//Top view of a node (above, +y):
//
// face=0 +y up      face=1          face=2           face=3
//      +x               -z               -x               +z
//    |-----|          |-----|          |-----|          |-----|
// -z |     | +z    -x |     | +x    +z |     | -z    +x |     | -x
//    |-----|          |-----|          |-----|          |-----|
//      -x               +z               +x               -z

//Default face order: -x +x -y +y -z +z

//mapping of rotation numbers (axis << 2 | face) to individual face shuffling and rotation
//rotations are a number of clockwise rotations ranging from 0 to 3
//shuffle[physicalFace] = faceToRetrieve
//shuffle and rotation both take the physical face index
var stdRotMapping = [
  //axis 0, facing +y
  {shuffle: [0, 1, 2, 3, 4, 5], rot: [0, 0, 0, 0, 0, 0]}, //axis 0 face 0
  {shuffle: [5, 4, 2, 3, 0, 1], rot: [0, 0, 1, 1, 0, 0]}, //axis 0 face 1
  {shuffle: [1, 0, 2, 3, 5, 4], rot: [0, 0, 2, 2, 0, 0]}, //axis 0 face 2
  {shuffle: [4, 5, 2, 3, 1, 0], rot: [0, 0, 3, 3, 0, 0]}, //axis 0 face 3
  
  //axis 1, facing -y
  {shuffle: [0, 1, 3, 2, 5, 4], rot: [2, 2, 0, 0, 2, 2]}, //axis 1 face 0
  {shuffle: [4, 5, 3, 2, 0, 1], rot: [2, 2, 3, 1, 2, 2]}, //axis 1 face 1
  {shuffle: [1, 0, 3, 2, 4, 5], rot: [2, 2, 2, 2, 2, 2]}, //axis 1 face 2
  {shuffle: [5, 4, 3, 2, 1, 0], rot: [2, 2, 1, 3, 2, 2]}, //axis 1 face 3
  
  //axis 2, facing +x
  {shuffle: [2, 3, 0, 1, 5, 4], rot: [0, 0, 0, 0, 3, 1]}, //axis 2 face 0
  {shuffle: [4, 5, 0, 1, 2, 3], rot: [1, 3, 3, 1, 0, 0]}, //axis 2 face 1
  {shuffle: [3, 2, 0, 1, 4, 5], rot: [0, 0, 2, 2, 1, 3]}, //axis 2 face 2
  {shuffle: [5, 4, 0, 1, 3, 2], rot: [3, 1, 1, 3, 0, 0]}, //axis 2 face 3
  
  //axis 3, facing +z
  {shuffle: [2, 3, 4, 5, 0, 1], rot: [1, 3, 0, 0, 3, 1]}, //axis 3 face 0
  {shuffle: [1, 0, 4, 5, 2, 3], rot: [1, 3, 3, 1, 1, 3]}, //axis 3 face 1
  {shuffle: [3, 2, 4, 5, 1, 0], rot: [3, 1, 2, 2, 1, 3]}, //axis 3 face 2
  {shuffle: [0, 1, 4, 5, 3, 2], rot: [3, 1, 1, 3, 3, 1]}, //axis 3 face 3
  
  //axis 4, facing -x
  {shuffle: [2, 3, 1, 0, 4, 5], rot: [2, 2, 0, 0, 3, 1]}, //axis 4 face 0
  {shuffle: [5, 4, 1, 0, 2, 3], rot: [1, 3, 3, 1, 2, 2]}, //axis 4 face 1
  {shuffle: [3, 2, 1, 0, 5, 4], rot: [2, 2, 2, 2, 1, 3]}, //axis 4 face 2
  {shuffle: [4, 5, 1, 0, 3, 2], rot: [3, 1, 1, 3, 2, 2]}, //axis 4 face 3
  
  //axis 5, facing -z
  {shuffle: [2, 3, 5, 4, 1, 0], rot: [3, 1, 0, 0, 3, 1]}, //axis 5 face 0
  {shuffle: [0, 1, 5, 4, 2, 3], rot: [1, 3, 3, 1, 3, 1]}, //axis 5 face 1
  {shuffle: [3, 2, 5, 4, 0, 1], rot: [1, 3, 2, 2, 1, 3]}, //axis 5 face 2
  {shuffle: [1, 0, 5, 4, 3, 2], rot: [3, 1, 1, 3, 1, 3]}  //axis 5 face 3
];

//Mapping of face rotations to vertex coordinate transformations.
// (original) => (90 degrees) => (180 degrees) => (270 degrees)
//
//About face 0 (-x):
//  (x, y, z) => (x, -z, y) => (x, -y, -z) => (x, z, -y)
//About face 1 (+x):
//  (x, y, z) => (x, z, -y) => (x, -y, -z) => (x, -z, y)
//About face 2 (-y):
//  (x, y, z) => (z, y, -x) => (-x, y, -z) => (-z, y, x)
//About face 3 (+y):
//  (x, y, z) => (-z, y, x) => (-x, y, -z) => (z, y, -x)
//About face 4 (-z):
//  (x, y, z) => (-y, x, z) => (-x, -y, z) => (y, -x, z)
//About face 5 (+z):
//  (x, y, z) => (y, -x, z) => (-x, -y, z) => (-y, x, z)

//this map tells how to rotate a given face 90 degrees clockwise by modifying the individual vertex coordinates
var vertexRotMap = [
  //physical face 0 (-x)
  //(x, y, z) => (x, -z, y)
  [0, 2, 1, 1, -1, 1],
  
  //physical face 1 (+x)
  //(x, y, z) => (x, z, -y)
  [0, 2, 1, 1, 1, -1],
  
  //physical face 2 (-y)
  //(x, y, z) => (z, y, -x)
  [2, 1, 0, 1, 1, -1],
  
  //physical face 3 (+y)
  //(x, y, z) => (-z, y, x)
  [2, 1, 0, -1, 1, 1],
  
  //physical face 4 (-z)
  //(x, y, z) => (-y, x, z)
  [1, 0, 2, -1, 1, 1],
  
  //physical face 5 (+z)
  //(x, y, z) => (y, -x, z)
  [1, 0, 2, 1, -1, 1]
];

//this map tells how to transform verticies of a custom mesh when shuffling faces
//indexed by faceShuffleMap[sourceFaceIndex][destFaceIndex]
var faceShuffleMap = [
  //source face 0 (-x)
  [
    //dest face 0 (-x)
    //(x, y, z) => (x, y, z)
    [0, 1, 2, 1, 1, 1],
    
    //dest face 1 (+x)
    //(x, y, z) => (-x, y, -z)
    [0, 1, 2, -1, 1, -1],
    
    //dest face 2 (-y)
    //(x, y, z) => (y, x, -z)
    [1, 0, 2, 1, 1, -1],
    
    //dest face 3 (+y)
    //(x, y, z) => (y, -x, z)
    [1, 0, 2, 1, -1, 1],
    
    //dest face 4 (-z)
    //(x, y, z) => (-z, y, x)
    [2, 1, 0, -1, 1, 1],
    
    //dest face 5 (+z)
    //(x, y, z) => (z, y, -x)
    [2, 1, 0, 1, 1, -1]
  ],
  
  //source face 1 (+x)
  [
    //dest face 0 (-x)
    //(x, y, z) => (-x, y, -z)
    [0, 1, 2, -1, 1, -1],
    
    //dest face 1 (+x)
    //(x, y, z) => (x, y, z)
    [0, 1, 2, 1, 1, 1],
    
    //dest face 2 (-y)
    //(x, y, z) => (y, -x, z)
    [1, 0, 2, 1, -1, 1],
    
    //dest face 3 (+y)
    //(x, y, z) => (y, x, -z)
    [1, 0, 2, 1, 1, -1],
    
    //dest face 4 (-z)
    //(x, y, z) => (z, y, -x)
    [2, 1, 0, 1, 1, -1],
    
    //dest face 5 (+z)
    //(x, y, z) => (-z, y, x)
    [2, 1, 0, -1, 1, 1]
  ],
  
  //source face 2 (-y)
  [
    //dest face 0 (-x)
    //(x, y, z) => (y, x, -z)
    [1, 0, 2, 1, 1, -1],
    
    //dest face 1 (+x)
    //(x, y, z) => (-y, x, z)
    [1, 0, 2, -1, 1, 1],
    
    //dest face 2 (-y)
    //(x, y, z) => (x, y, z)
    [0, 1, 2, 1, 1, 1],
    
    //dest face 3 (+y)
    //(x, y, z) => (x, -y, -z)
    [0, 1, 2, 1, -1, -1],
    
    //dest face 4 (-z)
    //(x, y, z) => (z, x, y)
    [2, 0, 1, 1, 1, 1],
    
    //dest face 5 (+z)
    //(x, y, z) => (z, -x, -y)
    [2, 0, 1, 1, -1, -1]
  ],
  
  //source face 3 (+y)
  [
    //dest face 0 (-x)
    //(x, y, z) => (-y, x, z)
    [1, 0, 2, -1, 1, 1],
    
    //dest face 1 (+x)
    //(x, y, z) => (y, x, -z)
    [1, 0, 2, 1, 1, -1],
    
    //dest face 2 (-y)
    //(x, y, z) => (x, -y, -z)
    [0, 1, 2, 1, -1, -1],
    
    //dest face 3 (+y)
    //(x, y, z) => (x, y, z)
    [0, 1, 2, 1, 1, 1],
    
    //dest face 4 (-z)
    //(x, y, z) => (-z, x, -y)
    [2, 0, 1, -1, 1, -1],
    
    //dest face 5 (+z)
    //(x, y, z) => (z, x, y)
    [2, 0, 1, 1, 1, 1]
  ],
  
  //source face 4 (-z)
  [
    //dest face 0 (-x)
    //(x, y, z) => (z, y, -x)
    [2, 1, 0, 1, 1, -1],
    
    //dest face 1 (+x)
    //(x, y, z) => (-z, y, x)
    [2, 1, 0, -1, 1, 1],
    
    //dest face 2 (-y)
    //(x, y, z) => (y, z, x)
    [1, 2, 0, 1, 1, 1],
    
    //dest face 3 (+y)
    //(x, y, z) => (y, -z, -x)
    [1, 2, 0, 1, -1, -1],
    
    //dest face 4 (-z)
    //(x, y, z) => (x, y, z)
    [0, 1, 2, 1, 1, 1],
    
    //dest face 5 (+z)
    //(x, y, z) => (-x, y, -z)
    [0, 1, 2, -1, 1, -1]
  ],
  
  //source face 5 (+z)
  [
    //dest face 0 (-x)
    //(x, y, z) => (-z, y, x)
    [2, 1, 0, -1, 1, 1],
    
    //dest face 1 (+x)
    //(x, y, z) => (z, y, -x)
    [2, 1, 0, 1, 1, -1],
    
    //dest face 2 (-y)
    //(x, y, z) => (-y, z, -x)
    [1, 2, 0, -1, 1, -1],
    
    //dest face 3 (+y)
    //(x, y, z) => (y, z, x)
    [1, 2, 0, 1, 1, 1],
    
    //dest face 4 (-z)
    //(x, y, z) => (-x, y, -z)
    [0, 1, 2, -1, 1, -1],
    
    //dest face 5 (+z)
    //(x, y, z) => (x, y, z)
    [0, 1, 2, 1, 1, 1]
  ]
];

//map between rotation numbers (see above) and Euler angles; used to rotate bounding boxes
//angles are in degrees
//FIXME needs rigorous testing
var boudingBoxRotMap = [
  //axis 0, facing +y
  {x: 0, y: 0, z: 0, order: "YZX"}, //axis 0 face 0
  {x: 0, y: 270, z: 0, order: "YZX"}, //axis 0 face 1
  {x: 0, y: 180, z: 0, order: "YZX"}, //axis 0 face 2
  {x: 0, y: 90, z: 0, order: "YZX"}, //axis 0 face 3
  
  //axis 1, facing -y
  {x: 180, y: 0, z: 0, order: "YZX"}, //axis 1 face 0
  {x: 180, y: 270, z: 0, order: "YZX"}, //axis 1 face 1
  {x: 180, y: 180, z: 0, order: "YZX"}, //axis 1 face 2
  {x: 180, y: 90, z: 0, order: "YZX"}, //axis 1 face 3
  
  //axis 2, facing +x
  {x: 0, y: 0, z: 270, order: "YZX"}, //axis 2 face 0
  {x: 0, y: 270, z: 270, order: "YZX"}, //axis 2 face 1
  {x: 0, y: 180, z: 270, order: "YZX"}, //axis 2 face 2
  {x: 0, y: 90, z: 270, order: "YZX"}, //axis 2 face 3
  
  //FIXME: axes 3, 4, and 5 are probably wrong.
  
  //axis 3, facing +z
  {x: 90, y: 90, z: 0, order: "YZX"}, //axis 3 face 0
  {x: 90, y: 180, z: 0, order: "YZX"}, //axis 3 face 1
  {x: 90, y: 270, z: 0, order: "YZX"}, //axis 3 face 2
  {x: 90, y: 0, z: 0, order: "YZX"}, //axis 3 face 3
  
  //axis 4, facing -x
  {x: 0, y: 0, z: 90, order: "YZX"}, //axis 4 face 0
  {x: 0, y: 90, z: 90, order: "YZX"}, //axis 4 face 1
  {x: 0, y: 180, z: 90, order: "YZX"}, //axis 4 face 2
  {x: 0, y: 270, z: 90, order: "YZX"}, //axis 4 face 3
  
  //axis 5, facing -z
  {x: 270, y: 270, z: 0, order: "YZX"}, //axis 5 face 0
  {x: 270, y: 0, z: 0, order: "YZX"}, //axis 5 face 1
  {x: 270, y: 90, z: 0, order: "YZX"}, //axis 5 face 2
  {x: 270, y: 180, z: 0, order: "YZX"}  //axis 5 face 3
];

/*
  1----36
  |   / |
  | /   |
  24----5
*/

var stdVerts = [
  [
    -0.5, 0.5, -0.5,
    -0.5, -0.5, -0.5,
    -0.5, 0.5, 0.5,
    
    -0.5, -0.5, -0.5,
    -0.5, -0.5, 0.5,
    -0.5, 0.5, 0.5
  ],
  [
    0.5, 0.5, 0.5,
    0.5, -0.5, 0.5,
    0.5, 0.5, -0.5,
    
    0.5, -0.5, 0.5,
    0.5, -0.5, -0.5,
    0.5, 0.5, -0.5
  ],
  [
    0.5, -0.5, 0.5,
    -0.5, -0.5, 0.5,
    0.5, -0.5, -0.5,
    
    -0.5, -0.5, 0.5,
    -0.5, -0.5, -0.5,
    0.5, -0.5, -0.5
  ],
  [
    0.5, 0.5, -0.5,
    -0.5, 0.5, -0.5,
    0.5, 0.5, 0.5,
    
    -0.5, 0.5, -0.5,
    -0.5, 0.5, 0.5,
    0.5, 0.5, 0.5
  ],
  [
    0.5, 0.5, -0.5,
    0.5, -0.5, -0.5,
    -0.5, 0.5, -0.5,
    
    0.5, -0.5, -0.5,
    -0.5, -0.5, -0.5,
    -0.5, 0.5, -0.5
  ],
  [
    -0.5, 0.5, 0.5,
    -0.5, -0.5, 0.5,
    0.5, 0.5, 0.5,
    
    -0.5, -0.5, 0.5,
    0.5, -0.5, 0.5,
    0.5, 0.5, 0.5
  ]
];

var stdUVs = [
  0.0, 1.0,
  0.0, 0.0,
  1.0, 1.0,
  
  0.0, 0.0,
  1.0, 0.0,
  1.0, 1.0
];

//var lightCurve = [0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240];
//var lightCurve = [0, 16 + 4, 32 + 4, 48 + 6, 64 + 8, 80 + 10, 96 + 12, 112 + 12, 128 + 14, 144 + 14, 160 + 14, 176 + 12, 192 + 10, 208 + 8, 224 + 6, 240];
var lightCurve = [0, 6, 15, 30, 50, 73, 98, 120, 139, 155, 168, 181, 196, 213, 234, 255];
