//=============================================================================================
// Created by: Ködmön Dániel Ákos
// https://github.com/KodmonDaniel
// 2021
//=============================================================================================

#include "framework.h"

template<class T> struct Dnum {
	float f;
	T d;
	Dnum(float f0 = 0, T d0 = T(0)) { f = f0, d = d0; }
	Dnum operator+(Dnum r) { return Dnum(f + r.f, d + r.d); }
	Dnum operator-(Dnum r) { return Dnum(f - r.f, d - r.d); }
	Dnum operator*(Dnum r) {
		return Dnum(f * r.f, f * r.d + d * r.f);
	}
	Dnum operator/(Dnum r) {
		return Dnum(f / r.f, (r.f * d - r.d * f) / r.f / r.f);
	}
};

template<class T> Dnum<T> Exp(Dnum<T> g) { return Dnum<T>(expf(g.f), expf(g.f) * g.d); }
template<class T> Dnum<T> Sin(Dnum<T> g) { return  Dnum<T>(sinf(g.f), cosf(g.f) * g.d); }
template<class T> Dnum<T> Cos(Dnum<T>  g) { return  Dnum<T>(cosf(g.f), -sinf(g.f) * g.d); }
template<class T> Dnum<T> Tan(Dnum<T>  g) { return Sin(g) / Cos(g); }
template<class T> Dnum<T> Sinh(Dnum<T> g) { return  Dnum<T>(sinh(g.f), cosh(g.f) * g.d); }
template<class T> Dnum<T> Cosh(Dnum<T> g) { return  Dnum<T>(cosh(g.f), sinh(g.f) * g.d); }
template<class T> Dnum<T> Tanh(Dnum<T> g) { return Sinh(g) / Cosh(g); }
template<class T> Dnum<T> Log(Dnum<T> g) { return  Dnum<T>(logf(g.f), g.d / g.f); }
template<class T> Dnum<T> Pow(Dnum<T> g, float n) {
	return  Dnum<T>(powf(g.f, n), n * powf(g.f, n - 1) * g.d);
}

typedef Dnum<vec2> Dnum2;

const int tessellationLevel = 20;

struct Camera {
	vec3 wEye, wLookat, wVup;
	float fov, asp, fp, bp;
public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = 45.0f * (float)M_PI / 180.0f;
		fp = 1;
		bp = 20;
	}
	mat4 V() {
		vec3 w = normalize(wEye - wLookat);
		vec3 u = normalize(cross(wVup, w));
		vec3 v = cross(w, u);
		return TranslateMatrix(wEye * (-1)) * mat4(u.x, v.x, w.x, 0,
			u.y, v.y, w.y, 0,
			u.z, v.z, w.z, 0,
			0, 0, 0, 1);
	}

	mat4 P() {
		float sy = 1 / tanf(fov / 2);
		return mat4(sy / asp, 0, 0, 0,
			0, sy, 0, 0,
			0, 0, -(fp + bp) / (bp - fp), -1,
			0, 0, -2 * fp * bp / (bp - fp), 0);
	}

	void Animate(float dt) {
		wEye = vec3((wEye.x - wLookat.x) * cos(dt) + (wEye.z - wLookat.z) * sin(dt) + wLookat.x,
			wEye.y,
			-(wEye.x - wLookat.x) * sin(dt) + (wEye.z - wLookat.z) * cos(dt) + wLookat.z);
	}
};

struct Material {
	vec3 kd, ks, ka;
	float shininess;
};

struct Light {
	vec3 La, Le;
	vec4 wLightPos;
	vec4 direction;
};

struct RenderState {
	mat4 MVP, M, Minv, V, P;
	Material* material;
	std::vector<Light> lights;
	vec3 wEye;
};

class Shader : public GPUProgram {
public:
	virtual void Bind(RenderState state) = 0;

	void setUniformMaterial(const Material& material, const std::string& name) {
		setUniform(material.kd, name + ".kd");
		setUniform(material.ks, name + ".ks");
		setUniform(material.ka, name + ".ka");
		setUniform(material.shininess, name + ".shininess");
	}

	void setUniformLight(const Light& light, const std::string& name) {
		setUniform(light.La, name + ".La");
		setUniform(light.Le, name + ".Le");
		setUniform(light.wLightPos, name + ".wLightPos");
		setUniform(light.direction, name + ".direction");
	}
};

class PhongShader : public Shader {
	const char* vertexSource = R"(
		#version 330
		precision highp float;
 
		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
			vec4 direction;
		};
 
		uniform mat4  MVP, M, Minv; 
		uniform Light[8] lights;    
		uniform int   nLights;
		uniform vec3  wEye;       
 
		layout(location = 0) in vec3  vtxPos;            
		layout(location = 1) in vec3  vtxNorm;      	 
		layout(location = 2) in vec2  vtxUV;
 
		out vec3 wNormal;		    
		out vec3 wView;             
		out vec3 wLight[8];
		out vec4 wPos;		   
 
		void main() {
			gl_Position = vec4(vtxPos, 1) * MVP; 
			wPos = vec4(vtxPos, 1) * M;
			for(int i = 0; i < nLights; i++) {
				wLight[i] = lights[i].wLightPos.xyz * wPos.w - wPos.xyz * lights[i].wLightPos.w;
			}
		    wView  = wEye * wPos.w - wPos.xyz;
		    wNormal = (Minv * vec4(vtxNorm, 0)).xyz;
		}
	)";

	const char* fragmentSource = R"(
		#version 330
		precision highp float;
 
		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
			vec4 direction;
		};
 
		struct Material {
			vec3 kd, ks, ka;
			float shininess;
		};
 
		uniform Material material;
		uniform Light[8] lights;   
		uniform int   nLights;
 
		in  vec3 wNormal;       
		in  vec3 wView;         
		in  vec3 wLight[8];   
		in  vec4 wPos; 
		
        out vec4 fragmentColor; 
 
	void main() {
			vec3 N = normalize(wNormal);
			vec3 V = normalize(wView); 
			if (dot(N, V) < 0) N = -N;			
			vec3 ka = material.ka;
			vec3 kd = material.kd;
 
			vec3 radiance = vec3(0, 0, 0);
			for(int i = 0; i < nLights; i++) {
				vec3 L = normalize(wLight[i]);
				vec3 H = normalize(L + V);
				float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);
					if(i == 1){  //1es a lampa fenye					
						vec3 lightDirection = normalize(lights[1].direction.xyz - wPos.xyz);
						float theta = dot(lightDirection, normalize(-lights[1].direction.xyz));
						float angle =  cos(0.33333333333 * 3.14159265);   //fix szogmeret (60deg)  
 
						if(theta > angle){
							radiance += ka * lights[i].La + (kd * cost + material.ks * pow(cosd, material.shininess)) * lights[i].Le;
						}
						else {}
					}				
						else {radiance += ka * lights[i].La + (kd * cost + material.ks * pow(cosd, material.shininess)) * lights[i].Le;
							 }
            }
			fragmentColor = vec4(radiance, 1);
		}
	)";
public:
	PhongShader() { create(vertexSource, fragmentSource, "fragmentColor"); }

	void Bind(RenderState state) {
		Use();
		setUniform(state.MVP, "MVP");
		setUniform(state.M, "M");
		setUniform(state.Minv, "Minv");
		setUniform(state.wEye, "wEye");
		setUniformMaterial(*state.material, "material");

		setUniform((int)state.lights.size(), "nLights");
		for (unsigned int i = 0; i < state.lights.size(); i++) {
			setUniformLight(state.lights[i], std::string("lights[") + std::to_string(i) + std::string("]"));
		}
	}
};

class Geometry {
protected:
	unsigned int vao, vbo;
public:
	Geometry() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
	virtual void Draw() = 0;
	~Geometry() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}
};

class ParamSurface : public Geometry {
	struct VertexData {
		vec3 position, normal;
	};

	unsigned int nVtxPerStrip, nStrips;
public:
	ParamSurface() { nVtxPerStrip = nStrips = 0; }

	virtual void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) = 0;

	VertexData GenVertexData(float u, float v) {
		VertexData vtxData;
		Dnum2 X, Y, Z;
		Dnum2 U(u, vec2(1, 0)), V(v, vec2(0, 1));
		eval(U, V, X, Y, Z);
		vtxData.position = vec3(X.f, Y.f, Z.f);
		vec3 drdU(X.d.x, Y.d.x, Z.d.x), drdV(X.d.y, Y.d.y, Z.d.y);
		vtxData.normal = cross(drdU, drdV);
		return vtxData;
	}

	void create(int N = tessellationLevel, int M = tessellationLevel) {
		nVtxPerStrip = (M + 1) * 2;
		nStrips = N;
		std::vector<VertexData> vtxData;
		for (int i = 0; i < N; i++) {
			for (int j = 0; j <= M; j++) {
				vtxData.push_back(GenVertexData((float)j / M, (float)i / N));
				vtxData.push_back(GenVertexData((float)j / M, (float)(i + 1) / N));
			}
		}
		glBufferData(GL_ARRAY_BUFFER, nVtxPerStrip * nStrips * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
	}

	void Draw() {
		glBindVertexArray(vao);
		for (unsigned int i = 0; i < nStrips; i++) glDrawArrays(GL_TRIANGLE_STRIP, i * nVtxPerStrip, nVtxPerStrip);
	}
};

class Sphere : public ParamSurface {
public:
	Sphere() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * (float)M_PI, V = V * (float)M_PI;
		X = Cos(U) * Sin(V); Y = Sin(U) * Sin(V); Z = Cos(V);
	}
};

class Cylinder : public ParamSurface {
public:
	Cylinder() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * M_PI,
			X = Cos(U); Z = Sin(U); Y = V;
	}
};

class Circle : public ParamSurface {
public:
	Circle() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * (float)M_PI, V = V * (float)M_PI;
		X = Cos(U) * Sin(V); Y = 0; Z = Sin(U) * Sin(V);
	}
};

class Paraboloid : public ParamSurface {
public:
	Paraboloid() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		V = V * (float)M_PI, U = U * 2.0f * (float)M_PI;
		X = V * Cos(U); Y = V * V; Z = V * Sin(U);
	}
};

struct Object {
	int id;
	Shader* shader;
	Material* material;
	Geometry* geometry;
	vec3 scale, translation, rotationAxis;
	float rotationAngle;
	vec3 top, rotation;
public:
	Object(Shader* _shader, Material* _material, Geometry* _geometry, int _id) :
		scale(vec3(1, 1, 1)), translation(vec3(0, 0, 0)), rotationAxis(0, 0, 1), rotationAngle(0) {
		shader = _shader;
		material = _material;
		geometry = _geometry;
		id = _id;
		if (id == 3)top = vec3(0, 2, 0);
		else if (id == 5) { top = vec3(0, 4.2, 0); rotation = vec3(0, 2, 0);}
		else if (id == 7) { top = vec3(0, 4.0, 0); rotation = vec3(0, 0.5, 0);}	
	}

	void Draw(RenderState state) {
		mat4 M = ScaleMatrix(scale) * RotationMatrix(rotationAngle, rotationAxis) * TranslateMatrix(translation);
		mat4 Minv = TranslateMatrix(-translation) * RotationMatrix(-rotationAngle, rotationAxis) * ScaleMatrix(vec3(1 / scale.x, 1 / scale.y, 1 / scale.z));
		state.M = M;
		state.Minv = Minv;
		state.MVP = state.M * state.V * state.P;
		state.material = material;
		shader->Bind(state);
		geometry->Draw();
	}

	virtual void Animate(float dt) {
		if (id == 3) {

			rotationAxis = vec3(0, 0, 1);
			rotationAngle = rotationAngle + dt;
			vec3 newTop = vec3(top.x * cosf(dt) - top.y * sinf(dt),
				top.x * sinf(dt) + top.y * cosf(dt),
				top.z);
			top = newTop;
		}
		if (id == 5) {

			rotationAxis = vec3(1, 0, 0);
			rotationAngle = rotationAngle + dt;
			top = vec3(translation.x, translation.y - 0.15f, translation.z);

			rotation = vec3(0,
				rotation.y * cosf(dt) - rotation.z * sinf(dt),
				rotation.y * sinf(dt) + rotation.z * cosf(dt));
			top = top + rotation;
		}
		if (id == 7) {

			rotationAxis = vec3(1, 0, 0);
			rotationAngle = rotationAngle + dt;

			top = vec3(translation.x, translation.y + 0.15f, translation.z);

			rotation = vec3(0,
				rotation.y * cosf(dt) - rotation.z * sinf(dt),
				rotation.y * sinf(dt) + rotation.z * cosf(dt));
			top = top + rotation;
		}
	}
};

class Scene {
	std::vector<Object*> objects;
	Camera camera;
	std::vector<Light> lights;
	int counter0 = 0; int counter1 = 0; int counter2 = 0;
	bool flag = true; bool flag1 = true; bool flag2 = true;
public:
	void Build() {
		Shader* phongShader = new PhongShader();

		Material* material0 = new Material;
		material0->kd = vec3(0.1f, 0.1f, 0.4f);
		material0->ks = vec3(0.5f, 0.5f, 0.5f);
		material0->ka = vec3(0.1f, 0.1f, 0.4f);
		material0->shininess = 50;

		Material* material1 = new Material;
		material1->kd = vec3(0.4f, 0.2f, 0.05f);
		material1->ks = vec3(0.2, 0.2, 0.2);
		material1->ka = vec3(0.4f, 0.2f, 0.05f);
		material1->shininess = 30;

		Material* material2 = new Material;
		material2->kd = vec3(0.9f, 0.9f, 0.9f);
		material2->ks = vec3(10.2, 10.2, 10.2);
		material2->ka = vec3(0.9f, 0.9f, 0.9f);
		material2->shininess = 1;

		Geometry* sphere = new Sphere();
		Geometry* cylinder = new Cylinder();
		Geometry* circle = new Circle();
		Geometry* paraboloid = new Paraboloid();

		Object* floor0 = new Object(phongShader, material1, circle, 8);
		floor0->translation = vec3(0, 0, 0);
		floor0->scale = vec3(30, 30, 30);
		objects.push_back(floor0);

		Object* cylinder1 = new Object(phongShader, material0, cylinder, 0);
		cylinder1->translation = vec3(0, 0, 0);
		cylinder1->scale = vec3(1, 0.167, 1);
		objects.push_back(cylinder1);

		Object* sphere1 = new Object(phongShader, material0, sphere, 1);
		sphere1->translation = vec3(0, 0.17, 0);
		sphere1->scale = vec3(0.2f, 0.2f, 0.2f);
		objects.push_back(sphere1);

		Object* circle1 = new Object(phongShader, material0, circle, 2);
		circle1->translation = vec3(0, 0.165, 0);
		objects.push_back(circle1);

		Object* cylinder2 = new Object(phongShader, material0, cylinder, 3);
		cylinder2->translation = vec3(0, 0.17, 0);
		cylinder2->scale = vec3(0.1f, 2.0f, 0.1f);
		objects.push_back(cylinder2);

		Object* sphere2 = new Object(phongShader, material0, sphere, 4);
		sphere2->translation = vec3(0, 2.2, 0);
		sphere2->scale = vec3(0.2f, 0.2f, 0.2f);
		objects.push_back(sphere2);

		Object* cylinder3 = new Object(phongShader, material0, cylinder, 5);
		cylinder3->translation = vec3(0, 2.2, 0);
		cylinder3->scale = vec3(0.1f, 2.0f, 0.1f);
		objects.push_back(cylinder3);

		Object* sphere3 = new Object(phongShader, material0, sphere, 6);
		sphere3->translation = vec3(0, 4.2, 0);
		sphere3->scale = vec3(0.2f, 0.2f, 0.2f);
		objects.push_back(sphere3);

		Object* paraboloid1 = new Object(phongShader, material0, paraboloid, 7);
		paraboloid1->translation = vec3(0, 4, 0);
		paraboloid1->scale = vec3(0.3, 0.15, 0.3);
		objects.push_back(paraboloid1);

		Object* bulb = new Object(phongShader, material2, sphere, 8);
		bulb->translation = vec3(0, 4.4, 0);
		bulb->scale = vec3(0.3f, 0.3f, 0.3f);
		objects.push_back(bulb);

		camera.wEye = vec3(0, 10, 6);
		camera.wLookat = vec3(0, 2, 0);
		camera.wVup = vec3(0, 1, 0);

		lights.resize(2);
		lights[0].wLightPos = vec4(1, 1, 1, 0);
		lights[0].La = vec3(0.2f, 0.2f, 0.2);
		lights[0].Le = vec3(0.6, 0.6, 0.6);
		lights[0].direction = vec4(0, 0, 0, 1);

		lights[1].wLightPos = vec4(0, 4.2, 0, 1);
		lights[1].La = vec3(0.0f, 0.0f, 0.0f);
		lights[1].Le = vec3(2.9, 2.9, 2.9);
		lights[1].direction = vec4(0, 5, 0, 1);
	}

	void Render() {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		state.lights = lights;
		for (Object* obj : objects) obj->Draw(state);
	}



	void Animate(float dt) {
		vec3 cTop1, cTop2, focus; vec3 dir;
		for (Object* obj : objects) {
			if (obj->id == 3 && flag) {
				if (counter0 == 100) { flag = false; counter0 = 0; }
				obj->Animate(dt);
				cTop1 = obj->top;
				counter0++;
			}

			if (obj->id == 3 && !flag) {
				if (counter0 == 100) { flag = true; counter0 = 0; }
				obj->Animate(-dt);
				cTop1 = obj->top;
				counter0++;
			}
			if (obj->id == 4) {
				vec3 correct = cTop1;
				correct.y += 0.15f;
				obj->translation = correct;
			}
			if (obj->id == 5 && flag1) {
				vec3 correct = cTop1;
				correct.y += 0.15f;
				obj->translation = correct;

				if (counter1 == 200) { flag1 = false; counter1 = 0; }
				obj->Animate(dt);

				cTop2 = obj->top;
				counter1++;
			}
			if (obj->id == 5 && !flag1) {
				vec3 correct = cTop1;
				correct.y += 0.15f;
				obj->translation = correct;

				if (counter1 == 200) { flag1 = true; counter1 = 0; }
				obj->Animate(-dt);
				cTop2 = obj->top;
				counter1++;
			}
			if (obj->id == 6) {
				vec3 correct1 = cTop2;
				correct1.y += 0.15f;
				obj->translation = correct1;
			}
			if (obj->id == 7 && flag2) {
				vec3 correct2 = cTop2;
				correct2.y += 0.15f;
				obj->translation = correct2;

				if (counter2 == 100) { flag2 = false; counter2 = 0; }
				obj->Animate(dt * 3);
				focus = obj->top;
				counter2++;
			}
			if (obj->id == 7 && !flag2) {
				vec3 correct2 = cTop2;
				correct2.y += 0.15f;
				obj->translation = correct2;

				if (counter2 == 100) { flag2 = true; counter2 = 0; }
				obj->Animate(-dt * 3);
				focus = obj->top;
				counter2++;
			}
			if (obj->id == 8) {
				vec3 correct2 = focus;
				//correct2.y += 0.15f;
				obj->translation = correct2;
			}
		}
		camera.Animate(dt);

		lights[1].wLightPos = vec4(focus.x, focus.y, focus.z, 1);
		dir = focus - cTop2 / 1.5;
		lights[1].direction = vec4(dir.x, dir.y, dir.z, 1);
	}
};

Scene scene;

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	scene.Build();
}

void onDisplay() {
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene.Render();
	glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) { }

void onKeyboardUp(unsigned char key, int pX, int pY) { }

void onMouse(int button, int state, int pX, int pY) { }

void onMouseMotion(int pX, int pY) { }

void onIdle() {
	float speed = 1;
	static float tend = 0;
	const float dt = 0.01f;
	float tstart = tend;
	tend = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	tend *= speed;
	for (float t = tstart; t < tend; t += dt) {
		float Dt = fmin(dt, tend - t);
		scene.Animate(Dt);
	}
	glutPostRedisplay();
}