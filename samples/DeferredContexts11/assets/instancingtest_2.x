xof 0303txt 0032
template Frame {
 <3d82ab46-62da-11cf-ab39-0020af71e433>
 [...]
}

template Matrix4x4 {
 <f6f23f45-7686-11cf-8f52-0040333594a3>
 array FLOAT matrix[16];
}

template FrameTransformMatrix {
 <f6f23f41-7686-11cf-8f52-0040333594a3>
 Matrix4x4 frameMatrix;
}

template Vector {
 <3d82ab5e-62da-11cf-ab39-0020af71e433>
 FLOAT x;
 FLOAT y;
 FLOAT z;
}

template MeshFace {
 <3d82ab5f-62da-11cf-ab39-0020af71e433>
 DWORD nFaceVertexIndices;
 array DWORD faceVertexIndices[nFaceVertexIndices];
}

template Mesh {
 <3d82ab44-62da-11cf-ab39-0020af71e433>
 DWORD nVertices;
 array Vector vertices[nVertices];
 DWORD nFaces;
 array MeshFace faces[nFaces];
 [...]
}

template MeshNormals {
 <f6f23f43-7686-11cf-8f52-0040333594a3>
 DWORD nNormals;
 array Vector normals[nNormals];
 DWORD nFaceNormals;
 array MeshFace faceNormals[nFaceNormals];
}

template Coords2d {
 <f6f23f44-7686-11cf-8f52-0040333594a3>
 FLOAT u;
 FLOAT v;
}

template MeshTextureCoords {
 <f6f23f40-7686-11cf-8f52-0040333594a3>
 DWORD nTextureCoords;
 array Coords2d textureCoords[nTextureCoords];
}

template ColorRGBA {
 <35ff44e0-6c7c-11cf-8f52-0040333594a3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
 FLOAT alpha;
}

template IndexedColor {
 <1630b820-7842-11cf-8f52-0040333594a3>
 DWORD index;
 ColorRGBA indexColor;
}

template MeshVertexColors {
 <1630b821-7842-11cf-8f52-0040333594a3>
 DWORD nVertexColors;
 array IndexedColor vertexColors[nVertexColors];
}

template ColorRGB {
 <d3e16e81-7835-11cf-8f52-0040333594a3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
}

template Material {
 <3d82ab4d-62da-11cf-ab39-0020af71e433>
 ColorRGBA faceColor;
 FLOAT power;
 ColorRGB specularColor;
 ColorRGB emissiveColor;
 [...]
}

template MeshMaterialList {
 <f6f23f42-7686-11cf-8f52-0040333594a3>
 DWORD nMaterials;
 DWORD nFaceIndexes;
 array DWORD faceIndexes[nFaceIndexes];
 [Material <3d82ab4d-62da-11cf-ab39-0020af71e433>]
}

template TextureFilename {
 <a42790e1-7810-11cf-8f52-0040333594a3>
 STRING filename;
}

template VertexDuplicationIndices {
 <b8d65549-d7c9-4995-89cf-53a9a8b031e3>
 DWORD nIndices;
 DWORD nOriginalVertices;
 array DWORD indices[nIndices];
}

template VertexElement {
 <f752461c-1e23-48f6-b9f8-8350850f336f>
 DWORD Type;
 DWORD Method;
 DWORD Usage;
 DWORD UsageIndex;
}

template DeclData {
 <bf22e553-292c-4781-9fea-62bd554bdd93>
 DWORD nElements;
 array VertexElement Elements[nElements];
 DWORD nDWords;
 array DWORD data[nDWords];
}


Frame DXCC_ROOT {
 

 FrameTransformMatrix {
  1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000;;
 }

 Frame pSphere1 {
  

  FrameTransformMatrix {
   1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000;;
  }

  Frame pSphereShape1 {
   

   FrameTransformMatrix {
    1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000,0.000000,0.000000,0.000000,0.000000,1.000000;;
   }

   Mesh pSphereShape1_Mesh {
    4;
    -0.500000;-0.500000;0.00000;,
    0.500000;-0.500000;0.00000;,
    0.500000;0.500000;0.00000;,
    -0.500000;0.500000;0.00000;;
    2;
    3;0,1,2;,
    3;0,2,3;;

    MeshNormals {
     4;
     0.000000;0.000000;1.000000;,
	 0.000000;0.000000;1.000000;,
	 0.000000;0.000000;1.000000;,
	 0.000000;0.000000;1.000000;;
     2;
     3;0,1,2;,
     3;0,2,3;;
    }

    MeshTextureCoords {
     4;
     0.000000;0.000000;,
     1.000000;0.000000;,
     1.000000;1.000000;,
     0.000000;1.000000;;
    }

    MeshVertexColors {
     4;
     0;0.000000;0.000000;0.000000;0.000000;;,
     1;0.000000;0.000000;0.000000;0.000000;;,
     2;0.000000;0.000000;0.000000;0.000000;;,
     3;0.000000;0.000000;0.000000;0.000000;;;
    }

    MeshMaterialList {
     1;
     2;
     0,
     0;

     Material {
      0.800000;0.800000;0.800000;1.000000;;
      0.000000;
      0.250000;0.250000;0.250000;;
      0.000000;0.000000;0.000000;;

     }

    }

    VertexDuplicationIndices {
     4;
     4;
     0,
     1,
     2,
     3;
    }

    DeclData {
     2;
     2;0;6;0;,
     2;0;7;0;;
     174;
     3207922931,
     3203598055,
     3204852828,
     1060439282,
     3203598054,
     3204852829,
     3207475251,
     1050696895,
     3207174800,
     3206352103,
     3208440476,
     1049622523,
     3007022743,
     858379510,
     3212836864,
     1061011436,
     3207321748,
     2994898280,
     3212836863,
     430136232,
     859611076,
     638253836,
     3212836863,
     842833860,
     1050102662,
     3196650762,
     1063962291,
     1059980469,
     3206388058,
     3201048183,
     1064974257,
     3188803042,
     1042311291,
     3193465338,
     3207067362,
     1060730546,
     1065349540,
     1017871502,
     3007156709,
     1017871480,
     3212833187,
     841796066,
     1048609197,
     3198965631,
     1063802664,
     1057677973,
     3208266422,
     3201759852,
     3212241016,
     3191181440,
     1044937488,
     1049050408,
     3206921773,
     1060569787,
     3194939469,
     1050686322,
     1064022574,
     1057809429,
     3208442719,
     1053283691,
     3211423002,
     3197215068,
     3197193684,
     1021573744,
     3208634430,
     1059672476,
     3212361465,
     1042446711,
     1043556062,
     3195144705,
     3207002624,
     3208142712,
     853504000,
     0,
     3212836863,
     1065353215,
     0,
     873127243,
     3198534352,
     3206715194,
     3207825335,
     1064449628,
     3193866936,
     3195386415,
     1064767022,
     1043602761,
     3192316202,
     1048979612,
     3206928252,
     1060576942,
     1065351753,
     0,
     3159901376,
     1012417600,
     0,
     1065351753,
     1054865689,
     3200964377,
     3209603416,
     1058614773,
     3205151220,
     1058477582,
     3205598398,
     1057840723,
     1058805962,
     3209861355,
     3200477267,
     3201811475,
     3204157124,
     0,
     1063188570,
     3210672219,
     2147483648,
     3204157122,
     1061575718,
     3201909779,
     1055909585,
     3206665414,
     3204786014,
     1058211517,
     3203193481,
     1057334448,
     3208081629,
     3209326678,
     3206266846,
     1032439732,
     867687423,
     0,
     3212836863,
     3212836863,
     0,
     3006528208,
     3185869904,
     1065248448,
     2983101616,
     3212732096,
     3185869904,
     859878714,
     1064878267,
     3195137294,
     3006014647,
     3195137296,
     3212361915,
     850707694,
     1065351751,
     0,
     3159901312,
     3159901600,
     0,
     3212835404,
     1058170760,
     3208927303,
     1049952432,
     1043473191,
     3194939134,
     3212110689,
     3205654407,
     1061443656,
     1049952424,
     1043473192,
     3194939127,
     1064627041,
     3212835400,
     0,
     3159901536,
     3159901664,
     0,
     1065351754,
     3210479492,
     1015763824,
     3204630625,
     3198314114,
     3209506428,
     1057081311;
    }
   }
  }
 }
}