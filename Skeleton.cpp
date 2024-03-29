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
unsigned int r = 10;

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

vec3 toGlobe(vec2 szelessegEsHosszusag) {
    vec3 transformedCoordinates = vec3();
    float u = szelessegEsHosszusag.y;
    float v = szelessegEsHosszusag.x;

    u -= degToRad(70);
    v = degToRad(90) - v;

    float z = r * cosf(u) * sinf(v);
    float x = r * sinf(u) * sinf(v);
    float y = r * cosf(v);


    transformedCoordinates = vec3(x, y, z);
    return transformedCoordinates;
}

std::vector<vec2> toGlobe(std::vector<vec2> szelessegEsHosszusag) {
    std::vector<vec2> transformedCoordinates = std::vector<vec2>();
    //Kontinens:
    for (int i = 0; i < szelessegEsHosszusag.size(); ++i) {
        float u = szelessegEsHosszusag[i].y;
        float v = szelessegEsHosszusag[i].x;

        u -= degToRad(70);
        v = degToRad(90) - v;

        float z = r * cosf(u) * sinf(v);
        float x = r * sinf(u) * sinf(v);
        float y = r * cosf(v);


        transformedCoordinates.push_back(vec2(x, y));
    }
    return transformedCoordinates;
}

vec2 fromGlobe(vec2 transformedCoordinates) {
    vec2 szelessegEsHosszusag = vec2();
    float x = transformedCoordinates.x;
    float y = transformedCoordinates.y;
    float z = sqrtf((10 * 10) - (x * x) - (y * y));

    float v = acosf(y / r);
    float u = asinf(x / (r * sinf(v)));

    u += degToRad(70);
    v = degToRad(90) - v;

    szelessegEsHosszusag = vec2(v, u);
    return szelessegEsHosszusag;
}

//képlet forrása: Wikipedia - Great-circle distance
float distance(vec2 aPoint, vec2 bPoint) {
    float deltaL = abs(aPoint.y - bPoint.y);
    float angle = acosf((sinf(aPoint.x) * sinf(bPoint.x)) + (cosf(aPoint.x) * cosf(bPoint.x) * cosf(deltaL)));
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
vec3 colorOcean(0.15f, 0.0f, 0.9f);
vec3 colorEurazsia(0.3f, 1.0f, 0.0f);
vec3 colorAfrika(1.0f, 0.8f, 0.0f);
//példából:
vec3 colorCircle(1.0f, 1.0f, 1.0f);
vec3 colorPath(1.0f, 1.0f, 0.0f);
vec3 colorPoint(1.0f, 0.0f, 0.0f);

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

class Ocean {
    unsigned int vaoOcean, vboOcean;
    vec3 oceanColor;
    std::vector<vec2> vertexOcean;
public:

    Ocean() {
        oceanColor = colorOcean;

        vertexOcean.push_back(vec2(degToRad(0), degToRad(70)));

        for (float i = degToRad(-90); i < degToRad(90) + 0.02f; i += degToRad(170) / nTesselatedVertices) {
            vertexOcean.push_back(vec2(i, degToRad(-20)));
        }
        for (float i = -10.0f; i < 10.0f + 0.02f; i += 20.0f / nTesselatedVertices) {
            vertexOcean.push_back(fromMercator(vec2(i, 10)));
        }
        for (float i = degToRad(90); i > degToRad(-90) - 0.02f; i -= degToRad(170) / nTesselatedVertices) {
            vertexOcean.push_back(vec2(i, degToRad(160)));
        }
        for (float i = 10.0f; i > -10.0f - 0.02f; i -= 20.0f / nTesselatedVertices) {
            vertexOcean.push_back(fromMercator(vec2(i, -10)));
        }

        glGenVertexArrays(1, &vaoOcean);
        glBindVertexArray(vaoOcean);

        glGenBuffers(1, &vboOcean);
        glBindBuffer(GL_ARRAY_BUFFER, vboOcean);

        glEnableVertexAttribArray(0); //attribute array 0
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL); //attribute array, components...
    }

    void Draw() {
        mat4 VPTransform = camera.V() * camera.P();
        gpuProgram.setUniform(VPTransform, "MVP");

        std::vector<vec2> transformedCordinates = std::vector<vec2>();
        if (isMercator) transformedCordinates = toMercator(vertexOcean);
        else transformedCordinates = toGlobe(vertexOcean);

        glBindVertexArray(vaoOcean);
        glBindBuffer(GL_ARRAY_BUFFER, vboOcean);
        glBufferData(GL_ARRAY_BUFFER, transformedCordinates.size() * sizeof(vec2), &transformedCordinates[0],
                     GL_DYNAMIC_DRAW);
        gpuProgram.setUniform(oceanColor, "color");
        glDrawArrays(GL_TRIANGLE_FAN, 0, transformedCordinates.size());
    }
};

class Circle {
protected:
    unsigned int vaoCircle, vboCircle;
    vec3 circleColor;
    std::vector<vec2> vertexCircle;
public:

    Circle(float angle, bool isLat) {
        //szin beallitasa:
        circleColor = colorCircle;

        //vertex letrehozasa: x=Sz, y=h

        if (!isLat) {
            for (float i = degToRad(-20); i <= degToRad(160) + 0.02f; i += degToRad(180) / nTesselatedVertices) {
                vertexCircle.push_back(vec2(angle, i));
            }
        } else {
            for (float i = degToRad(-90); i < degToRad(90) + 0.02f; i += degToRad(170) / nTesselatedVertices) {
                vertexCircle.push_back(vec2(i, angle));
            }
        }

        //gpu-ra
        glGenVertexArrays(1, &vaoCircle);
        glBindVertexArray(vaoCircle);

        glGenBuffers(1, &vboCircle);
        glBindBuffer(GL_ARRAY_BUFFER, vboCircle);

        glEnableVertexAttribArray(0); //attribute array 0
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), NULL); //attribute array, components...
    }

    void Draw() {
        mat4 VPTransform = camera.V() * camera.P();
        gpuProgram.setUniform(VPTransform, "MVP");

        std::vector<vec2> transformedCordinates = std::vector<vec2>();
        if (isMercator) transformedCordinates = toMercator(vertexCircle);
        else transformedCordinates = toGlobe(vertexCircle);

        glBindVertexArray(vaoCircle);
        glBindBuffer(GL_ARRAY_BUFFER, vboCircle);
        glBufferData(GL_ARRAY_BUFFER, transformedCordinates.size() * sizeof(vec2), &transformedCordinates[0],
                     GL_DYNAMIC_DRAW);
        gpuProgram.setUniform(circleColor, "color");
        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_STRIP, 0, transformedCordinates.size());
    }
};

//forrás: Curve osztály forrása a grafika tananyag spline-okról szóló példakódja
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
        ts.push_back((float) wCtrlPoints.size());
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
            glPointSize(15.0f);
            glDrawArrays(GL_POINTS, 0, transformedPointCordinates.size());
        }


        //draw curve:
        if (wCtrlPoints.size() > 1) {
            std::vector<vec2> vertexData;
            bool first = true;
            float localTesselation = nTesselatedVertices*wCtrlPoints.size();
            for (int i = 0; i < localTesselation; ++i) { //tesselate
                float tNormalized = (float) i / (localTesselation - 1);
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
            glLineWidth(5.0f);
            glDrawArrays(GL_LINE_STRIP, 0, localTesselation);
        }
    }
};

class OSpline : public Curve {

    vec2 S(vec2 p_1, vec2 p0, vec2 p1, float t_1, float t0, float t1, float t) {
        vec2 a = ((p0 - p_1) * ((t_1 - t0) - (t1 - t0)) + (p1 - p_1) * ((t0 - t0) - (t_1 - t0))) *
                 (1.0f / (((t_1 - t0) - (t1 - t0)) * (((t0 - t0) * (t0 - t0)) - ((t_1 - t0) * (t_1 - t0))) +
                          ((t0 - t0) - (t_1 - t0)) * (((t1 - t0) * (t1 - t0)) - ((t_1 - t0) * (t_1 - t0)))));

        vec2 b = ((p0 - p_1) - a * (((t0 - t0) * (t0 - t0)) - ((t_1 - t0) * (t_1 - t0)))) *
                 (1.0f / ((t0 - t0) - (t_1 - t0)));

        vec2 c = p_1 - a * ((t_1 - t0) * (t_1 - t0)) - b * (t_1 - t0);

        return (a * (t - t0) + b) * (t - t0) + c;
    }

public:
    OSpline(std::vector<vec2> newCtrlPoints, vec3 newCurveColor) {
        curveColor = newCurveColor;
        pointColor = newCurveColor;

        //Kontinens:
        for (int i = 0; i < newCtrlPoints.size(); ++i) {
            wCtrlPoints.push_back(vec2(newCtrlPoints[i].x, newCtrlPoints[i].y));
            ts.push_back((float) wCtrlPoints.size());
        }

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
        return ts[ts.size() - 1] + 1;
    }

    vec2 r(float t) {
        for (int i = 0; i < wCtrlPoints.size(); i++) {
            if (ts[i] <= t && t <= ts[i + 1] && t <= ts[ts.size() - 1]) {
                if (i == 0) {//elsö pont
                    vec2 s0, s1;
                    s0 = S(wCtrlPoints[wCtrlPoints.size() - 1], wCtrlPoints[i], wCtrlPoints[i + 1], 0, ts[i], ts[i + 1],
                           t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[i + 2], ts[i], ts[i + 1], ts[i + 2], t);
                    vec2 rt = (s0 * (ts[i + 1] - t) + s1 * (t - ts[i])) * (1.0f / (ts[i + 1] - ts[i]));
                    return rt;
                } else if (i == (wCtrlPoints.size() - 2)) {//utolsó előtti pont
                    vec2 s0, s1;
                    s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[i + 1], ts[i - 1], ts[i], ts[i + 1], t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[0], ts[i], ts[i + 1], (ts[i + 1] + 1), t);
                    vec2 rt = (s0 * (ts[i + 1] - t) + s1 * (t - ts[i])) * (1.0f / (ts[i + 1] - ts[i]));
                    return rt;
                } else {
                    vec2 s0, s1;
                    s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[i + 1], ts[i - 1], ts[i], ts[i + 1], t);
                    s1 = S(wCtrlPoints[i], wCtrlPoints[i + 1], wCtrlPoints[i + 2], ts[i], ts[i + 1], ts[i + 2], t);
                    vec2 rt = (s0 * (ts[i + 1] - t) + s1 * (t - ts[i])) * (1.0f / (ts[i + 1] - ts[i]));
                    return rt;
                }
            } else if (t > ts[ts.size() - 1] && i == (wCtrlPoints.size() - 1)) {//utolsó pont utáni görbe
                vec2 s0, s1;
                s0 = S(wCtrlPoints[i - 1], wCtrlPoints[i], wCtrlPoints[0], ts[i - 1], ts[i], (ts[i] + 1), t);
                s1 = S(wCtrlPoints[i], wCtrlPoints[0], wCtrlPoints[1], ts[i], (ts[i] + 1), (ts[i] + 2), t);
                vec2 rt = (s0 * ((ts[i] + 1) - t) + s1 * (t - ts[i])) * (1.0f / ((ts[i] + 1) - ts[i]));
                return rt;
            }
        }
        return wCtrlPoints[0];
    }
};

class Path : public Curve {
    vec2 Plane(vec2 p0, vec2 p1, float t0, float t) {
        t = t - t0;
        //konstansok:
        float d = acos(sin(p0.x) * sin(p1.x) + cos(p0.x) * cos(p1.x) * cos(p0.y - p1.y));
        float A = sin((1 - t) * d) / sin(d);
        float B = sin(t * d) / sin(d);
        //koordinatak:
        float x = A * cos(p0.x) * cos(p0.y) + B * cos(p1.x) * cos(p1.y);
        float y = A * cos(p0.x) * sin(p0.y) + B * cos(p1.x) * sin(p1.y);
        float z = A * sin(p0.x) + B * sin(p1.x);
        //vissza sz,h-ban:
        return vec2(atan2(z, sqrt((x*x) + (y*y))), atan2(y,x));
    }

public:
    float tStart() {
        return ts[0];
    }

    float tEnd() {
        return ts[wCtrlPoints.size() - 1];
    }

    virtual vec2 r(float t) {
        vec2 wPoint(0, 0);
        for (int i = 0; i < wCtrlPoints.size() - 1; i++) {
            if (ts[i] <= t && t <= ts[i + 1]) {
                return Plane(wCtrlPoints[i], wCtrlPoints[i+1],ts[i], t);
            }
        }
        return wPoint;
    }
};

//the virtual world:
Ocean *ocean;
OSpline *eurazsiaSpline;
OSpline *afrikaSpline;
std::vector<Circle *> circles;
Path *path;

// Initialization, create an OpenGL context
void onInitialization() {

    glViewport(0, 0, windowWidth, windowHeight);
    glLineWidth(2.0f);
    ocean = new Ocean();
    eurazsiaSpline = new OSpline(ctrlPtsEurazsia, colorEurazsia);
    afrikaSpline = new OSpline(ctrlPtsAfrika, colorAfrika);
    for (int i = -90; i <= 90; i += 20) {
        circles.push_back(new Circle(degToRad(i), false));
    }
    for (int i = -20; i <= 160; i += 20) {
        circles.push_back(new Circle(degToRad(i), true));
    }
    path = new Path();

    //create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0, 0, 0, 0); //background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the screen

    ocean->Draw();
    eurazsiaSpline->Draw();
    afrikaSpline->Draw();
    for (int i = 0; i < circles.size(); i++) {
        circles[i]->Draw();
    }
    path->Draw();

    glutSwapBuffers();
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'm') {
        isMercator = !isMercator;
        glutPostRedisplay();
    }
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
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