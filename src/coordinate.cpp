/*

	CWP-Mockup with FLTK 1.3

	(c) 2020 by Christian.Lorenz@gromeck.de

	This file is part of CWP-MockUp.

    CWP-MockUp is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CWP-MockUp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CWP-MockUp.  If not, see <https://www.gnu.org/licenses/>.

*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "coordinate.h"

Coordinate::Coordinate()
{
	this->set(0,0,0);
}

Coordinate::Coordinate(double x,double y,double z)
{
	this->set(x,y,z);
}

Coordinate::~Coordinate()
{
}

void Coordinate::set(double x,double y,double z)
{
	this->setX(x);
	this->setY(y);
	this->setZ(z);
}

void Coordinate::setX(double x)
{
	this->x = x;
}

void Coordinate::setY(double y)
{
	this->y = y;
}

void Coordinate::setZ(double z)
{
	this->z = z;
}

void Coordinate::setRange(double minX,double maxX,double minY,double maxY,double minZ,double maxZ)
{
	this->setRangeX(minX,maxX);
	this->setRangeY(minY,maxY);
	this->setRangeZ(minZ,maxZ);
}

void Coordinate::setRangeX(double minX,double maxX)
{
	this->minX = minX;
	this->maxX = maxX;
}

void Coordinate::setRangeY(double minY,double maxY)
{
	this->minY = minY;
	this->maxY = maxY;
}

void Coordinate::setRangeZ(double minZ,double maxZ)
{
	this->minZ = minZ;
	this->maxZ = maxZ;
}

void Coordinate::setRandom(void)
{
	this->x = this->minX + (double) rand() * (this->maxX - this->minX) / RAND_MAX;
	this->y = this->minY + (double) rand() * (this->maxY - this->minY) / RAND_MAX;
	this->z = this->minZ + (double) rand() * (this->maxZ - this->minZ) / RAND_MAX;
}

void Coordinate::print(const char *title)
{
		printf("%s(%6.1f/%6.1f/%6.1f)\n",
				(title) ? title : "",
				this->x,
				this->y,
				this->z);
}

void Coordinate::printRange(const char *title)
{
	printf("%s(%6.1f/%6.1f/%6.1f)/(%6.1f/%6.1f/%6.1f)\n",
			(title) ? title : "",
			this->minX,
			this->minY,
			this->minZ,
			this->maxX,
			this->maxY,
			this->maxZ);
}

double Coordinate::getX(void)
{
	return this->x;
}

double Coordinate::getY(void)
{
	return this->y;
}

double Coordinate::getZ(void)
{
	return this->z;
}

Coordinate Coordinate::operator+(const Coordinate b)
{
    Coordinate c;

    c.x = this->x + b.x;
    c.y = this->y + b.y;
    c.z = this->z + b.z;

    return c;
}

Coordinate Coordinate::operator+=(const Coordinate b)
{
    this->x += b.x;
    this->y += b.y;
    this->z += b.z;

    return *this;
}

Coordinate Coordinate::operator-(const Coordinate b)
{
    Coordinate c;

    c.x = this->x - b.x;
    c.y = this->y - b.y;
    c.z = this->z - b.z;

    return c;
}

Coordinate Coordinate::operator-=(const Coordinate b)
{
    this->x -= b.x;
    this->y -= b.y;
    this->z -= b.z;

    return *this;
}

Coordinate Coordinate::operator*(const Coordinate b)
{
    Coordinate c;

    c.x = this->x * b.x;
    c.y = this->y * b.y;
    c.z = this->z * b.z;

    return c;
}


Coordinate Coordinate::operator*(double scale)
{
    Coordinate c;

    c.x = this->x * scale;
    c.y = this->y * scale;
    c.z = this->z * scale;

    return c;
}

Coordinate Coordinate::operator*=(double scale)
{
    this->x *= scale;
    this->y *= scale;
    this->z *= scale;

    return *this;
}

double Coordinate::getDistance(Coordinate b)
{
	return sqrt(
			(this->x - b.x) * (this->x - b.x) +
			(this->y - b.y) * (this->y - b.y) +
			(this->z - b.z) * (this->z - b.z));
}

double Coordinate::getDistanceXY(Coordinate b)
{
	return sqrt(
			(this->x - b.x) * (this->x - b.x) +
			(this->y - b.y) * (this->y - b.y));
}

bool Coordinate::isInsideRange(void)
{
	if (this->x < this->minX || this->x > this->maxX)
		return false;
	if (this->y < this->minY || this->y > this->maxY)
		return false;
	if (this->z < this->minZ || this->z > this->maxZ)
		return false;
	return true;
}

void Coordinate::wrapToRange(void)
{
	if (this->x < this->minX)
	 	this->x = this->minX;
	if (this->x > this->maxX)
		this->x = this->maxX;

	if (this->y < this->minY)
	 	this->y = this->minY;
	if (this->y > this->maxY)
		this->y = this->maxY;

	if (this->z < this->minZ)
	 	this->z = this->minZ;
	if (this->z > this->maxZ)
		this->z = this->maxZ;
}/**/
