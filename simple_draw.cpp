//=============================================================================================
// Created by: Ködmön Dániel Ákos
// https://github.com/KodmonDaniel
// 2021
//=============================================================================================

#include "framework.h"

class Shader : public GPUProgram {
	const char* const vertexSource = R"(
		#version 330
		precision highp float;

		uniform mat4 MVP;
		layout(location = 0) in vec4 vp;	
		
void main() { 
			gl_Position = vp * MVP;   
		}
	)";

	const char* const fragmentSource = R"(
		#version 330
		precision highp float;

		uniform vec3 color;
		out vec4 outColor;		

		void main() { 
			outColor = vec4(color, 1);
		}
	)";

	unsigned int vao, vbo;

public:

	Shader() {
		create(vertexSource, fragmentSource, "outColor");
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);  
		glVertexAttribPointer(0,4, GL_FLOAT, GL_FALSE, 0, NULL); 
		
	}
	void Draw(int type, vec3 color, std::vector<vec4> points) {
		if (points.size() == 0) return;
		setUniform(color, "color");
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec4), &points[0], GL_STATIC_DRAW);

		glDrawArrays(type, 0, points.size());
	}

	~Shader() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

Shader* gpuProgram;

class Object {
public:
	virtual bool Contain(vec4 r) = 0;
	virtual void setPick(bool tf) = 0;
	virtual int getType() = 0;
	virtual void Draw() = 0;
	virtual vec4 getPropData() = 0;
};

class Point : public Object {
	vec4 p;
	vec3 color;
	bool picked = false;
public:
	Point(vec4 pIn) {
		p = pIn;
		color = vec3(1, 1, 0);
	}

	int getType() { return 0; }

	void setPick(bool tf) { picked = tf; }

	bool Contain(vec4 in) {					
		if (dot(in - p, in - p) - 0.02f * 0.02f < 0) { return true; };
		return false;
	}

	void Draw() {
		if (picked) { color = vec3(1, 1, 1); }
		else { color = vec3(1, 1, 0); }
		gpuProgram->Draw(GL_POINTS, color, { p });
	}

	vec4 getPropData() { return vec4(p.x, p.y, 0, 0); }
};


class Line : public Object {
	vec4 p, q;     float m, c;
	vec3 color;
	bool picked = false;
public:
	Line(vec4 pIn, vec4 qIn) {
		color = vec3(1, 0, 0);
 		m = (qIn.y - pIn.y) / (qIn.x - pIn.x);
		c = pIn.y - m * pIn.x;

		if (m == 0) {p = vec4(-1.0f, pIn.y, 0.0f, 1.0f); q = vec4(1.0f, qIn.y, 0.0f, 1.0f);	}

		else if (isinf(m)) { p = vec4(pIn.x, 1.0f, 0.0f, 1.0f); q = vec4(qIn.x, -1.0f, 0.0f, 1.0f); }
		
		else {
			float px; bool pInited = false;
			float py;	
		
			if ((m * 1.0f + c) > -1.0f && (m * 1.0f + c) <= 1.0f){				
				p = vec4(1.0f, (m * 1.0f + c), 0.0f, 1.0f);
				pInited = true;
			}

			if (((-1.0f - c) / m) > -1.0f && ((-1.0f - c) / m) <= 1.0f) {				
				if (pInited) {
					q = vec4(((-1.0f - c) / m), -1.0f, 0.0f, 1.0f);
				}
				else{
					p = vec4(((-1.0f - c) / m), -1.0f, 0.0f, 1.0f);
					pInited = true;
				}		
			}

			if ((m * -1.0f + c) >= -1.0f && (m * -1.0f + c) < 1.0f) {		
				if (pInited) {
					q = vec4(-1.0f, (m * -1.0f + c), 0.0f, 1.0f);
				}
				else {
					p = vec4(-1.0f, (m * -1.0f + c), 0.0f, 1.0f);
					pInited = true;
				}
			}

			if (((1.0f - c) / m) >= -1.0f && ((1.0f - c) / m) < 1.0f) {				
				if (pInited) {
					q = vec4(((1.0f - c) / m), 1.0f, 0.0f, 1.0f);
				}
			}
		}
	}

	int getType() { return 1; }

	bool Contain(vec4 in) {
		if (p.x == q.x && fabs(in.x - p.x) <= 0.02f) { return true; }    
		else if ((m == 0) && (fabs(in.y - (m * in.x + c)) <= 0.02f)) { return true; }
		else {
			
			float x, y, dist, powx, powy;

			x = (in.x + m * in.y - m * c) / (m * m + 1);
			y = (-(x - in.x) / m + in.y);

			powx = (in.x - x) * (in.x - x);
			powy = (in.y - y) * (in.y - y);
			
			dist = sqrt(powx + powy);

			if (dist <= 0.02f) { return true; }
		
		}

		return false;
	}

	void setPick(bool tf) { picked = tf; }

	void Draw() {
		if (picked) { color = vec3(1, 1, 1); }
		else { color = vec3(1, 0, 0); }
		gpuProgram->Draw(GL_LINES, color, { p, q });
	}

	vec4 getPropData() { return vec4(m,c,p.x,p.y); }
};

class Circle : public Object {
	vec4 c;
	float r;
	vec3 color;
	bool picked = false;

	std::vector<vec4> verticles;
public:
	Circle(vec4 cIn, float rIn) {
		c = cIn; r = rIn;
		color = vec3(0, 1, 1);

		float dphi = 2.0f * M_PI;

		for (int i = 0; i < 360; i++) {
			vec4 verticle = vec4(c.x + r * cosf(i * dphi / 360), c.y + r * sinf(i * dphi / 360), 0, 1);
			verticles.push_back(verticle);
		}
	}

	int getType() { return 2; }

	bool Contain(vec4 in) {  
		float f = (in.x - c.x); float s = (in.y - c.y);
		f = f * f; s = s * s;
		float rSquareMin = (r - 0.02f) * (r - 0.02f);
		float rSquareMax = (r + 0.02f) * (r + 0.02f);
		
		if(((f + s) > rSquareMin) && ((f + s) < rSquareMax)){ return true; }  	
		return false;
	}

	void setPick(bool tf) { picked = tf; }

	void Draw() {
		if (picked) { color = vec3(1, 1, 1); }
		else { color = vec3(0, 1, 1); }
		gpuProgram->Draw(GL_LINE_LOOP, color, verticles);
	}

	vec4 getPropData() { return vec4(c.x,c.y,r,0); }
};


class VirtualScene {
	std::vector<Object*> objects;
	float radius = 0.0f;  
	Object* picked = nullptr;      
	Object* picked2 = nullptr;    
public:
	void Add(Object* o) { objects.push_back(o); }  

	int Pick(vec4 &p, bool pointInt) {
		if (pointInt == true) {
			for (auto o : objects) {
				if ((o->getType() == 0) && (o->Contain(p))) {
					picked = o; o->setPick(true); return o->getType();
				}
			}
		}
		else {
			for (auto o : objects) {
				if (o->Contain(p)) { picked = o; o->setPick(true); return o->getType(); }
			}
		}

		return 3;		
	}

	int Pick2(vec4 &p, bool pointInt) {
		if (pointInt == true) {
			for (auto o : objects) {
				if ((o->getType() == 0) && (o->Contain(p))) {
					picked2 = o; o->setPick(true); return o->getType();
				}
			}
		}
		else {
			for (auto o : objects) {
				if (o->Contain(p)) { picked2 = o; o->setPick(true); return o->getType(); }
			}
		}

		return 3;
	}

	void DrawScene() {
		for (auto o : objects) {
			if (o->getType() == 2) o->Draw();
		}
		
		for (auto o : objects) {
			if (o->getType() == 1) o->Draw();
		}

		for (auto o : objects) {
			if (o->getType() == 0) o->Draw();
		}
	}

	void setRad() {

		vec4 po = picked->getPropData();
		vec4 po2 = picked2->getPropData();

		float x = fabs(po.x - po2.x);
		float y = fabs(po.y - po2.y);

		x = x * x;
		y = y * y;

		float r = x + y;

		r = sqrt(r);
		radius = r;  	
	}

	float getRad() { return radius; }

	void newCircle() {

		vec4 po = picked->getPropData();

		Circle* circle = new Circle(vec4(po.x,po.y,0.0f,1.0f), radius);
		Add(circle);
	}

	void newLine() {
	
		vec4 po = picked->getPropData();
		vec4 po2 = picked2->getPropData();

		Line* line = new Line(vec4(po.x, po.y, 0.0f, 1.0f), vec4(po2.x, po2.y, 0.0f, 1.0f));
		Add(line);	
	}

	void interSecLL() {         
	
		vec4 l1 = picked->getPropData();
		vec4 l2 = picked2->getPropData();

		vec4 i; float x, y;

		if (isinf(l1.x)) {
			x = l1.z;
			y = l2.y + (l2.x * x);  

			Point* np = new Point(vec4(x, y, 0.0f, 1.0f));
			Add(np);
			return;
		}

		else if(isinf(l2.x)) {
			x = l2.z;
			y = l1.y + l1.x * x; 

			Point* np = new Point(vec4(x, y, 0.0f, 1.0f));
			Add(np);
			return;
		}

		else if (l1.x != l2.x) {
			x = (l2.y - l1.y) / (l1.x - l2.x);
		    y = l1.x * x + l1.y;

			i = vec4(x, y, 0.0f, 1.0f);
			
			Point* np = new Point(vec4(i.x, i.y, 0.0f, 1.0f));
			Add(np);
			return;
		}
		else { }
	}

	void interSecLC() {       

		vec4 o1Data = picked->getPropData();
		vec4 o2Data = picked2->getPropData();

		float r1, x1, y1, m, c, pX, pY, iX1, iY1, iX2, iY2, qA, qB, qC;
		float qbSqr, fourAC, discr, doubleqA;

		if (picked->getType() == 1) {
			x1 = o2Data.x; y1 = o2Data.y; r1 = o2Data.z;
			m = o1Data.x; c = o1Data.y; pX = o1Data.z; pY = o1Data.w;
		}
		else {
			vec4 o1Data = picked->getPropData();
			vec4 o2Data = picked2->getPropData();

			x1 = o1Data.x; y1 = o1Data.y; r1 = o1Data.z;
			m = o2Data.x; c = o2Data.y; pX = o2Data.z; pY = o2Data.w;
		}

		if (isinf(m)) {

			qB = (-2) * (y1);									
			qC = (y1 * y1) - (r1 * r1) + (x1 * x1) - (2 * pX * x1) + (pX * pX);
            
			qbSqr = qB * qB;
			fourAC = 4 * qC;
			discr = qbSqr - fourAC;

			if (discr > 0) {   

						iY1 = (-qB - sqrt(discr)) / 2;
						iY2 = (-qB + sqrt(discr)) / 2;

				Point* p1 = new Point(vec4(pX, iY1, 0.0f, 1.0f));
				Point* p2 = new Point(vec4(pX, iY2, 0.0f, 1.0f));
				Add(p1);
				Add(p2);
			}
			else if (discr == 0) {    

				iY1 = y1;

				Point* p = new Point(vec4(pX, iY1, 0.0f, 1.0f));
				Add(p);
			}
			else {}
		}

		else {
			qA = m * m + 1;  
			qB = 2 * ((m * c) - ( m * y1) - x1);
			qC = (y1 * y1) - (r1 * r1) + (x1 * x1) - (2 * c * y1) + (c * c);

			qbSqr = qB * qB;                          
			fourAC = 4 * qA * qC;
			discr = qbSqr - fourAC;
			doubleqA = qA * 2;

			if (discr > 0.0f) {   
		
				iX1 = (-qB + sqrt(discr)) / doubleqA;
				iY1 = m * iX1 + c;

				iX2 = (-qB - sqrt(discr)) / doubleqA;
				iY2 = m * iX2 + c;
				
				Point* p1 = new Point(vec4(iX1, iY1, 0.0f, 1.0f));
				Point* p2 = new Point(vec4(iX2, iY2, 0.0f, 1.0f));
				Add(p1);				
				Add(p2);
			}
			else if (discr == 0) {
				
				iX1 = -qB / doubleqA;
				iY1 = m * iX1 + c;

				Point* p = new Point(vec4(iX1, iY1, 0.0f, 1.0f));
				Add(p);
			}
			else{}
		}
	}

	void interSecCC() {   
	
		vec4 c1Data = picked->getPropData();
		vec4 c2Data = picked2->getPropData();

		float x1 = c1Data.x; float y1 = c1Data.y; float r1 = c1Data.z;
		float x2 = c2Data.x; float y2 = c2Data.y; float r2 = c2Data.z;
		float xx = x1 - x2; float yy = y1 - y2;
		float distance = sqrt((xx * xx) + (yy * yy));

		if (distance > (r1 + r2)) { return; }

			float a = (r1 * r1 - r2 * r2 + distance * distance) / (2.0f * distance);
			
			float rSqr = r1 * r1; float aSqr = a * a;
			float hSqr = rSqr - aSqr;
			float h = sqrt(hSqr);
					
			vec4 p = vec4(x1 + (a * (x2 - x1)) / distance, y1 + (a * (y2 - y1)) / distance, 0.0f, 0.1f);

			Point* p1 = new Point(vec4(p.x + (h * (y2 - y1) / distance), p.y - (h * (x2 - x1) / distance), 0.0f, 1.0f));	
			Add(p1);


			if (distance < (r1 + r2)) {
				vec4 p2xy = vec4(p.x - (h * (y2 - y1) / distance), p.y + (h * (x2 - x1) / distance), 0.0f, 1.0f);
				Point* p2 = new Point(p2xy);
				Add(p2);
			}
	}

	void DeletePicks() {
		for (auto o : objects) { o->setPick(false); }
		picked = nullptr;
		picked2 = nullptr;
	}
};

VirtualScene vs;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glPointSize(8.0f);
	glLineWidth(2.0f);
	gpuProgram = new Shader();

	

	Point* p1 = new Point(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	Point* p2 = new Point(vec4(0.2f, 0.0f, 0.0f, 1.0f));
	Line* l = new Line(vec4(0.1f, 0.0f, 0.0f, 1.0f), vec4(0.4, 0.0f, 0.0f, 1.0f));

	vs.Add(p1);
	vs.Add(p2);
	vs.Add(l);
}

void onDisplay() {
	glClearColor(0, 0, 0, 0);							
	glClear(GL_COLOR_BUFFER_BIT); 

	mat4 MVP = mat4(1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1);

	gpuProgram->setUniform(MVP, "MVP");

	vs.DrawScene();

	glutSwapBuffers();									
}


bool compassOpen = false;	bool secCoordCompass = false; vec4 psComp;
bool circle = false;
bool line = false;			bool secCoordLine = false; vec4 psLine;
bool intersection = false;	bool secCoordInter = false; vec4 osInter; int firstObjType;

void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 's':
		compassOpen = true; circle = false; line = false; intersection = false; 
		break;
	case 'c':
		compassOpen = false; circle = true; line = false; intersection = false;
		break;
	case 'l':
		compassOpen = false; circle = false; line = true; intersection = false;
		break;
	case 'i':
		compassOpen = false; circle = false; line = false; intersection = true; 
		break;
	}

	secCoordCompass = false; secCoordLine = false; secCoordInter = false;
	vs.DeletePicks();
	glutPostRedisplay();        
}

void onKeyboardUp(unsigned char key, int pX, int pY) { }

vec4 getClick(int pX, int pY) {
	const int res = 10;
	float cX = 2.0f * (((pX + res / 2) / res) * res) / windowWidth - 1;
	float cY = 1.0f - 2.0f * (((pY + res / 2) / res) * res) / windowHeight;
	return vec4(cX, cY, 0, 1);
}
  
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {

		if (secCoordCompass) {
			compassOpen = false;
			vec4 pf = vec4(getClick(pX, pY));           	
			if (vs.Pick2(pf,true) == 0) {
				vs.setRad();
				vs.DeletePicks();
				secCoordCompass = false;
			}

			glutPostRedisplay();
		}

		else if (compassOpen) {
			psComp = vec4(getClick(pX, pY));
			if (vs.Pick(psComp,true) == 0) {
				secCoordCompass = true;
			}

			glutPostRedisplay();
		}


		else if (circle) {
			vec4 c = vec4(getClick(pX, pY));
			if (vs.Pick(c,true) == 0) {
				vs.newCircle();			
				vs.DeletePicks();
			}

			circle = false;
			glutPostRedisplay();
		}

		else if (secCoordLine) {
			line = false;
			vec4 pf = vec4(getClick(pX, pY));
			if (vs.Pick2(pf,true) == 0) {
				vs.newLine();
				vs.DeletePicks();
				secCoordLine = false;
			}

			glutPostRedisplay();
		}

		else if (line) { 

			psLine = vec4(getClick(pX, pY));
			if (vs.Pick(psLine,true) == 0) {
				secCoordLine = true;
			}

			glutPostRedisplay();
		}

		else if (secCoordInter) {
			intersection = false;
			vec4 of = vec4(getClick(pX, pY));
			if ((vs.Pick2(of,false) == 1)) {
				if(firstObjType == 1){  
					vs.interSecLL();				
				}
				
				else if (firstObjType == 2) {  
					vs.interSecLC();
				}

				secCoordInter = false;
				vs.DeletePicks();
			}
			else if (vs.Pick2(of,false) == 2) {
				
				if (firstObjType == 2) {  
					vs.interSecCC();			
				}
				else if (firstObjType == 1) { 
					vs.interSecLC();
				}

				secCoordInter = false;
				vs.DeletePicks();
			}

			glutPostRedisplay();
		}

		else if (intersection) {
			osInter = vec4(getClick(pX, pY));
			if (vs.Pick(osInter,false) == 1)  {
				firstObjType = 1;
				secCoordInter = true;			
			}
			else if (vs.Pick(osInter,false) == 2) {
				firstObjType = 2;
				secCoordInter = true;
			}
			else if (vs.Pick(osInter,false) == 0) {  
				vs.DeletePicks();
			}
			
			glutPostRedisplay();
		}
		else {  }
	}
}

void onMouseMotion(int pX, int pY) {}

void onIdle() { }