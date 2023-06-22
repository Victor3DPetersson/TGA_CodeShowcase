#include "Particle_Structs.hlsli"

[maxvertexcount(4)]
void main(
	point VertexToGeometry input[1], 
	inout TriangleStream< GeometryToPixel > output
)
{
	const float2 offsets[4] =
	{
		{-1.0f, 	1.0f},
		{1.0f, 		1.0f},
		{-1.0f, 	-1.0f},
		{1.0f, 		-1.0f}
	};
	const float2 uvs[4] = 
	{
		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f}
	};

	float cs = cos(input[0].myRotation);//Z rotation
	float sn = sin(input[0].myRotation);

	float3x3 rotationMatrixZ = 
	{cs, sn, 0,
	-sn, cs, 0,
	0, 0, 1};

	if(input[0].myBillboardState == 1)
	{
		float csY = cos(input[0].myRotationXY.y);
		float snY = sin(input[0].myRotationXY.y);
		float3x3 rotationMatrixY = 
		{csY, 0, -snY,
		0, 1, 0,
		snY, 0, csY};

		float csX = cos(input[0].myRotationXY.x);
		float snX = sin(input[0].myRotationXY.x);
		float3x3 rotationMatrixX = 
		{1, 0, 0,
		0, csX, snX,
		0, -snX, csX};

		//rotationMatrixY = mul(rotationMatrixY, rotationMatrixX);
		rotationMatrixZ = mul(rotationMatrixZ, mul(rotationMatrixY, rotationMatrixX));
	}

	float3 rotatedPosition;
	for (uint i = 0; i < 4; i++)
	{
		GeometryToPixel vertex;
		// rotatedPosition.x = ((offsets[i].x) * cs - (offsets[i].y) * sn);
		// rotatedPosition.y = ((offsets[i].x) * sn + (offsets[i].y) * cs);
		rotatedPosition = mul(rotationMatrixZ, float3(offsets[i]  ,1) * float3(input[0].mySize * 100, input[0].mySizeZ * 100));
		//vertex.myPosition = 0;
		vertex.myPosition = float4(rotatedPosition, 0);
		vertex.myPosition += input[0].myPosition;
		vertex.myPosition.w = 1;
		vertex.myPosition = mul(renderCamera.projection, vertex.myPosition);
		vertex.myColor = input[0].myColor;
		vertex.myEmissiveStrength = input[0].myEmissiveStrength;
		vertex.myUV = uvs[i];
		vertex.myDistToCam = input[0].myDistToCam;
		vertex.myUVPanningSpeed = input[0].myUVPanningSpeed;
		output.Append(vertex);
	}
}