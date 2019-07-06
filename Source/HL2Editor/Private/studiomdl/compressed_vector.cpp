#include "HL2EditorPrivatePCH.h"

#include "compressed_vector.h"

Vector32::Vector32(void)
	: x(0), y(0), z(0), exp(0)
{ }

Vector32::Vector32(float X, float Y, float Z)
{
	operator=(FVector(X, Y, Z));
}

Normal32::Normal32(void)
	: x(0), y(0), zneg(0)
{ }

Normal32::Normal32(float X, float Y, float Z)
{
	operator=(FVector(X, Y, Z));
}

Quaternion64::Quaternion64(void)
	: x(0), y(0), z(0), wneg(0)
{ }

Quaternion64::Quaternion64(float X, float Y, float Z, float W)
{
	operator=(FQuat(X, Y, Z, W));
}

Quaternion48::Quaternion48(void)
	: x(0), y(0), z(0), wneg(0)
{ }

Quaternion48::Quaternion48(float X, float Y, float Z, float W)
{
	operator=(FQuat(X, Y, Z, W));
}

Quaternion32::Quaternion32(void)
	: x(0), y(0), z(0), wneg(0)
{ }

Quaternion32::Quaternion32(float X, float Y, float Z, float W)
{
	operator=(FQuat(X, Y, Z, W));
}