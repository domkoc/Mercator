//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!!
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Kocka Dominik Csaba
// Neptun : FIBRPN
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers

	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

//2D Camera
class Camera2D {
    vec2 wCenter; //Center in world coordinates
    vec2 wSize; //width and height in world coordinates
public:
    Camera2D() : wCenter(0, 0), wSize(20, 20) {}

    mat4 V() {
        return TranslateMatrix(-wCenter);
    }
    mat4 P() {
        return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y));
    }

    mat4 Vinv() {
        return TranslateMatrix(wCenter);
    }
    mat4 Pinv() {
        return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2));
    }

    void Zoom(float s) {
        wSize = wSize * s;
    }
    void Pan(vec2 t) {
        wCenter = wCenter + t;
    }

};


Camera2D camera;
GPUProgram gpuProgram; // vertex and fragment shaders

const int nTesselatedVertices = 100; //görbe aproximálása

class Curve {

protected:
    std::vector<vec2> wCtrlPoints; //coordinates of control points
public:
    unsigned int vaoVectorisedCurve, vboVectorisedCurve;
    unsigned int vaoCtrlPoints, vboCtrlPoints;
    Curve() {
        //Curve:
        glGenVertexArrays(1, &vaoVectorisedCurve);
        glBindVertexArray(vaoVectorisedCurve);

        glGenBuffers(1, &vboVectorisedCurve); //generate 1 vertex buffer object
        glBindBuffer(GL_ARRAY_BUFFER, vboVectorisedCurve);

        glEnableVertexAttribArray(0); //attribute array 0
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL); //attribute array, components...

        //Control points:
        glGenVertexArrays(1, &vaoCtrlPoints);
        glBindVertexArray(vaoCtrlPoints);

        glGenBuffers(1, &vboCtrlPoints); //generate 1 vertex buffer object
        glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);

        glEnableVertexAttribArray(0); //attribute array 0
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL); //attribute array, components...
    }

    virtual ~Curve() {
        glDeleteBuffers(1, &vboCtrlPoints);
        glDeleteVertexArrays(1, &vaoCtrlPoints);

        glDeleteBuffers(1, &vboVectorisedCurve);
        glDeleteVertexArrays(1, &vaoVectorisedCurve);

    }

    virtual vec2 r(float t) = 0;
    virtual float tStart() = 0;
    virtual float tEnd() = 0;

    virtual void AddControlPoint(float cX, float cY) {
        vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
        wCtrlPoints.push_back(vec2(wVertex.x, wVertex.y));
    }

    //returns the selected control point or -1
    int PickControlPoint(float cX, float cY) {
        vec4 hVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
        vec2 wVertex = vec2(hVertex.x, hVertex.y);
        for (unsigned int p = 0; p < wCtrlPoints.size(); p++) {
            if (dot(wCtrlPoints[p] - wVertex, wCtrlPoints[p] - wVertex) < 0.1) return p;
        }
        return -1;
    }

    void MoveControlPoint(int p, float cX, float cY) {
        vec4 hVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
        wCtrlPoints[p] = vec2(hVertex.x, hVertex.y);
    }

    void Draw() {
        mat4 VPTransform = camera.V() * camera.P();
        gpuProgram.setUniform(VPTransform, "MVP");

        //draw control points:
        if (wCtrlPoints.size() > 0) {
            glBindVertexArray(vaoCtrlPoints);
            glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);
            glBufferData(GL_ARRAY_BUFFER, wCtrlPoints.size() * sizeof(vec2), &wCtrlPoints[0], GL_DYNAMIC_DRAW);
            gpuProgram.setUniform(vec3(1, 0, 0), "color");
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, 0, wCtrlPoints.size());
        }

        //draw curve:
        if (wCtrlPoints.size() > 2) {
            std::vector<vec2> vertexData;
            for (int i = 0; i < nTesselatedVertices; ++i) { //tesselate
                float tNormalized = (float)i /(nTesselatedVertices - 1);
                float t = tStart() + (tEnd() - tStart()) * tNormalized;
                vec2 wVertex = r(t);
                vertexData.push_back(wVertex);
            }
            //copy data to the GPU:
            glBindVertexArray(vaoVectorisedCurve);
            glBindBuffer(GL_ARRAY_BUFFER, vboVectorisedCurve);
            glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(vec2), &vertexData[0], GL_DYNAMIC_DRAW);
            gpuProgram.setUniform(vec3(1, 1, 0), "color");
            glLineWidth(2.0f);
            glDrawArrays(GL_LINE_STRIP, 0, nTesselatedVertices);
        }
    }
};

//Bezier using Bernstein polynomials
class BezierCurve : public Curve {
    float B(int i, float t) {
        int n = wCtrlPoints.size() - 1; //n deg polynomial = n+1 pts!
        float choose = 1;
        for (int j = 1; j < i; j++) {
            choose = choose * (float)(n - j + 1) / j;
        }
        return choose * pow(t, i) * pow(1 - t, n - i);
    }

public:
    float tStart() { return 0; };
    float tEnd() { return 1; };

    virtual vec2 r(float t) {
        vec2 wPoint(0, 0);
        for (int i = 0; i < wCtrlPoints.size(); i++) {
            wPoint = wPoint + wCtrlPoints[i] * B(i, t);
        }
        return wPoint;
    }
};

//Lagrange curve
class LagrangeCurve : public Curve {
    std::vector<float> ts; //knots

    float L(int i, float t) {
        float Li = 1.0f;
        for (unsigned int j = 0; j < wCtrlPoints.size(); j++) {
            if (j != i) {
                Li = Li * (t - ts[j]) / (ts[i] - ts[j]);
            }
        }
        return Li;
    }

public:
    void AddControlPoint(float cX, float cY) {
        ts.push_back((float)wCtrlPoints.size());
        Curve::AddControlPoint(cX, cY);
    }
    float tStart() {
        return ts[0];
    }
    float tEnd() {
        return ts[wCtrlPoints.size() - 1];
    }

    virtual vec2 r(float t) {
        vec2 wPoint(0, 0);
        for (unsigned int i = 0; i < wCtrlPoints.size(); i++) {
            wPoint = wPoint + wCtrlPoints[i] * L(i, t);
        }
        return wPoint;
    }
};

//CatmullRomSpline
class CatmullRomSpline : public Curve {
    std::vector<float> ts; //knots

    vec2 Hermite(vec2 p0, vec2 v0, float t0, vec2 p1, vec2 v1, float t1, float t) {
        float deltat = t1 - t0;
        t = t - t0;
        float deltat2 = deltat * deltat;
        float deltat3 = deltat * deltat2;
        vec2 a0 = p0, a1 = v0;
        vec2 a2 = (p1 - p0) * 3 / deltat2 - (v1 + v0 * 2) / deltat;
        vec2 a3 = (p0 - p1) * 2 / deltat3 + (v1 + v0) / deltat;
        return ((a3 * t + a2) * t + a1) * t + a0;
    }

public:
    void AddControlPoint(float cX, float cY) {
        ts.push_back((float)wCtrlPoints.size());
        Curve::AddControlPoint(cX, cY);
    }

    float tStart() {
        return ts[0];
    }
    float tEnd() {
        return ts[wCtrlPoints.size() - 1];
    }

    vec2 r(float t) {
        vec2 wPoint(0, 0);
        for (int i = 0; i < wCtrlPoints.size() - 1; i++) {
            if (ts[i] <= t && t <= ts[i + 1]) {
                vec2 vPrev = (i > 0)? (wCtrlPoints[i] - wCtrlPoints[i - 1]) * (1.0f /(ts[i] - ts[i - 1])) : vec2(0, 0); // nem jó :-tól
                vec2 vCur = (wCtrlPoints[i + 1] - wCtrlPoints[i]) / (ts[i + 1] - ts[i]);
                vec2 vNext = (i < wCtrlPoints.size() - 2)? (wCtrlPoints[i + 2] - wCtrlPoints[i + 1]) / (ts[i + 2] - ts[i + 1]) : vec2(0, 0); //nem jó ts[i + 2]-től
                vec2 v0 = (vPrev + vCur) * 0.5f;
                vec2 v1 = (vCur + vNext) * 0.5f;
                return Hermite(wCtrlPoints[i], v0, ts[i], wCtrlPoints[i + i], v1, ts[i + 1], t);
            }
        }
        return wCtrlPoints[0];
    }
};


//TODO: O-spline
class OSpline : public Curve {
    std::vector<float> ts; //knots

    vec2 S(vec2 p0, vec2 v0, float t0, vec2 p1, vec2 v1, float t1, float t) {
        t = t - t0;

        vec2 a = p0;
        vec2 b = v0;
        vec2 c = (v1 - v0) / (2*(t1-t0));

        return (a * t + b) * t + c;
    }

public:
    void AddControlPoint(float cX, float cY) {
        ts.push_back((float)wCtrlPoints.size());
        Curve::AddControlPoint(cX, cY);
    }

    float tStart() {
        return ts[0];
    }
    float tEnd() {
        return ts[wCtrlPoints.size() - 1];
    }

    //TODO: r(t)
    vec2 r(float t) {
        for (int i = 0; i < wCtrlPoints.size() - 1; i++) {
            if (ts[i] <= t && t <= ts[i + 1]) {
                vec2 vPrev = (i > 0)? (wCtrlPoints[i] - wCtrlPoints[i - 1]) * (1.0f /(ts[i] - ts[i - 1])) : vec2(0, 0); // nem jó :-tól
                vec2 vCur = (wCtrlPoints[i + 1] - wCtrlPoints[i]) / (ts[i + 1] - ts[i]);
                vec2 vNext = (i < wCtrlPoints.size() - 2)? (wCtrlPoints[i + 2] - wCtrlPoints[i + 1]) / (ts[i + 2] - ts[i + 1]) : vec2(0, 0); //nem jó ts[i + 2]-től
                vec2 v0 = (vPrev + vCur) * 0.5f;
                vec2 v1 = (vCur + vNext) * 0.5f;
                return S(wCtrlPoints[i], v0, ts[i], wCtrlPoints[i + i], v1, ts[i + 1], t);
            }
        }
        return wCtrlPoints[0];
    }
};



//the virtual world: one object
Curve * curve;

//popup menu event handler
void processMenuEvents(int option) {
    //if (curve != nullptr) delete curve;
    switch (option) {
        case 0:
            new LagrangeCurve();
            break;
        case 1:
            new BezierCurve();
            break;
        case 2:
            new CatmullRomSpline();
            break;
        case 3:
            new OSpline();
            break;
    }
    glutPostRedisplay();
}

// Initialization, create an OpenGL context
void onInitialization() {
    int menu = glutCreateMenu(processMenuEvents); //create menu and register handler

    //add entries
    glutAddMenuEntry("Lagrange", 0);
    glutAddMenuEntry("Bezier", 1);
    glutAddMenuEntry("Catmull-Rom", 2);
    glutAddMenuEntry("O-Spline", 3);

    //attach menu to right(middle) mouse button
    glutAttachMenu(GLUT_MIDDLE_BUTTON);

    glViewport(0, 0, windowWidth, windowHeight);
    glLineWidth(2.0f);
    curve = new LagrangeCurve(); //TODO: set starting curve

    //create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0, 0, 0, 0); //background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the screen
    curve->Draw();
    glutSwapBuffers();
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

int pickedControlPoint = -1;

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        curve->AddControlPoint(cX, cY);
        glutPostRedisplay();
    }
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        pickedControlPoint = curve->PickControlPoint(cX, cY);
    }
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        pickedControlPoint = -1;
    }

}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;
    if (pickedControlPoint >= 0) {
        curve->MoveControlPoint(pickedControlPoint, cX, cY);
    }
    glutPostRedisplay(); //redraw
}


// Idle event indicating that some time elapsed: do animation here
void onIdle() {
    long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}

#pragma clang diagnostic pop