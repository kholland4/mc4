/*
    mc4, a web voxel building game
    Copyright (C) 2019 kholland4

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

/*var stdFaces = [
  new THREE.Vector3(-1, 0, 0),
  new THREE.Vector3(1, 0, 0),
  new THREE.Vector3(0, -1, 0),
  new THREE.Vector3(0, 1, 0),
  new THREE.Vector3(0, 0, -1),
  new THREE.Vector3(0, 0, 1)
];*/
var stdFaces = [
  {x: -1, y: 0, z: 0},
  {x: 1, y: 0, z: 0},
  {x: 0, y: -1, z: 0},
  {x: 0, y: 1, z: 0},
  {x: 0, y: 0, z: -1},
  {x: 0, y: 0, z: 1}
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
