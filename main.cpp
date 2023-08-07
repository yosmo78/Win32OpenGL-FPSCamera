#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h> //use __declspec(restrict) on funcs that return ptrs, like __declspec(restrict) void* func();
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL\gl.h>			// Header File For The OpenGL32 Library
#include <math.h>
#include <time.h>

#define DEFAULT_SCREEN_WIDTH 680
#define DEFAULT_SCREEN_HEIGHT 420

#define PI_F 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679f
#define PI_D 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float    f32; //floating 32
typedef double   f64; //floating 64

//state
u8 Running;
u8 isPaused;
u8 isFullscreen;

//Timing
int64_t PerfCountFrequency;
LARGE_INTEGER LastCounter;

//movement
f32 speed = 10.0f; //movement speed
f32 mouseSensitivity = 5.f;

//GL
#define NUM_MODELS 2
GLuint shaderProgram = 0;
GLuint modelVAOs[NUM_MODELS];
HGLRC ourOpenGLRC = NULL;

//win32 screen
HDC hDC;
HWND WindowHandle;
RECT winRect;
RECT nonFullScreenRect;
RECT snapPosRect;
u32 midX;
u32 midY;

u32 screen_width = DEFAULT_SCREEN_WIDTH;
u32 screen_height = DEFAULT_SCREEN_HEIGHT;

//uniform locations
u32 modelUniformLoc;
u32 projViewUniformLoc;
u32 ambientUniformLoc;
u32 lightDirUniformLoc;


const char * vertexShaderSrc = 	  "#version 330 core\n"
								  "layout (location = 0) in vec3 aPos;\n"   // the position variable has attribute position 0
								  "layout (location = 1) in vec3 aNorm;\n"  // the normal variable has attribute position 1
								  "layout (location = 2) in vec4 aColor;\n" // the color variable has attribute position 2
								  ""
								  "out vec3 worldNorm;\n" // output a color to the fragment shader
								  "out vec4 vertColor;\n"
								  ""
								  "uniform mat4 u_pvMat;\n"
								  "uniform mat4 u_mMat;\n"
								  ""
								  "void main()\n"
								  "{\n"
								  "    gl_Position = (u_pvMat*u_mMat)*vec4(aPos, 1.0);\n"
								  "    worldNorm = mat3(transpose(inverse(u_mMat)))*aNorm;\n" // set ourColor to the input color we got from the vertex data
								  "    vertColor = aColor;\n"
								  "}";

const char * fragementShaderSrc = "#version 330 core\n"
								  "out vec4 FragColor;\n"  
								  "in vec3 worldNorm;\n"
								  "in vec4 vertColor;\n"
								  ""
								  "uniform vec3 inv_light_dir;\n"
								  "uniform vec4 ambient_color;\n"
								  ""
								  "void main()\n"
								  "{\n"
								  "    FragColor = max(dot(worldNorm,inv_light_dir),0.0)*vertColor + ambient_color;\n"
								  "}";

#define GL_COMPILE_STATUS   	0x8B81
#define GL_LINK_STATUS      	0x8B82
#define GL_VERTEX_SHADER    	0x8B31
#define GL_FRAGMENT_SHADER  	0x8B30
#define GL_ARRAY_BUFFER     	0x8892
#define GL_ELEMENT_ARRAY_BUFFER	0x8893
#define GL_STATIC_DRAW      	0x88E4

#ifdef _WIN64
typedef s64 khronos_ssize_t;
#else
typedef s32 khronos_ssize_t;
#endif

typedef khronos_ssize_t GLsizeiptr;
typedef char GLchar;

GLuint (__stdcall *glCreateShader)(GLenum shaderType) = NULL;
void   (__stdcall *glShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length) = NULL;
void   (__stdcall *glCompileShader)(GLuint shader) = NULL;
void   (__stdcall *glGetShaderiv)(GLuint shader, GLenum pname, GLint* params) = NULL;
void   (__stdcall *glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) = NULL;
GLuint (__stdcall *glCreateProgram)(void) = NULL;
void   (__stdcall *glAttachShader)(GLuint program, GLuint shader) = NULL;
void   (__stdcall *glLinkProgram)(GLuint program) = NULL;
void   (__stdcall *glGetProgramiv)(GLuint program, GLenum pname,GLint* params) = NULL;
void   (__stdcall *glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) = NULL;
void   (__stdcall *glDeleteShader)(GLuint shader) = NULL;
void   (__stdcall *glUseProgram)(GLuint program) = NULL;
void   (__stdcall *glGenVertexArrays)(GLsizei n, GLuint* arrays) = NULL; 
void   (__stdcall *glGenBuffers)(GLsizei n, GLuint* buffers) = NULL;
void   (__stdcall *glBindVertexArray)(GLuint array) = NULL; 
void   (__stdcall *glBindBuffer)(GLenum target, GLuint buffer) = NULL;
void   (__stdcall *glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage) = NULL;
void   (__stdcall *glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) = NULL;
void   (__stdcall *glEnableVertexAttribArray)(GLuint index) = NULL;
GLint  (__stdcall *glGetUniformLocation)(GLuint program, const GLchar *name) = NULL;
void   (__stdcall *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) = NULL;
void   (__stdcall *glUniform3fv)( GLint location, GLsizei count, const GLfloat *value) = NULL;
void   (__stdcall *glUniform4fv)( GLint location, GLsizei count, const GLfloat *value) = NULL;

typedef struct Mat3f
{
	union
	{
		f32 m[3][3];
	};
} Mat3f;

typedef struct Mat4f
{
	union
	{
		f32 m[4][4];
	};
} Mat4f;

typedef struct Vec2f
{
	union
	{
		f32 v[2];
		struct
		{
			f32 x;
			f32 y;
		};
	};
} Vec2f;

typedef struct Vec3f
{
	union
	{
		f32 v[3];
		struct
		{
			f32 x;
			f32 y;
			f32 z;
		};
	};
} Vec3f;

typedef struct Vec4f
{
	union
	{
		f32 v[4];
		struct
		{
			f32 x;
			f32 y;
			f32 z;
			f32 w;
		};
	};
} Vec4f;

typedef struct Quatf
{
	union
	{
		f32 q[4];
		struct
		{
			f32 w; //real
			f32 x;
			f32 y;
			f32 z;
		};
		struct
		{
			f32 real; //real;
			Vec3f v;
		};
	};
} Quatf;


bool movingForward = false;
bool movingLeft = false;
bool movingRight = false;
bool movingBackwards = false;
bool movingUp = false;
bool movingDown = false;

//camera
Vec3f position;
f32 rotHor;
f32 rotVert;
const f32 hFov = 60.0f;
const f32 vFov = 60.0f;

inline
void InitMat3f( Mat3f *a_pMat )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = 0; a_pMat->m[2][2] = 1;
}

//for 2D translations
inline
void InitTransMat3f( Mat3f *a_pMat, f32 x, f32 y )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0;
	a_pMat->m[2][0] = x; a_pMat->m[2][1] = y; a_pMat->m[2][2] = 1;
}

//for 2D translations
inline
void InitTransMat3f( Mat3f *a_pMat, Vec2f *a_pTrans )
{
	a_pMat->m[0][0] = 1;           a_pMat->m[0][1] = 0;           a_pMat->m[0][2] = 0;
	a_pMat->m[1][0] = 0;           a_pMat->m[1][1] = 1;           a_pMat->m[1][2] = 0;
	a_pMat->m[2][0] = a_pTrans->x; a_pMat->m[2][1] = a_pTrans->y; a_pMat->m[2][2] = 1;
}

inline
void InitRotXMat3f( Mat3f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0;                        a_pMat->m[0][2] = 0;                   
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = cosf(angle*PI_F/180.0f);  a_pMat->m[1][2] = sinf(angle*PI_F/180.0f); 
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = -sinf(angle*PI_F/180.0f); a_pMat->m[2][2] = cosf(angle*PI_F/180.0f);
}

inline
void InitRotYMat3f( Mat3f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = cosf(angle*PI_F/180.0f);  a_pMat->m[0][1] = 0; a_pMat->m[0][2] = -sinf(angle*PI_F/180.0f);
	a_pMat->m[1][0] = 0;                        a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0;                    
	a_pMat->m[2][0] = sinf(angle*PI_F/180.0f);  a_pMat->m[2][1] = 0; a_pMat->m[2][2] = cosf(angle*PI_F/180.0f); 
}

inline
void InitRotZMat3f( Mat3f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = cosf(angle*PI_F/180.0f);  a_pMat->m[0][1] = sinf(angle*PI_F/180.0f); a_pMat->m[0][2] = 0;
	a_pMat->m[1][0] = -sinf(angle*PI_F/180.0f); a_pMat->m[1][1] = cosf(angle*PI_F/180.0f); a_pMat->m[1][2] = 0;
	a_pMat->m[2][0] = 0;                        a_pMat->m[2][1] = 0; 					   a_pMat->m[2][2] = 1;
}

inline
void InitRotArbAxisMat3f( Mat3f *a_pMat, Vec3f *a_pAxis, f32 angle )
{
	f32 c = cosf(angle*PI_F/180.0f);
	f32 mC = 1.0f-c;
	f32 s = sinf(angle*PI_F/180.0f);
	a_pMat->m[0][0] = c                          + (a_pAxis->x*a_pAxis->x*mC); a_pMat->m[0][1] = (a_pAxis->y*a_pAxis->x*mC) + (a_pAxis->z*s);             a_pMat->m[0][2] = (a_pAxis->z*a_pAxis->x*mC) - (a_pAxis->y*s);            
	a_pMat->m[1][0] = (a_pAxis->x*a_pAxis->y*mC) - (a_pAxis->z*s);             a_pMat->m[1][1] = c                          + (a_pAxis->y*a_pAxis->y*mC); a_pMat->m[1][2] = (a_pAxis->z*a_pAxis->y*mC) + (a_pAxis->x*s);            
	a_pMat->m[2][0] = (a_pAxis->x*a_pAxis->z*mC) + (a_pAxis->y*s);             a_pMat->m[2][1] = (a_pAxis->y*a_pAxis->z*mC) - (a_pAxis->x*s);             a_pMat->m[2][2] = c                          + (a_pAxis->z*a_pAxis->z*mC);
}

inline
void InitOrientationMat3f( Mat3f *a_pMat, Vec3f *a_pRight, Vec3f *a_pUp, Vec3f*a_pForward )
{
	a_pMat->m[0][0] = a_pRight->x;   a_pMat->m[0][1] = a_pRight->y;   a_pMat->m[0][2] = a_pRight->z;  
	a_pMat->m[1][0] = a_pUp->x;      a_pMat->m[1][1] = a_pUp->y;      a_pMat->m[1][2] = a_pUp->z;     
	a_pMat->m[2][0] = a_pForward->x; a_pMat->m[2][1] = a_pForward->y; a_pMat->m[2][2] = a_pForward->z;
}


//need to verify ordering, may need to be flipped
inline
void Mat3fMult( Mat3f *a, Mat3f *b, Mat3f *out)
{
	out->m[0][0] = a->m[0][0]*b->m[0][0] + a->m[0][1]*b->m[1][0] + a->m[0][2]*b->m[2][0];
	out->m[0][1] = a->m[0][0]*b->m[0][1] + a->m[0][1]*b->m[1][1] + a->m[0][2]*b->m[2][1];
	out->m[0][2] = a->m[0][0]*b->m[0][2] + a->m[0][1]*b->m[1][2] + a->m[0][2]*b->m[2][2];

	out->m[1][0] = a->m[1][0]*b->m[0][0] + a->m[1][1]*b->m[1][0] + a->m[1][2]*b->m[2][0];
	out->m[1][1] = a->m[1][0]*b->m[0][1] + a->m[1][1]*b->m[1][1] + a->m[1][2]*b->m[2][1];
	out->m[1][2] = a->m[1][0]*b->m[0][2] + a->m[1][1]*b->m[1][2] + a->m[1][2]*b->m[2][2];

	out->m[2][0] = a->m[2][0]*b->m[0][0] + a->m[2][1]*b->m[1][0] + a->m[2][2]*b->m[2][0];
	out->m[2][1] = a->m[2][0]*b->m[0][1] + a->m[2][1]*b->m[1][1] + a->m[2][2]*b->m[2][1];
	out->m[2][2] = a->m[2][0]*b->m[0][2] + a->m[2][1]*b->m[1][2] + a->m[2][2]*b->m[2][2];
}

inline
void Mat3fVecMult( Mat3f *a_pMat, Vec3f *a_pIn, Vec3f *a_pOut)
{
	a_pOut->x = a_pMat->m[0][0]*a_pIn->x + a_pMat->m[1][0]*a_pIn->y + a_pMat->m[2][0]*a_pIn->z;
	a_pOut->y = a_pMat->m[0][1]*a_pIn->x + a_pMat->m[1][1]*a_pIn->y + a_pMat->m[2][1]*a_pIn->z;
	a_pOut->z = a_pMat->m[0][2]*a_pIn->x + a_pMat->m[1][2]*a_pIn->y + a_pMat->m[2][2]*a_pIn->z;
}

inline
void InitMat4f( Mat4f *a_pMat )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0; a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0; a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = 0; a_pMat->m[2][2] = 1; a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0; a_pMat->m[3][1] = 0; a_pMat->m[3][2] = 0; a_pMat->m[3][3] = 1;
}

inline
void InitTransMat4f( Mat4f *a_pMat, f32 x, f32 y, f32 z )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0; a_pMat->m[0][2] = 0; a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0; a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = 0; a_pMat->m[2][2] = 1; a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = x; a_pMat->m[3][1] = y; a_pMat->m[3][2] = z; a_pMat->m[3][3] = 1;
}

inline
void InitTransMat4f( Mat4f *a_pMat, Vec3f *a_pTrans )
{
	a_pMat->m[0][0] = 1;           a_pMat->m[0][1] = 0;           a_pMat->m[0][2] = 0;           a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;           a_pMat->m[1][1] = 1;           a_pMat->m[1][2] = 0;           a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;           a_pMat->m[2][1] = 0;           a_pMat->m[2][2] = 1;           a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = a_pTrans->x; a_pMat->m[3][1] = a_pTrans->y; a_pMat->m[3][2] = a_pTrans->z; a_pMat->m[3][3] = 1;
}

inline
void InitRotXMat4f( Mat4f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = 1; a_pMat->m[0][1] = 0;                        a_pMat->m[0][2] = 0;                       a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0; a_pMat->m[1][1] = cosf(angle*PI_F/180.0f);  a_pMat->m[1][2] = sinf(angle*PI_F/180.0f); a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0; a_pMat->m[2][1] = -sinf(angle*PI_F/180.0f); a_pMat->m[2][2] = cosf(angle*PI_F/180.0f); a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0; a_pMat->m[3][1] = 0;                        a_pMat->m[3][2] = 0;                       a_pMat->m[3][3] = 1;
}

inline
void InitRotYMat4f( Mat4f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = cosf(angle*PI_F/180.0f);  a_pMat->m[0][1] = 0; a_pMat->m[0][2] = -sinf(angle*PI_F/180.0f); a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;                        a_pMat->m[1][1] = 1; a_pMat->m[1][2] = 0;                        a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = sinf(angle*PI_F/180.0f);  a_pMat->m[2][1] = 0; a_pMat->m[2][2] = cosf(angle*PI_F/180.0f);  a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0;                        a_pMat->m[3][1] = 0; a_pMat->m[3][2] = 0;                        a_pMat->m[3][3] = 1;
}

inline
void InitRotZMat4f( Mat4f *a_pMat, f32 angle )
{
	a_pMat->m[0][0] = cosf(angle*PI_F/180.0f);  a_pMat->m[0][1] = sinf(angle*PI_F/180.0f); a_pMat->m[0][2] = 0; a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = -sinf(angle*PI_F/180.0f); a_pMat->m[1][1] = cosf(angle*PI_F/180.0f); a_pMat->m[1][2] = 0; a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;                        a_pMat->m[2][1] = 0; 					   a_pMat->m[2][2] = 1; a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0;                        a_pMat->m[3][1] = 0;                       a_pMat->m[3][2] = 0; a_pMat->m[3][3] = 1;
}

inline
void InitRotArbAxisMat4f( Mat4f *a_pMat, Vec3f *a_pAxis, f32 angle )
{
	f32 c = cosf(angle*PI_F/180.0f);
	f32 mC = 1.0f-c;
	f32 s = sinf(angle*PI_F/180.0f);
	a_pMat->m[0][0] = c                          + (a_pAxis->x*a_pAxis->x*mC); a_pMat->m[0][1] = (a_pAxis->y*a_pAxis->x*mC) + (a_pAxis->z*s);             a_pMat->m[0][2] = (a_pAxis->z*a_pAxis->x*mC) - (a_pAxis->y*s);             a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = (a_pAxis->x*a_pAxis->y*mC) - (a_pAxis->z*s);             a_pMat->m[1][1] = c                          + (a_pAxis->y*a_pAxis->y*mC); a_pMat->m[1][2] = (a_pAxis->z*a_pAxis->y*mC) + (a_pAxis->x*s);             a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = (a_pAxis->x*a_pAxis->z*mC) + (a_pAxis->y*s);             a_pMat->m[2][1] = (a_pAxis->y*a_pAxis->z*mC) - (a_pAxis->x*s);             a_pMat->m[2][2] = c                          + (a_pAxis->z*a_pAxis->z*mC); a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = 0;                                                       a_pMat->m[3][1] = 0;                                                       a_pMat->m[3][2] = 0;                                                       a_pMat->m[3][3] = 1;
}

//can't use near and far cause of stupid msvc https://stackoverflow.com/questions/3869830/near-and-far-pointers/3869852
inline
void InitPerspectiveProjectionMat4f( Mat4f *a_pMat, u64 width, u64 height, f32 a_hFOV, f32 a_vFOV, f32 nearPlane, f32 farPlane )
{
	f32 thFOV = tanf(a_hFOV*PI_F/360);
	f32 tvFOV = tanf(a_vFOV*PI_F/360);
	f32 nMinF = (nearPlane-farPlane);
	f32 xmax = nearPlane * thFOV;
	f32 ymax = nearPlane * tvFOV;
  	f32 ymin = -ymax;
  	f32 xmin = -xmax;
  	f32 w = xmax - xmin;
  	f32 h = ymax - ymin;
  	f32 aspect = height / (f32)width;
	a_pMat->m[0][0] = 2.0f*nearPlane*aspect/(w*thFOV); a_pMat->m[0][1] = 0;                        a_pMat->m[0][2] = 0;                               a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 0;                               a_pMat->m[1][1] = 2.0f*nearPlane/(h*tvFOV); a_pMat->m[1][2] = 0;                               a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 0;                               a_pMat->m[2][1] = 0;                        a_pMat->m[2][2] = (farPlane+nearPlane)/nMinF;      a_pMat->m[2][3] = -1.0f;
	a_pMat->m[3][0] = 0;                               a_pMat->m[3][1] = 0;                        a_pMat->m[3][2] = 2.0f*(farPlane*nearPlane)/nMinF; a_pMat->m[3][3] = 0;
}

inline
void Mat4fMult( Mat4f *__restrict a, Mat4f *__restrict b, Mat4f *__restrict out)
{
	out->m[0][0] = a->m[0][0]*b->m[0][0] + a->m[0][1]*b->m[1][0] + a->m[0][2]*b->m[2][0] + a->m[0][3]*b->m[3][0];
	out->m[0][1] = a->m[0][0]*b->m[0][1] + a->m[0][1]*b->m[1][1] + a->m[0][2]*b->m[2][1] + a->m[0][3]*b->m[3][1];
	out->m[0][2] = a->m[0][0]*b->m[0][2] + a->m[0][1]*b->m[1][2] + a->m[0][2]*b->m[2][2] + a->m[0][3]*b->m[3][2];
	out->m[0][3] = a->m[0][0]*b->m[0][3] + a->m[0][1]*b->m[1][3] + a->m[0][2]*b->m[2][3] + a->m[0][3]*b->m[3][3];

	out->m[1][0] = a->m[1][0]*b->m[0][0] + a->m[1][1]*b->m[1][0] + a->m[1][2]*b->m[2][0] + a->m[1][3]*b->m[3][0];
	out->m[1][1] = a->m[1][0]*b->m[0][1] + a->m[1][1]*b->m[1][1] + a->m[1][2]*b->m[2][1] + a->m[1][3]*b->m[3][1];
	out->m[1][2] = a->m[1][0]*b->m[0][2] + a->m[1][1]*b->m[1][2] + a->m[1][2]*b->m[2][2] + a->m[1][3]*b->m[3][2];
	out->m[1][3] = a->m[1][0]*b->m[0][3] + a->m[1][1]*b->m[1][3] + a->m[1][2]*b->m[2][3] + a->m[1][3]*b->m[3][3];

	out->m[2][0] = a->m[2][0]*b->m[0][0] + a->m[2][1]*b->m[1][0] + a->m[2][2]*b->m[2][0] + a->m[2][3]*b->m[3][0];
	out->m[2][1] = a->m[2][0]*b->m[0][1] + a->m[2][1]*b->m[1][1] + a->m[2][2]*b->m[2][1] + a->m[2][3]*b->m[3][1];
	out->m[2][2] = a->m[2][0]*b->m[0][2] + a->m[2][1]*b->m[1][2] + a->m[2][2]*b->m[2][2] + a->m[2][3]*b->m[3][2];
	out->m[2][3] = a->m[2][0]*b->m[0][3] + a->m[2][1]*b->m[1][3] + a->m[2][2]*b->m[2][3] + a->m[2][3]*b->m[3][3];

	out->m[3][0] = a->m[3][0]*b->m[0][0] + a->m[3][1]*b->m[1][0] + a->m[3][2]*b->m[2][0] + a->m[3][3]*b->m[3][0];
	out->m[3][1] = a->m[3][0]*b->m[0][1] + a->m[3][1]*b->m[1][1] + a->m[3][2]*b->m[2][1] + a->m[3][3]*b->m[3][1];
	out->m[3][2] = a->m[3][0]*b->m[0][2] + a->m[3][1]*b->m[1][2] + a->m[3][2]*b->m[2][2] + a->m[3][3]*b->m[3][2];
	out->m[3][3] = a->m[3][0]*b->m[0][3] + a->m[3][1]*b->m[1][3] + a->m[3][2]*b->m[2][3] + a->m[3][3]*b->m[3][3];
}

inline
void Vec3fAdd( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = a->x + b->x;
	out->y = a->y + b->y;
	out->z = a->z + b->z;
}

inline
void Vec3fMult( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = a->x * b->x;
	out->y = a->y * b->y;
	out->z = a->z * b->z;
}

inline
void Vec3fCross( Vec3f *a, Vec3f *b, Vec3f *out )
{
	out->x = (a->y * b->z) - (a->z * b->y);
	out->y = (a->z * b->x) - (a->x * b->z);
	out->z = (a->x * b->y) - (a->y * b->x);
}

inline
void Vec3fScale( Vec3f *a, f32 scale, Vec3f *out )
{
	out->x = a->x * scale;
	out->y = a->y * scale;
	out->z = a->z * scale;
}

inline
f32 Vec3fDot( Vec3f *a, Vec3f *b )
{
	return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

inline
void Vec3fNormalize( Vec3f *a, Vec3f *out )
{

	f32 mag = sqrtf((a->x*a->x) + (a->y*a->y) + (a->z*a->z));
	if(mag == 0)
	{
		out->x = 0;
		out->y = 0;
		out->z = 0;
	}
	else
	{
		out->x = a->x/mag;
		out->y = a->y/mag;
		out->z = a->z/mag;
	}
}

inline
void InitUnitQuatf( Quatf *q, f32 angle, Vec3f *axis )
{
	f32 s = sinf(angle*PI_F/360.0f);
	q->w = cosf(angle*PI_F/360.0f);
	q->x = axis->x * s;
	q->y = axis->y * s;
	q->z = axis->z * s;
}

inline
void QuatfMult( Quatf *__restrict a, Quatf *__restrict b, Quatf *__restrict out )
{
	out->w = (a->w * b->w) - (a->x* b->x) - (a->y* b->y) - (a->z* b->z);
	out->x = (a->w * b->x) + (a->x* b->w) + (a->y* b->z) - (a->z* b->y);
	out->y = (a->w * b->y) + (a->y* b->w) + (a->z* b->x) - (a->x* b->z);
	out->z = (a->w * b->z) + (a->z* b->w) + (a->x* b->y) - (a->y* b->x);
}

inline
void InitViewMat4ByQuatf( Mat4f *a_pMat, f32 horizontalAngle, f32 verticalAngle, Vec3f *a_pPos )
{
	Quatf qHor, qVert;
	Vec3f vertAxis = {cosf(horizontalAngle*PI_F/180.0f),0,sinf(horizontalAngle*PI_F/180.0f)};
	Vec3f horAxis = {0,1,0};
	InitUnitQuatf( &qVert, -verticalAngle, &vertAxis );
	InitUnitQuatf( &qHor, -horizontalAngle, &horAxis );

	Quatf qRot;
	QuatfMult( &qVert, &qHor, &qRot);

	a_pMat->m[0][0] = 1.0f - 2.0f*(qRot.y*qRot.y + qRot.z*qRot.z);                                        a_pMat->m[0][1] = 2.0f*(qRot.x*qRot.y - qRot.w*qRot.z);                                               a_pMat->m[0][2] = 2.0f*(qRot.x*qRot.z + qRot.w*qRot.y);        		                                  a_pMat->m[0][3] = 0;
	a_pMat->m[1][0] = 2.0f*(qRot.x*qRot.y + qRot.w*qRot.z);                                               a_pMat->m[1][1] = 1.0f - 2.0f*(qRot.x*qRot.x + qRot.z*qRot.z);                                        a_pMat->m[1][2] = 2.0f*(qRot.y*qRot.z - qRot.w*qRot.x);        		                                  a_pMat->m[1][3] = 0;
	a_pMat->m[2][0] = 2.0f*(qRot.x*qRot.z - qRot.w*qRot.y);                                               a_pMat->m[2][1] = 2.0f*(qRot.y*qRot.z + qRot.w*qRot.x);                                               a_pMat->m[2][2] = 1.0f - 2.0f*(qRot.x*qRot.x + qRot.y*qRot.y); 		                                  a_pMat->m[2][3] = 0;
	a_pMat->m[3][0] = -a_pPos->x*a_pMat->m[0][0] - a_pPos->y*a_pMat->m[1][0] - a_pPos->z*a_pMat->m[2][0]; a_pMat->m[3][1] = -a_pPos->x*a_pMat->m[0][1] - a_pPos->y*a_pMat->m[1][1] - a_pPos->z*a_pMat->m[2][1]; a_pMat->m[3][2] = -a_pPos->x*a_pMat->m[0][2] - a_pPos->y*a_pMat->m[1][2] - a_pPos->z*a_pMat->m[2][2]; a_pMat->m[3][3] = 1;
}

void PrintMat4f( Mat4f *a_pMat )
{
	for( u32 dwIdx = 0; dwIdx < 4; ++dwIdx )
	{
		for( u32 dwJdx = 0; dwJdx < 4; ++dwJdx )
		{
			printf("%f ", a_pMat->m[dwIdx][dwJdx] );
		}
		printf("\n");
	}
}

f32 clamp(f32 d, f32 min, f32 max) {
  const f32 t = d < min ? min : d;
  return t > max ? max : t;
}

/* make sure you are in wgl Context!*/
inline void loadGLFuncPtrs()
{
    //load needed version specific opengl funcs
    int ver = glGetString(GL_VERSION)[0] - '0'; //is opengl 2.0 compatible
    if (ver > 4) ver = 4; //future proofing
    switch (ver)
    {
        case 4: 
        {
            //opengl version 4.0 required
        }
        case 3: 
        {
            //opengl version 3.0 required
            glGenVertexArrays = (void(__stdcall*)(GLsizei n, GLuint * arrays))((void*)wglGetProcAddress("glGenVertexArrays"));
            glBindVertexArray = (void(__stdcall*)(GLuint array))((void*)wglGetProcAddress("glBindVertexArray"));
        }
        case 2:
        {
            //opengl version 2.0 required
            glCreateShader = (GLuint(__stdcall *)(GLenum shaderType))((void*)wglGetProcAddress("glCreateShader"));
            glShaderSource = (void (__stdcall*)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length))((void*)wglGetProcAddress("glShaderSource"));
            glCompileShader = (void (__stdcall*)(GLuint shader))((void*)wglGetProcAddress("glCompileShader"));
            glGetShaderiv = (void (__stdcall*)(GLuint shader, GLenum pname, GLint* params))((void*)wglGetProcAddress("glGetShaderiv"));
            glGetShaderInfoLog = (void (__stdcall*)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog))((void*)wglGetProcAddress("glGetShaderInfoLog"));
            glCreateProgram = (GLuint(__stdcall*)(void))((void*)wglGetProcAddress("glCreateProgram"));
            glAttachShader = (void (__stdcall*)(GLuint program, GLuint shader))((void*)wglGetProcAddress("glAttachShader"));
            glLinkProgram = (void (__stdcall*)(GLuint program))((void*)wglGetProcAddress("glLinkProgram"));
            glGetProgramiv = (void (__stdcall*)(GLuint program, GLenum pname, GLint* params))((void*)wglGetProcAddress("glGetProgramiv"));
            glGetProgramInfoLog = (void (__stdcall*)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog))((void*)wglGetProcAddress("glGetProgramInfoLog"));
            glDeleteShader = (void (__stdcall*)(GLuint shader))((void*)wglGetProcAddress("glDeleteShader"));
            glUseProgram = (void(__stdcall*)(GLuint program))((void*)wglGetProcAddress("glUseProgram"));
            glGenBuffers = (void(__stdcall*)(GLsizei n, GLuint * buffers))((void*)wglGetProcAddress("glGenBuffers"));
            glBindBuffer = (void(__stdcall*)(GLenum target, GLuint buffer))((void*)wglGetProcAddress("glBindBuffer"));
            glBufferData = (void(__stdcall*)(GLenum target, GLsizeiptr size, const void* data, GLenum usage))((void*)wglGetProcAddress("glBufferData"));
            glVertexAttribPointer = (void(__stdcall*)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer))((void*)wglGetProcAddress("glVertexAttribPointer"));
            glEnableVertexAttribArray = (void(__stdcall*)(GLuint index))((void*)wglGetProcAddress("glEnableVertexAttribArray"));
        	glGetUniformLocation = (GLint(__stdcall *)(GLuint program, const GLchar *name))((void*)wglGetProcAddress("glGetUniformLocation"));
        	glUniformMatrix4fv = (void(__stdcall *)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))((void*)wglGetProcAddress("glUniformMatrix4fv"));
        	glUniform3fv = (void(__stdcall *)( GLint location, GLsizei count, const GLfloat *value))((void*)wglGetProcAddress("glUniform3fv"));
        	glUniform4fv = (void(__stdcall *)( GLint location, GLsizei count, const GLfloat *value))((void*)wglGetProcAddress("glUniform4fv"));
        } break;
        case 1:
            printf("Compiled Shaders NOT SUPPORTED!\n");
            break;
        default:
            printf("UNKNOWN OPENGL MAJOR VERSION");
    }
    return;
}

// look into https://stackoverflow.com/questions/49591533/opengl-compiling-shaders-on-a-different-thread
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_parallel_shader_compile.txt
// better to inline shaders than load from a file anyways
/* shaderType is one of these GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER or GL_FRAGMENT_SHADER */
/* just give shader a name for error msgs :) */
inline GLuint LoadShaderFromString(const char* shaderString, GLenum shaderType, const char* shaderName)
{
	GLuint shaderDescriptor = glCreateShader(shaderType);
    glShaderSource(shaderDescriptor, 1, (const GLchar**) &shaderString, NULL); 
    glCompileShader(shaderDescriptor);
    GLint success;
    glGetShaderiv(shaderDescriptor, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shaderDescriptor, 1024, NULL, infoLog);
        FILE * file = fopen("GLERROR.txt","w");
        fprintf(file,"ERROR SHADER_COMPILATION_ERROR of type: %s\n%s\n", shaderName, infoLog);
        fclose(file);
        exit(-1);
    }
    return shaderDescriptor;
}

/* don't forget to use after compiling*/
inline GLuint compileShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader); //attach order does matter
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        printf("ERROR PROGRAM_LINKING_ERROR of type: PROGRAM\n%s\n",infoLog);
        exit(-1);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

inline
void InitModelVAOs()
{
	float planeVertices[] =
	{
		// positions              // vertex norms    // vertex colors
		 1000.0f,  -1.0f,  1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
		 1000.0f,  -1.0f, -1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
		-1000.0f,  -1.0f,  1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f,
		-1000.0f,  -1.0f, -1000.0f, 0.0f,  1.0f, 0.0f, 0.5882f, 0.2941f, 0.0f, 1.0f
	};

	unsigned int planeIndices[] = 
	{
		0, 1, 2,
		2, 1, 3,
		0, 2, 1,
		2, 3, 1
	};

    float cubeVertices[] = {
    	// positions          // vertex norms     //vertex colors
       	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,

       	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f,

       	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

       	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

       	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

       	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
       	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f
    };
    unsigned int cubeIndicies[] =
    {
    	 0,  1,  2, //back
    	 3,  4,  5,

    	 6,  7,  8,
    	 9, 10, 11,

    	12, 13, 14,
    	15, 16, 17,

    	18, 20, 19,
    	21, 23, 22,

    	24, 25, 26, //bottom
    	27, 28, 29,

    	30, 31, 32, //top
    	33, 34, 35
    };

    GLuint VBOs[NUM_MODELS];
    GLuint EBOs[NUM_MODELS];
    glGenVertexArrays(2, modelVAOs);
    glGenBuffers(2, VBOs);
    glGenBuffers(2, EBOs);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(modelVAOs[0]); //can only bind one VAO at a time

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    // position attribute
    //https://stackoverflow.com/questions/14249634/opengl-vaos-and-multiple-buffers
    // - first arg sets bound vertex buffer to 0th slot of vertex array, meaning in shader (location = 0) is first slot of VAO
    //Think of it like this. glBindBuffer sets a global variable (GL_ARRAY_BUFFER), then glVertexAttribPointer reads that global variable and stores it in the VAO. 
    // Changing that global variable after it's been read doesn't affect the VAO. You can think of it that way because that's exactly how it works.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // norm attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    //FOR EAO's
    // The index buffer binding is stored within the VAO. If no VAO is bound, then you cannot bind a buffer object to GL_ELEMENT_ARRAY_BUFFER (this might not be true, so verify). Meaning GL_ELEMENT_ARRAY_BUFFER is not global like GL_ARRAY_BUFFER 
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndicies), cubeIndicies, GL_STATIC_DRAW);

    glBindVertexArray(modelVAOs[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    //glBindVertexArray(NULL); in a state machine unbind here
}


void drawScene(GLuint shaderProgram, f32 deltaTime )
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE); 
    glClearColor(0.5294f, 0.8078f, 0.9216f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glBindVertexArray(modelVAOs[0]);

    Mat4f mTrans;
    InitTransMat4f( &mTrans, 0, 0, -5);
    Mat4f mRot;
    Vec3f rotAxis = {0.57735026919f,0.57735026919f,0.57735026919f};
    static f32 cubeRotAngle = 0;
    cubeRotAngle += 50.0f*deltaTime;
    InitRotArbAxisMat4f( &mRot, &rotAxis, cubeRotAngle );
    Mat4f modelMatrix;
    Mat4fMult(&mRot, &mTrans, &modelMatrix);

	glUniformMatrix4fv(modelUniformLoc, 1, GL_FALSE,(const GLfloat *)modelMatrix.m);

    Mat4f viewMatrix;
    InitViewMat4ByQuatf( &viewMatrix, rotHor, rotVert, &position );

    Mat4f projectMatrix;
    InitPerspectiveProjectionMat4f(&projectMatrix, screen_width, screen_height, hFov, vFov, 0.04f, 8000.0f );
    Mat4f viewProjMatrix;
    Mat4fMult(&viewMatrix, &projectMatrix, &viewProjMatrix);
	glUniformMatrix4fv(projViewUniformLoc, 1, GL_FALSE,(const GLfloat *)viewProjMatrix.m);

	Vec3f inv_light_dir = {0.57735026919f,0.57735026919f,0.57735026919f};
	glUniform3fv(lightDirUniformLoc, 1, (const GLfloat *)inv_light_dir.v);

	Vec4f ambientCubeColor = {0.0f,0.2f,0.0f,1.0f};
	glUniform4fv(ambientUniformLoc, 1, (const GLfloat *)ambientCubeColor.v);

    glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, 0);
    

    glBindVertexArray(modelVAOs[1]);

    Mat4f identMatrix;
    InitMat4f(&identMatrix);
    glUniformMatrix4fv(modelUniformLoc, 1, GL_FALSE,(const GLfloat *)identMatrix.m);
    Vec4f ambientPlaneColor = {0.0f,0.0f,0.0f,1.0f};
    glUniform4fv(ambientUniformLoc, 1, (const GLfloat *)ambientPlaneColor.v);
    glDrawElements(GL_TRIANGLES, 4*3, GL_UNSIGNED_INT, 0);
    SwapBuffers(hDC);

}

inline
void CenterCursor( HWND Window )
{
    GetWindowRect(Window, &winRect);
    midX = winRect.left + (screen_width / 2);
    midY = winRect.top + (screen_height / 2);
    snapPosRect.left = (s32)midX;
    snapPosRect.right = (s32)midX;
    snapPosRect.top = (s32)midY;
    snapPosRect.bottom = (s32)midY;
    if( !isPaused )
    {
        ClipCursor(&snapPosRect);
    }	
}


void Pause()
{
	if( !isPaused )
	{
		isPaused = 1;
    	ClipCursor( NULL );
    	ShowCursor( TRUE );
	}
}

void TogglePause()
{
	isPaused = isPaused ^ 1;
    if( isPaused )
    {
    	ClipCursor( NULL );
    	ShowCursor( TRUE );
    }
    else
    {
    	ClipCursor(&snapPosRect);
    	ShowCursor( FALSE );
    }
}

//look into display modes
//https://stackoverflow.com/questions/7193197/is-there-a-graceful-way-to-handle-toggling-between-fullscreen-and-windowed-mode
void Fullscreen( HWND WindowHandle )
{
	isFullscreen = isFullscreen ^ 1;
	if( isFullscreen )
	{
		nonFullScreenRect.left = winRect.left;
		nonFullScreenRect.right = winRect.right;
		nonFullScreenRect.bottom = winRect.bottom;
		nonFullScreenRect.top = winRect.top;
		SetWindowLong(WindowHandle, GWL_STYLE, WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE );
		HMONITOR hmon = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hmon, &mi);
		screen_width = mi.rcMonitor.right - mi.rcMonitor.left;
    	screen_height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    	MoveWindow(WindowHandle,mi.rcMonitor.left,mi.rcMonitor.top,(s32)screen_width,(s32)screen_height, FALSE );
        glViewport(0, 0, screen_width, screen_height); 
        CenterCursor( WindowHandle );
	}
	else
	{
		SetWindowLongPtr(WindowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		screen_width = nonFullScreenRect.right - nonFullScreenRect.left;
    	screen_height = nonFullScreenRect.bottom - nonFullScreenRect.top;
    	MoveWindow(WindowHandle,nonFullScreenRect.left,nonFullScreenRect.top,(s32)screen_width,(s32)screen_height, FALSE );
        glViewport(0, 0, screen_width, screen_height); 
        CenterCursor( WindowHandle );
	}
}

void CloseProgram()
{
	Running = 0;
}


int logWindowsError(const char* msg)
{
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    OutputDebugStringA(msg);
    OutputDebugStringA((LPCTSTR)lpMsgBuf);

    LocalFree(lpMsgBuf);
    return -1;
}

LRESULT CALLBACK
Win32MainWindowCallback(
    HWND Window,   //HWND a handle to a window
    UINT Message,  //System defined message, Window will call us and ask us to respond it
    WPARAM WParam, //
    LPARAM LParam) //
{
    LRESULT Result = 0;
    switch (Message)
    {
    case WM_SIZE:
    {
    	screen_width = LOWORD(LParam);
    	screen_height = HIWORD(LParam);
        glViewport(0, 0, screen_width, screen_height); //needed for window resizing
        CenterCursor( Window );
    
        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);
        //Display the value here
        s64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
        f32 deltaTime = (CounterElapsed / (f32)PerfCountFrequency);
        f64 MSPerFrame = (f64) ((1000.0f*CounterElapsed)/ (f64)PerfCountFrequency);
        f64 FPS = PerfCountFrequency/(f64)CounterElapsed;
        LastCounter = EndCounter;

        if( ourOpenGLRC )
        {
            char buf[55];
            sprintf(&buf[0],"FPS Camera Basic: fps %f",FPS);
            SetWindowTextA( WindowHandle, &buf[0]);
        	drawScene(shaderProgram, (1-isPaused)*deltaTime );
    	}
    }break;

    case WM_MOVE:
    {
    	CenterCursor( Window );
    }break;

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)LParam;
        lpMMI->ptMinTrackSize.x = 300;
        lpMMI->ptMinTrackSize.y = 300;
    }break;

    case WM_CLOSE: //when user clicks on the X button on the window
    {
        CloseProgram();
    } break;

    case WM_PAINT: //to allow us to update the window
    {
        drawScene( shaderProgram, 0.0f );
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint); //will fill out struct with information on where to paint
        EndPaint(Window, &Paint); //closes paint area
    } break;


    case WM_ACTIVATE:
    {
    	switch(WParam)
    	{
    		case WA_ACTIVE:
    		case WA_CLICKACTIVE:
    		{

    		} break;
    		case WA_INACTIVE:
    		{
    			Pause();
    		} break;
    		default:
    		{
    			//error
    		}
    	}
    } break;

    default:
        Result = DefWindowProc(Window, Message, WParam, LParam); //call windows to handle default behavior of things we don't handle
    }

    return Result;
}

#if MAIN_DEBUG
int main()
#else
int APIENTRY WinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ LPSTR CommandLine,
    _In_ int ShowCode)
#endif
{
    WNDCLASSEX WindowClass;
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; //https://devblogs.microsoft.com/oldnewthing/20060601-06/?p=31003
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
#if MAIN_DEBUG
    WindowClass.hInstance = GetModuleHandle(NULL);
#else
    WindowClass.hInstance = Instance;
#endif
    WindowClass.hIcon = LoadIcon(0, IDI_APPLICATION); //IDI_APPLICATION: Default application icon, 0 means use a default Icon
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW); //IDC_ARROW: Standard arrow, 0 means used a predefined Cursor
    WindowClass.hbrBackground = NULL; 
    WindowClass.lpszMenuName = NULL;	// No menu 
    WindowClass.lpszClassName = "WindowTestClass"; //name our class
    WindowClass.hIconSm = NULL; //can also do default Icon here? will NULL be default automatically?
    if (RegisterClassEx(&WindowClass))
    {
        WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "FPS Camera Basic",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
            CW_USEDEFAULT, CW_USEDEFAULT, screen_width, screen_height, //if fullscreen get monitor width and height
            0, 0, WindowClass.hInstance, NULL);
 
        if (WindowHandle)
        {        	
        	//  We need to make sure the window create in a suitable DC format
            PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
                PFD_TYPE_RGBA,												// The kind of framebuffer. RGBA or palette.
                32,															// Colordepth of the framebuffer.
                0, 0, 0, 0, 0, 0, 0, 0,
                0,
                0, 0, 0, 0,
                24,															// Number of bits for the depthbuffer
                8,															// Number of bits for the stencilbuffer
                0,															// Number of Aux buffers in the framebuffer.
                PFD_MAIN_PLANE,
                0,
                0, 0, 0
            };

            hDC = GetDC(WindowHandle);				// Get a DC for our window
            int letWindowsChooseThisPixelFormat = ChoosePixelFormat(hDC, &pfd); // Let windows select an appropriate pixel format
            if (letWindowsChooseThisPixelFormat)
            {
                if (SetPixelFormat(hDC, letWindowsChooseThisPixelFormat, &pfd))
                { // Try to set that pixel format
                    ourOpenGLRC = wglCreateContext(hDC);
                    if (ourOpenGLRC)
                    {
                        wglMakeCurrent(hDC, ourOpenGLRC); // Make our render context current
                        loadGLFuncPtrs(); // MUST BE AFTER wgkMakeCurrent! for functions to be loaded for correct context
                        GLuint vShad = LoadShaderFromString(vertexShaderSrc, GL_VERTEX_SHADER, "VERTEX SHADER");
                        GLuint fShad = LoadShaderFromString(fragementShaderSrc, GL_FRAGMENT_SHADER, "FRAGMENT SHADER");
                        shaderProgram = compileShaderProgram(vShad, fShad);

                        InitModelVAOs();

                        glUseProgram(shaderProgram);
    					modelUniformLoc = glGetUniformLocation(shaderProgram, "u_mMat");
    					projViewUniformLoc = glGetUniformLocation(shaderProgram, "u_pvMat");
						lightDirUniformLoc = glGetUniformLocation(shaderProgram, "inv_light_dir");
						ambientUniformLoc = glGetUniformLocation(shaderProgram, "ambient_color");

                        LARGE_INTEGER PerfCountFrequencyResult;
    					QueryPerformanceFrequency(&PerfCountFrequencyResult);
    					PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
                  		QueryPerformanceCounter(&LastCounter);
                        Running = 1;
                        isPaused = 0;
                        isFullscreen = 0;

                        CenterCursor( WindowHandle );

                        RAWINPUTDEVICE Rid[1];
						Rid[0].usUsagePage = ((USHORT) 0x01); 
						Rid[0].usUsage = ((USHORT) 0x02); 
						Rid[0].dwFlags = RIDEV_INPUTSINK;   
						Rid[0].hwndTarget = WindowHandle;
						RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
            			
                    	//https://stackoverflow.com/questions/14134076/winapi-hide-cursor-inside-window-client-area
                    	ShowCursor( FALSE );
						position  = { 0, 0, 0 };

						rotHor = 0;
						rotVert = 0;

                        while (Running)
                        {
							Vec2f frameRot = { 0, 0 };


                            MSG Message;
                            while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) //checks to see if windows wants to do something, returns 0 if no messages are present. PM_REMOVE: remove messages from queue
                            {
                            	//switch to raw input https://docs.microsoft.com/en-us/windows/win32/dxtecharts/taking-advantage-of-high-dpi-mouse-movement?redirectedfrom=MSDN
                            	switch(Message.message)
                            	{
                            		case WM_QUIT:
                            		{
                            			CloseProgram();
                            			break;
                            		}
                            		case WM_SYSKEYDOWN:
                              		case WM_SYSKEYUP:
                              		case WM_KEYDOWN:
                              		case WM_KEYUP:
                              		{
                              			uint32_t VKCode = (uint32_t) Message.wParam;
                                  		bool WasDown = (Message.lParam & (1<<30)) != 0;
                                  		bool IsDown = (Message.lParam & (1<<31)) == 0;
                                  		bool AltKeyWasDown = (Message.lParam & (1 << 29));
                                  		switch(VKCode)
                                  		{
                                  			case VK_UP:
                                  			case 'W':
                                  			{
                                  				if(WasDown != IsDown)
                                  				{
                                  					movingForward = IsDown;
                                  				}
                                  				break;
                                  			}
                                  			case VK_DOWN:
                                  			case 'S':
                                  			{
                                  				if(WasDown != IsDown)
                                  				{
                                  					movingBackwards = IsDown;
                                  				}
                                  				break;
                                  			}
                                  			case VK_LEFT:
                                  			case 'A':
                                  			{
                                  				if(WasDown != IsDown)
                                  				{
                                  					movingLeft = IsDown;
                                  				}
                                  				break;
                                  			}
                                  			case VK_RIGHT:
                                  			case 'D':
                                  			{
                                  				if(WasDown != IsDown)
                                  				{
                                  					movingRight = IsDown;
                                  				}
                                  				break;
                                  			}
                                  			case 'F':
                                  			{
                                  				if(WasDown != 1 && WasDown != IsDown)
                                  				{
                                  					Fullscreen( WindowHandle );
                                  				}
                                  				break;
                                  			}
                                  			case VK_SPACE:
                                  			{
                                  				if(WasDown != IsDown)
                                  				{
                                  					movingUp = IsDown;
                                  				}
                                  				break;
                                  			}
                                  			case VK_SHIFT:
                                    		{
                                    			if(WasDown != IsDown)
                                  				{
                                  					movingDown = IsDown;
                                  				}
                                        		break;
                                    		}
                                  			case VK_ESCAPE:
                                  			{
                                  				if(WasDown != 1 && WasDown != IsDown)
                                  				{
                                  					TogglePause();
                                  				}
                                  				break;
                                  			}
                                  			case VK_F4:
                                  			{
                                  				if(AltKeyWasDown)
                                  				{
                                  					CloseProgram();
                                  				}
                                  				break;
                                  			}
                                  			default:
                                  			{
                                  				break;
                                  			}
                                  		}
                              			break;
                              		}
                              		case WM_INPUT:
                              		{
                              			// https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse
                              			UINT dwSize = sizeof(RAWINPUT);
    									static BYTE lpb[sizeof(RAWINPUT)];

    									GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
									
    									RAWINPUT* raw = (RAWINPUT*)lpb;
									
    									if (raw->header.dwType == RIM_TYPEMOUSE) 
    									{
    									    frameRot.x += raw->data.mouse.lLastX;
    									    frameRot.y += raw->data.mouse.lLastY;
    									}
    									/*
    									else if(raw->header.dwType == RIM_TYPEKEYBOARD )
    									{
    										switch(data.keyboard.VKey)
    										{
    											default:
    											break;
    										}
    									}
    									*/
                              			break;
                              		}
                            		default:
                            		{
                            			TranslateMessage(&Message);
                                		DispatchMessage(&Message);
                                		break;
                            		}
                            	}
                            }

                        	uint64_t EndCycleCount = __rdtsc();
                      
                      		LARGE_INTEGER EndCounter;
                      		QueryPerformanceCounter(&EndCounter);
                  
                     		//Display the value here
                     		s64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                     		f32 deltaTime = (CounterElapsed / (f32)PerfCountFrequency);
                     		f64 MSPerFrame = (f64) ((1000.0f*CounterElapsed)/ (f64)PerfCountFrequency);
                     		f64 FPS = PerfCountFrequency/(f64)CounterElapsed;
                     		LastCounter = EndCounter;
                     		
                     		char buf[55];
                     		sprintf(&buf[0],"FPS Camera Basic: fps %f",FPS);
                     		SetWindowTextA( WindowHandle, &buf[0]);

                            if(!isPaused)
                            {
                     			Vec2f forwardOrientation = { sinf(rotHor*PI_F/180.0f), cosf(rotHor*PI_F/180.0f) };
                     			Vec2f rightOrientation = { sinf((rotHor-90)*PI_F/180.0f), cosf((rotHor-90)*PI_F/180.0f) };
                     			Vec3f positionChange = {0.0f,0.0f,0.0f};
                            	if(movingForward)
                            	{
                            		positionChange.x += forwardOrientation.x;
                                  	positionChange.z -= forwardOrientation.y;
                            	}
                            	if(movingLeft)
                            	{
                            		positionChange.x += rightOrientation.x;
                                  	positionChange.z -= rightOrientation.y;
                            	}
                            	if(movingRight)
                            	{
                                  	positionChange.x -= rightOrientation.x;
                                  	positionChange.z += rightOrientation.y;
                            	}
                            	if(movingBackwards)
                            	{
                            		positionChange.x -= forwardOrientation.x;
                                  	positionChange.z += forwardOrientation.y;
                            	}
                            	if(movingUp)
                            	{
                            		positionChange.y += 1;
                            	}
                            	if(movingDown)
                            	{
                            		positionChange.y -= 1;
                            	}

                            	Vec3f oldPos;
                                oldPos.x = position.x;
                                oldPos.y = position.y;
                                oldPos.z = position.z;
                                Vec3f val;
                                //normalize positionChange maybe?
                                Vec3fScale(&positionChange,speed*deltaTime,&val);
                            	Vec3fAdd( &oldPos, &val, &position );

                                rotHor  = fmodf(rotHor  + deltaTime*mouseSensitivity*frameRot.x, 360.0f );
                                rotVert = clamp(rotVert + deltaTime*mouseSensitivity*frameRot.y, -90.0f, 90.0f);
                            }

                            drawScene(shaderProgram, (1-isPaused)*deltaTime );
                            if( isPaused )
                            {
                            	//draw pause UI
                            }

                        }
                        wglMakeCurrent(NULL, NULL);
                        wglDeleteContext(ourOpenGLRC);
                    }
                }
                else
                    return logWindowsError("Failed to Set Pixel Format for Window:\n");
            }
            else
                return logWindowsError("Windows Failed to choose Pixel Format:\n");
            ReleaseDC(WindowHandle, hDC); //need to close before logError if doing symmetric programming
            DestroyWindow(WindowHandle);
        }
        else
            return logWindowsError("Failed to Instantiate Window Class:\n");
    }
    else
        return logWindowsError("Failed to Register Window Class:\n");
    return 0;
}