#include "DebugStructs.hlsli"

[maxvertexcount(4)]
void main(
	point VertexToGeometry input[1], 
	inout TriangleStream<GeometryToPixel> output
)
{	
	GeometryToPixel vertex1;
	vertex1.myPosition = input[0].myPosFrom;
	vertex1.myPosition.x += 1.0f * input[0].mySize;
	vertex1.myPosition.y += 2.0f * input[0].mySize;
	vertex1.myPosition.z += 2.0f * input[0].mySize;
	vertex1.myColor = input[0].myColor;
	output.Append(vertex1);

	GeometryToPixel vertex2;
	vertex2.myPosition = input[0].myPosTo;
	vertex2.myPosition.x += 1.0f * input[0].mySize;
	vertex2.myPosition.y += 2.0f * input[0].mySize;
	vertex2.myPosition.z += 2.0f * input[0].mySize;
	vertex2.myColor = input[0].myColor;
	output.Append(vertex2);	

	GeometryToPixel vertex3;
	vertex3.myPosition = input[0].myPosFrom;
	vertex3.myPosition.x -= 1.0f * input[0].mySize;
	vertex3.myPosition.y -= 2.0f * input[0].mySize;
	vertex3.myPosition.z -= 2.0f * input[0].mySize;
	vertex3.myColor = input[0].myColor;
	output.Append(vertex3);	

	GeometryToPixel vertex4;
	vertex4.myPosition = input[0].myPosTo;
	vertex4.myPosition.x -= 1.0f * input[0].mySize;
	vertex4.myPosition.y -= 2.0f * input[0].mySize;
	vertex4.myPosition.z -= 2.0f * input[0].mySize;
	vertex4.myColor = input[0].myColor;
	output.Append(vertex4);	
}