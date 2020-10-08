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
const char *const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char *const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers

	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

float degToRad(float deg){
    return deg * M_PI / 180;
}

vec2 degToRad(vec2 degVec){
    return vec2(degToRad(degVec.x), degToRad(degVec.y));
}

std::vector<vec2> degToRad(std::vector<vec2> degVector){
    std::vector<vec2> radVector;
    for (int i = 0; i < degVector.size(); ++i) {
        radVector.push_back(degToRad(degVector[i]));
    }
    return radVector;
}

std::vector<vec2> ctrlPtsEurazsia = {
        vec2(36, 0), vec2(42, 0), vec2(47, -3),
        vec2(61, 6), vec2(70, 28), vec2(65, 44),
        vec2(76, 113), vec2(60, 160), vec2(7, 105),
        vec2(19, 90), vec2(4, 80), vec2(42, 13)
};

std::vector<vec2> ctrlPtsAfrika = {
        vec2(33, -5), vec2(17, -16), vec2(3, 6),
        vec2(-35, 19), vec2(-3, 40), vec2(10, 53),
        vec2(30, 33)
};

struct mercatorParameters{
    float xRadMax = degToRad(160);
    float xRadMin = degToRad(-20);
    float yRadMax = degToRad(85);
    float yRadMin = degToRad(-85);

    float xMax = xRadMax;
    float xMin = xRadMin;
    float yMax = logf(tanf((M_PI/4)+(yRadMax/2)));
    float yMin = logf(tanf((M_PI/4)+(yRadMin/2)));

    float xScale = 10.0f / xMax;//xMin - (((abs(xMin) + abs(xMax)) / 2.0f) - abs(xMin));
    float yScale = 10.0f / yMax;
}mercatorParameters;

bool isMercator = true;

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
    std::vector<float> ts; //knots
    vec3 curveColor;
public:
    unsigned int vaoVectorisedCurve, vboVectorisedCurve;
    unsigned int vaoCtrlPoints, vboCtrlPoints;

    Curve() {
        curveColor = vec3(255, 255, 0);
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

    ~Curve() {
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

    void Draw() {
        mat4 VPTransform = camera.V() * camera.P();
        gpuProgram.setUniform(VPTransform, "MVP");

        //draw control points:
        if (wCtrlPoints.size() > 0) {
            glBindVertexArray(vaoCtrlPoints);
            glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);
            glBufferData(GL_ARRAY_BUFFER, wCtrlPoints.size() * sizeof(vec2), &wCtrlPoints[0], GL_DYNAMIC_DRAW);
            gpuProgram.setUniform(curveColor, "color");
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, 0, wCtrlPoints.size());
        }

        //draw curve:
        if (wCtrlPoints.size() > 2) {
            std::vector<vec2> vertexData;
            bool first = true;
            for (int i = 0; i < nTesselatedVertices; ++i) { //tesselate
                float tNormalized = (float) i / (nTesselatedVertices - 1);
                float t = tStart() + (tEnd() - tStart()) * tNormalized;
                vec2 wVertex = r(t);
                vertexData.push_back(wVertex);
            }
            //copy data to the GPU:
            glBindVertexArray(vaoVectorisedCurve);
            glBindBuffer(GL_ARRAY_BUFFER, vboVectorisedCurve);
            glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(vec2), &vertexData[0], GL_DYNAMIC_DRAW);
            gpuProgram.setUniform(curveColor, "color");
            glLineWidth(2.0f);
            glDrawArrays(GL_LINE_STRIP, 0, nTesselatedVertices);
        }
    }
};

//TODO: O-spline
class OSpline : public Curve {

    vec2 S(vec2 p_1, vec2 p0, vec2 p1, float t_1, float t0, float t1, float t) {

        //A = [(Y2-Y1)(X1-X3) + (Y3-Y1)(X2-X1)]/[(X1-X3)(X2^2-X1^2) + (X2-X1)(X3^2-X1^2)]
        vec2 a = ((p0 - p_1) * ((t_1 - t0) - (t1 - t0)) + (p1 - p_1) * ((t0 - t0) - (t_1 - t0))) *
                 (1.0f / (((t_1 - t0) - (t1 - t0)) * (((t0 - t0) * (t0 - t0)) - ((t_1 - t0) * (t_1 - t0))) +
                          ((t0 - t0) - (t_1 - t0)) * (((t1 - t0) * (t1 - t0)) - ((t_1 - t0) * (t_1 - t0)))));

        //B = [(Y2 - Y1) - A(X2^2 - X1^2)] / (X2-X1)
        vec2 b = ((p0 - p_1) - a * (((t0 - t0) * (t0 - t0)) - ((t_1 - t0) * (t_1 - t0)))) * (1.0f / ((t0 - t0) - (t_1 - t0)));

        //C = Y1 - AX1^2 - BX1
        vec2 c = p_1 - a * ((t_1 - t0) * (t_1 - t0)) - b * (t_1 - t0);


        return (a * (t - t0) + b) * (t - t0) + c;
    }

public:
    OSpline(std::vector<vec2> newCtrlPoints, vec3 newCurveColor) {
        curveColor = newCurveColor;

        //Kontinens:
        for (int i = 0; i < newCtrlPoints.size(); ++i) {
            float x = newCtrlPoints[i].y;
            x = degToRad(x);
            float y = newCtrlPoints[i].x;
            y = degToRad(y);

            //uj:
            y *= degToRad(90)/degToRad(85);
            x -= degToRad(70);
            x *= 2;

            x = x;
            y = logf(tanf((M_PI/4)+(y/2)));

            float Ymax = logf(tanf((M_PI/4)+(degToRad(85)/2)));
            float Xmax = degToRad(180);

            x *= 10 / Xmax;
            y *= 10 / Ymax;

            wCtrlPoints.push_back(vec2(x, y));
            ts.push_back((float) wCtrlPoints.size());
        }
        // TODO: plusz vonal
        wCtrlPoints.insert(wCtrlPoints.begin(), wCtrlPoints[wCtrlPoints.size()-1]);
        ts.push_back((float) wCtrlPoints.size());
        wCtrlPoints.push_back(wCtrlPoints[1]);
        ts.push_back((float) wCtrlPoints.size());
        wCtrlPoints.push_back(wCtrlPoints[2]);
        ts.push_back((float) wCtrlPoints.size());


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

    void AddControlPoint(float cX, float cY) {
        ts.push_back((float) wCtrlPoints.size());
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
        for (int i = 1; i < wCtrlPoints.size() - 2; i++) {
            if (ts[i] <= t && t <= ts[i + 1]) {
                //𝒓𝑡 =(𝒔𝑖 𝑡(𝑡𝑖+1−𝑡)+𝒔𝑖+1 𝑡(𝑡−𝑡𝑖))/(𝑡𝑖+1−𝑡𝑖)
                vec2 s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[i + 1], ts[i - 1], ts[i], ts[i + 1], t);
                vec2 s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[i + 2], ts[i], ts[i + 1], ts[i + 2], t);

                vec2 raaa = (s0 * (ts[i + 1] - t) + s1 * (t - ts[i])) * (1.0f / (ts[i + 1] - ts[i]));
                return raaa;
            }
        }
        return wCtrlPoints[0];
    }
};

//the virtual world: one object
OSpline *eurazsiaSpline;
OSpline *afrikaSpline;


// Initialization, create an OpenGL context
void onInitialization() {

    glViewport(0, 0, windowWidth, windowHeight);
    glLineWidth(2.0f);
    eurazsiaSpline = new OSpline(ctrlPtsEurazsia, vec3(0, 255, 0));
    afrikaSpline = new OSpline(ctrlPtsAfrika, vec3(255, 255, 0));

    //create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0, 0, 0, 0); //background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the screen

    eurazsiaSpline->Draw();
    afrikaSpline->Draw();

    glutSwapBuffers();
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'm') {
        isMercator = !isMercator;
        glutPostRedisplay();
    }         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Mouse click event
void onMouse(int button, int state, int pX,
             int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;    // flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        //eurazsiaSpline->AddControlPoint(cX, cY); // TODO: addCtrlPoint
        glutPostRedisplay();
    }
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}


// Idle event indicating that some time elapsed: do animation here
void onIdle() {
}

#pragma clang diagnostic pop