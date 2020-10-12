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

unsigned int R = 6371;

float radToDeg(float rad) {
    return rad * 180 / M_PI;
}

float degToRad(float deg) {
    return deg * M_PI / 180;
}

vec2 degToRad(vec2 degVec) {
    return vec2(degToRad(degVec.x), degToRad(degVec.y));
}

std::vector<vec2> degToRad(std::vector<vec2> degVector) {
    std::vector<vec2> radVector;
    for (int i = 0; i < degVector.size(); ++i) {
        radVector.push_back(degToRad(degVector[i]));
    }
    return radVector;
}

bool isMercator = true;

std::vector<vec2> toMercator(std::vector<vec2> szelessegEsHosszusag) {
    std::vector<vec2> transformedCoordinates = std::vector<vec2>();
    //Kontinens:
    for (int i = 0; i < szelessegEsHosszusag.size(); ++i) {
        float x = szelessegEsHosszusag[i].y;
        float y = szelessegEsHosszusag[i].x;

        //eltolás, nagyítás a földön:
        x -= degToRad(70);
        x *= 2;
        y *= degToRad(90) / degToRad(85);

        //mercator leképzés:
        x = x;
        y = logf(tanf((M_PI / 4) + (y / 2)));

        //skálázás 10x10-esbe:
        float Xmax = degToRad(180);
        float Ymax = logf(tanf((M_PI / 4) + (degToRad(85) / 2)));

        x *= 10 / Xmax;
        y *= 10 / Ymax;

        transformedCoordinates.push_back(vec2(x, y));
    }
    return transformedCoordinates;
}

vec2 fromMercator(vec2 transformedCoordinate) {
    vec2 szelessegEsHosszusag = vec2();
    //Kontinens:
    float x = transformedCoordinate.x;
    float y = transformedCoordinate.y;

    //skálázás 10x10-esből:
    float Xmax = degToRad(180);
    float Ymax = logf(tanf((M_PI / 4) + (degToRad(85) / 2)));

    x *= Xmax / 10;
    y *= Ymax / 10;

    //mercator leképzés inverz:
    x = x;
    y = (2 * atanf(powf(M_E, y))) - (M_PI / 2);

    //visszatolás, kicsinyítés a földön:
    x *= 0.5f;
    x += degToRad(70);
    y *= degToRad(85) / degToRad(90);
    szelessegEsHosszusag = vec2(y, x);
    return szelessegEsHosszusag;
}

std::vector<vec2> toGlobe(std::vector<vec2> szelessegEsHosszusag) {
    std::vector<vec2> transformedCoordinates = std::vector<vec2>();
    //Kontinens:
    for (int i = 0; i < szelessegEsHosszusag.size(); ++i) {
        float u = szelessegEsHosszusag[i].y;
        float v = szelessegEsHosszusag[i].x;

        u -= degToRad(70);
        v = degToRad(90) - v;

        float z = 10*cosf(u)*sinf(v);
        float x = 10*sinf(u)*sinf(v);
        float y = 10*cosf(v);


        transformedCoordinates.push_back(vec2(x, y));
    }
    return transformedCoordinates;
}

vec2 fromGlobe(vec2 transformedCoordinates) {
    vec2 szelessegEsHosszusag = vec2();
    float x = transformedCoordinates.x;
    float y = transformedCoordinates.y;
    float z = sqrtf((10*10)-(x*x)-(y*y));

    float v = acosf(y/10);
    float u = asinf(x/(10*sinf(v)));

    u += degToRad(70);
    v = degToRad(90) - v;

    szelessegEsHosszusag = vec2(v, u);
    return szelessegEsHosszusag;
}

//képlet forrása: Wikipedia - Great-circle distance
float distance(vec2 aPoint, vec2 bPoint){
    float deltaL = abs(aPoint.y - bPoint.y);
    float angle = acosf((sinf(aPoint.x)*sinf(bPoint.x))+(cosf(aPoint.x)*cosf(bPoint.x)*cosf(deltaL)));
    return R * angle;
}

std::vector<vec2> ctrlPtsEurazsia = degToRad({    //Sz, H
                                                     vec2(36, 0), vec2(42, 0), vec2(47, -3),
                                                     vec2(61, 6), vec2(70, 28), vec2(65, 44),
                                                     vec2(76, 113), vec2(60, 160), vec2(7, 105),
                                                     vec2(19, 90), vec2(4, 80), vec2(42, 13)});

std::vector<vec2> ctrlPtsAfrika = degToRad({
                                                   vec2(33, -5), vec2(17, -16), vec2(3, 6),
                                                   vec2(-35, 19), vec2(-3, 40), vec2(10, 53),
                                                   vec2(30, 33)});

//Monokrómból:
vec3 colorTenger(25.5, -25.5, 127.5);
vec3 colorEurazsia(-51, 255, 0);
vec3 colorAfrika(255, 127.5, 0);
//példából:
vec3 colorCircle(255, 255, 255);
vec3 colorPath(255, 255, 0);
vec3 colorPoint(255, 0, 0);

struct mercatorParameters {
    float xRadMax = degToRad(160);
    float xRadMin = degToRad(-20);
    float yRadMax = degToRad(85);
    float yRadMin = degToRad(-85);

    float xMax = xRadMax;
    float xMin = xRadMin;
    float yMax = logf(tanf((M_PI / 4) + (yRadMax / 2)));
    float yMin = logf(tanf((M_PI / 4) + (yRadMin / 2)));

    float xScale = 10.0f / xMax;//xMin - (((abs(xMin) + abs(xMax)) / 2.0f) - abs(xMin));
    float yScale = 10.0f / yMax;
} mercatorParameters;

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


//Curve osztály forrása a grafika tananyag spline-okról szóló példakódja
class Curve {

protected:
    std::vector<vec2> wCtrlPoints; //coordinates of control points
    std::vector<float> ts; //knots
    vec3 curveColor, pointColor;
public:
    unsigned int vaoVectorisedCurve, vboVectorisedCurve;
    unsigned int vaoCtrlPoints, vboCtrlPoints;

    Curve() {
        curveColor = colorPath;
        pointColor = colorPoint;
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

    virtual vec2 r(float t) = 0;

    virtual float tStart() = 0;

    virtual float tEnd() = 0;

    virtual void AddControlPoint(float cX, float cY) {
        vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv();
        vec2 point;
        if (isMercator) point = fromMercator(vec2(wVertex.x, wVertex.y));
        else point = fromGlobe(vec2(wVertex.x, wVertex.y));
        printf("Longitude: %lf, Latitude: %lf\n", radToDeg(point.y), radToDeg(point.x));
        if (wCtrlPoints.size() > 0) printf("Distance: %lf km\n", distance(point, wCtrlPoints[wCtrlPoints.size() - 1]));
        wCtrlPoints.push_back(point);
    }

    void Draw() {
        mat4 VPTransform = camera.V() * camera.P();
        gpuProgram.setUniform(VPTransform, "MVP");


        std::vector<vec2> transformedPointCordinates = std::vector<vec2>();
        if (isMercator) transformedPointCordinates = toMercator(wCtrlPoints);
        else transformedPointCordinates = toGlobe(wCtrlPoints);

        //draw control points:
        if (wCtrlPoints.size() > 0) {
            glBindVertexArray(vaoCtrlPoints);
            glBindBuffer(GL_ARRAY_BUFFER, vboCtrlPoints);
            glBufferData(GL_ARRAY_BUFFER, transformedPointCordinates.size() * sizeof(vec2),
                         &transformedPointCordinates[0], GL_DYNAMIC_DRAW);
            gpuProgram.setUniform(pointColor, "color");
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, 0, transformedPointCordinates.size());
        }


        //draw curve:
        if (wCtrlPoints.size() > 1) {
            std::vector<vec2> vertexData;
            bool first = true;
            for (int i = 0; i < nTesselatedVertices; ++i) { //tesselate
                float tNormalized = (float) i / (nTesselatedVertices - 1);
                float t = tStart() + (tEnd() - tStart()) * tNormalized;
                vec2 wVertex = r(t);
                vertexData.push_back(wVertex);
            }


            std::vector<vec2> transformedCordinates = std::vector<vec2>();
            if (isMercator) transformedCordinates = toMercator(vertexData);
            else transformedCordinates = toGlobe(vertexData);

            //copy data to the GPU:
            glBindVertexArray(vaoVectorisedCurve);
            glBindBuffer(GL_ARRAY_BUFFER, vboVectorisedCurve);
            glBufferData(GL_ARRAY_BUFFER, transformedCordinates.size() * sizeof(vec2), &transformedCordinates[0],
                         GL_DYNAMIC_DRAW);
            gpuProgram.setUniform(curveColor, "color");
            glLineWidth(2.0f);
            glDrawArrays(GL_LINE_STRIP, 0, nTesselatedVertices);
        }
    }
};

class OSpline : public Curve {

    vec2 S(vec2 p_1, vec2 p0, vec2 p1, float t_1, float t0, float t1, float t) {

        //A = [(Y2-Y1)(X1-X3) + (Y3-Y1)(X2-X1)]/[(X1-X3)(X2^2-X1^2) + (X2-X1)(X3^2-X1^2)]
        vec2 a = ((p0 - p_1) * ((t_1 - t0) - (t1 - t0)) + (p1 - p_1) * ((t0 - t0) - (t_1 - t0))) *
                 (1.0f / (((t_1 - t0) - (t1 - t0)) * (((t0 - t0) * (t0 - t0)) - ((t_1 - t0) * (t_1 - t0))) +
                          ((t0 - t0) - (t_1 - t0)) * (((t1 - t0) * (t1 - t0)) - ((t_1 - t0) * (t_1 - t0)))));

        //B = [(Y2 - Y1) - A(X2^2 - X1^2)] / (X2-X1)
        vec2 b = ((p0 - p_1) - a * (((t0 - t0) * (t0 - t0)) - ((t_1 - t0) * (t_1 - t0)))) *
                 (1.0f / ((t0 - t0) - (t_1 - t0)));

        //C = Y1 - AX1^2 - BX1
        vec2 c = p_1 - a * ((t_1 - t0) * (t_1 - t0)) - b * (t_1 - t0);


        return (a * (t - t0) + b) * (t - t0) + c;
    }

public:
    OSpline(std::vector<vec2> newCtrlPoints, vec3 newCurveColor) {
        curveColor = newCurveColor;
        pointColor = newCurveColor;

        //wCtrlPoints.insert(wCtrlPoints.begin(), newCtrlPoints[newCtrlPoints.size() - 1]);
        //ts.push_back((float) wCtrlPoints.size());

        //Kontinens:
        for (int i = 0; i < newCtrlPoints.size(); ++i) {
            wCtrlPoints.push_back(vec2(newCtrlPoints[i].x, newCtrlPoints[i].y));
            ts.push_back((float) wCtrlPoints.size());
        }
        // TODO: plusz vonal
        //wCtrlPoints.push_back(wCtrlPoints[0]);
        //ts.push_back((float) wCtrlPoints.size());
        //wCtrlPoints.push_back(wCtrlPoints[2]);
        //ts.push_back(ts[2]);


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

    float tStart() {
        return ts[0];
    }

    float tEnd() {
        return ts[wCtrlPoints.size() - 1];
    }

    vec2 r(float t) {
        for (int i = 0; i < wCtrlPoints.size(); i++) { // TODO: Vonal fix
            if (ts[i] <= t && t <= ts[i + 1]) {
                //𝒓𝑡 =(𝒔𝑖 𝑡(𝑡𝑖+1−𝑡)+𝒔𝑖+1 𝑡(𝑡−𝑡𝑖))/(𝑡𝑖+1−𝑡𝑖)
                vec2 s0, s1;
                if (i == 0){
                    s0 = S(wCtrlPoints[wCtrlPoints.size() - 1], wCtrlPoints[i], wCtrlPoints[i + 1], ts[ts.size() - 1], ts[i], ts[i + 1], t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[i + 2], ts[i], ts[i + 1], ts[i + 2], t);
                } else if (i == wCtrlPoints.size() - 2) {
                    s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[i + 1], ts[i - 1], ts[i], ts[i + 1], t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[0], ts[i], ts[i + 1], ts[0], t);
                } else if (i == wCtrlPoints.size() - 1) {
                    s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[0], ts[i - 1], ts[i], ts[0], t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[0], wCtrlPoints[1], ts[i], ts[0], ts[1], t);
                } else {
                    s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[i + 1], ts[i - 1], ts[i], ts[i + 1], t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[i + 2], ts[i], ts[i + 1], ts[i + 2], t);
                }
                vec2 rt = (s0 * (ts[i + 1] - t) + s1 * (t - ts[i])) * (1.0f / (ts[i + 1] - ts[i]));
                return rt;
            }
        }
        return wCtrlPoints[0];
    }
};

class Path : public Curve {
    float B(int i, float t) {
        int n = wCtrlPoints.size() - 1; //n deg polynomial = n+1 pts!
        float choose = 1;
        for (int j = 1; j < i; j++) {
            choose = choose * (float) (n - j + 1) / j;
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

//the virtual world: one object
OSpline *eurazsiaSpline;
OSpline *afrikaSpline;
Path *path;

// Initialization, create an OpenGL context
void onInitialization() {

    glViewport(0, 0, windowWidth, windowHeight);
    glLineWidth(2.0f);
    eurazsiaSpline = new OSpline(ctrlPtsEurazsia, colorEurazsia);
    afrikaSpline = new OSpline(ctrlPtsAfrika, colorAfrika);
    path = new Path();

    //create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0, 0, 0, 0); //background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the screen

    eurazsiaSpline->Draw();
    afrikaSpline->Draw();
    path->Draw();

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
        path->AddControlPoint(cX, cY);
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