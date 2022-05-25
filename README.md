# C++ OpenGL graphics
2D and 3D graphics program/animation created wiht OpenGL Shading Language (GLSL) in C++. The system-level I/O and window controll is done with OpenGL Utility Toolkit (GLUT). To run the cpp files the included framework is necessary.

## 2D compass, ruler program (simple_draw.cpp)
> Shapes: dot, line, circle

Functions:
- Set compass radius
- Draw a line between two points
- Draw a circle around a point with a set radius
- Determine the intersection(s) of two shape (line, circle)

To set the compass radius, select a dot then press `S` on the keyboard then select another dot by right click. The radius will be the distance between the two selected dots. Drawing a line is similar. Select a dot then press `L` select another dot. An infinite line will be drawn containing the two selected dots. 
To draw a circle simply  press `C` then select a dot. The dot will be the center of the circle and the radius will be the previously set value. To determine intersection points (0,1,2) between line-line, circle-line, line-circle, circle-circle press `I` then select the two shapes. New dots will appear at the intersections.


## 3D lamp animation
