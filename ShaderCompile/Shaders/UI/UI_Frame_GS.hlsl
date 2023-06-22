#include "UIStructs.hlsli"
#include "UIFrame.hlsli"

struct GSOutput
{
	float4 pos : SV_POSITION;
};

[maxvertexcount(4)]
void main(
	point VertexToGeometry input[1],
	inout TriangleStream<FrameGeometryToPixel> output
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
		/*{inputVertex.myUVOffsetTL.xy},
		{inputVertex.myUVOffsetBR.x, inputVertex.myUVOffsetTL.y},
		{inputVertex.myUVOffsetTL.x, inputVertex.myUVOffsetBR.y},
		{inputVertex.myUVOffsetBR.xy}*/
		float2(0, 0),
		float2(1, 0),
		float2(0, 1),
		float2(1, 1)
	};
	float ratio = frameRenderResolutionX / frameRenderResolutionY;
	float2 screenPosition = inputVertex.myPosition.xy;
	float2 pivot = ((inputVertex.myPivotOffset * 2) - 1);
	float2 normalizedImageSize;
	normalizedImageSize.x = (inputVertex.mySize.x / frameRenderResolutionX) * 0.5f;
	normalizedImageSize.y = (inputVertex.mySize.y / frameRenderResolutionY) * 0.5f;

	float cs = cos(inputVertex.myRotation);
	float sn = sin(inputVertex.myRotation);

	for (unsigned int i = 0; i < 4; i++)
	{
		FrameGeometryToPixel vertex;

		float2 rotatedPosition;
		rotatedPosition.x = ((offsets[i].x + pivot.x) * cs - (offsets[i].y + pivot.y) * sn);
		rotatedPosition.y = ((offsets[i].x + pivot.x) * sn + (offsets[i].y + pivot.y) * cs);

		rotatedPosition.x = (rotatedPosition.x * normalizedImageSize.x) + screenPosition.x;
		rotatedPosition.y = (rotatedPosition.y * normalizedImageSize.y) + screenPosition.y;
		vertex.myPosition = float4(rotatedPosition, 0, 1);
		vertex.myUV = uvs[i];
		vertex.myColor = inputVertex.myColor;
		vertex.myData = inputVertex.myData;
		vertex.mySize = inputVertex.mySize;
		vertex.myImgPos = inputVertex.myPosition;
		vertex.OGUV = float4(inputVertex.myUVOffsetTL.xy, inputVertex.myUVOffsetBR.xy);

		output.Append(vertex);
	}
}