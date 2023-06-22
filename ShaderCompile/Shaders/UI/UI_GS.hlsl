#include "UIStructs.hlsli"

struct GSOutput
{
	float4 pos : SV_POSITION;
};

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
		{inputVertex.myUVOffsetTL.xy + FLT_EPSILON},
		{inputVertex.myUVOffsetBR.x - FLT_EPSILON, inputVertex.myUVOffsetTL.y + FLT_EPSILON},
		{inputVertex.myUVOffsetTL.x + FLT_EPSILON, inputVertex.myUVOffsetBR.y - FLT_EPSILON},
		{inputVertex.myUVOffsetBR.xy - FLT_EPSILON}
	};
	float2 screenPosition = inputVertex.myPosition.xy;
	float2 pivot = ((inputVertex.myPivotOffset * 2) - 1);
	float2 normalizedImageSize;
	normalizedImageSize.x = (inputVertex.mySize.x / 1920) * 0.5f;
	normalizedImageSize.y = (inputVertex.mySize.y / 1080) * 0.5f;

	float cs = cos(inputVertex.myRotation);
	float sn = sin(inputVertex.myRotation);

	for (unsigned int i = 0; i < 4; i++)
	{
		GeometryToPixel vertex;
	
		float2 rotatedPosition;
		rotatedPosition.x = ((offsets[i].x + pivot.x) * cs - (offsets[i].y + pivot.y) * sn);
		rotatedPosition.y = ((offsets[i].x + pivot.x) * sn + (offsets[i].y + pivot.y) * cs);

		rotatedPosition.x = (rotatedPosition.x * normalizedImageSize.x) + screenPosition.x;
		rotatedPosition.y = (rotatedPosition.y * normalizedImageSize.y)+ screenPosition.y;
		vertex.myPosition = float4(rotatedPosition, 0, 1);
		vertex.myUV = uvs[i];
		vertex.myColor = inputVertex.myColor;
		vertex.myData = inputVertex.myData;
		
		output.Append(vertex);
	}
}