#include "World_UIStructs.hlsli"

[maxvertexcount(4)]
void main(
	point VertexToGeometry input[1], 
	inout TriangleStream<GeometryToPixel> output
)
{
	const float2 offsets[4] =
	{
		{-1.0f, 	1.0f},
		{1.0f, 		1.0f},
		{-1.0f, 	-1.0f},
		{1.0f, 		-1.0f}
	};
	VertexToGeometry inputVertex = input[0];
	const float2 uvs[4] = 
	{
		{inputVertex.myUVOffsetTL.xy},
		{inputVertex.myUVOffsetBR.x, inputVertex.myUVOffsetTL.y},
		{inputVertex.myUVOffsetTL.x, inputVertex.myUVOffsetBR.y},
		{inputVertex.myUVOffsetBR.xy}
	};
	float ratio = frameRenderResolutionX / frameRenderResolutionY;

	float4 cameraSpacePos = float4(inputVertex.myPosition, 1);
	float2 pivot = ((inputVertex.myPivotOffset * 2) - 1);

	float cs = cos(inputVertex.myRotation);
	float sn = sin(inputVertex.myRotation);

	for (unsigned int i = 0; i < 4; i++)
	{
		GeometryToPixel vertex;
	
		float4 rotatedPosition = 1;
		rotatedPosition.x = ((offsets[i].x + pivot.x) * cs - (offsets[i].y + pivot.y) * sn);
		rotatedPosition.y = ((offsets[i].x + pivot.x) * sn + (offsets[i].y + pivot.y) * cs);

		rotatedPosition.x = (rotatedPosition.x * inputVertex.mySize.x);
		rotatedPosition.y = (rotatedPosition.y * inputVertex.mySize.y);
		rotatedPosition += cameraSpacePos;

		float4 vertexPerspectivePos = mul(renderCamera.projection, rotatedPosition);
		vertex.myPosition = vertexPerspectivePos;
		vertex.myUV = uvs[i];
		vertex.myColor = inputVertex.myColor;
		vertex.myTrash = 1;
		
		output.Append(vertex);
	}
}