/*

    CWP Mockup with FLTK 1.3

    (c) 2020 by Christian.Lorenz@gromeck.de

  	This file is part of CWP Mockup.

    CWP Mockup is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CWP Mockup is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CWP Mockup.  If not, see <https://www.gnu.org/licenses/>.

*/
#ifndef __COORDINATE_H__
#define __COORDINATE_H__

class Coordinate {

private:
    double x;
    double y;
    double z;
    double minX,maxX;
    double minY,maxY;
    double minZ,maxZ;

public:
    Coordinate();
    Coordinate(double x,double y,double z);
    ~Coordinate();

    void set(double x,double y,double z);
    void setX(double x);
    void setY(double y);
    void setZ(double z);

    void print(const char *title);
    void printRange(const char *title);

    void setRandom(void);

    double getX(void);
    double getY(void);
    double getZ(void);

    Coordinate operator+(const Coordinate b);
    Coordinate operator-(const Coordinate b);
    Coordinate operator*(const Coordinate b);
    Coordinate operator*(double scale);

    double getDistance(Coordinate b);
    bool isInsideRange(void);
};

#endif
