/*
CPE/CSC 474 Lab base code Eckhardt/Dahl
based on CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#include "SmartTexture.h"
#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"
#include "Shape.h"
#include "skmesh.h"
#include "line.h"
#include "stb_image.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// assimp
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/vector3.h"
#include "assimp/scene.h"
#include <assimp/mesh.h>


using namespace std;
using namespace glm;
using namespace Assimp;

shared_ptr<Shape> plane;
shared_ptr<Shape> diamond;
vector<vec3> splinepoints1;
Line linerender1;
Line smoothrender1;
vector<vec3> line1;
vector<vec3> splinepointscam;
vector<vec3> linecam;
int renderstate = 1;

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}



class camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
//		pos = glm::vec3(0, 0, 0);
        pos = glm::vec3(0,15,-10);
		rot = glm::vec3(0, 0, 0);
	}
	
	glm::mat4 process(double ftime)
	{
		double speed = 0.0f;
		
		if (w == 1)
		{
			speed = 10*ftime;
		}
		else if (s == 1)
		{
			speed = -10*ftime;
		}
		double yangle=0;
		if (a == 1)
			yangle = -3*ftime;
		else if(d==1)
			yangle = 3*ftime;
		rot.y += (float)yangle;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::vec4 dir = glm::vec4(0, 0, speed, 1);
		dir = dir*R;
		pos += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R*T;
	}
};

camera mycam;

class Application : public EventCallbacks
{
public:
	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> psky, pplane1,skinProg, copProg, robberProg, diamondProg;

	// skinnedMesh
	SkinnedMesh skmesh, copMesh, robberMesh, diamondMesh;
	// textures
	shared_ptr<SmartTexture> skyTex;
	
	// shapes
	shared_ptr<Shape> skyShape;
    
    GLuint Texture2;
	
	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		else if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		else if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			//mycam.pos = vec3(mycam.pos.x, mycam.pos.y-0.1, mycam.pos.z);
			skmesh.print_animations(0);
		}
        if (key == GLFW_KEY_P && action == GLFW_RELEASE)
        {
            if (smoothrender1.is_active())
                smoothrender1.reset();
            else
                {
                spline(splinepoints1, line1, 10, 1.0);
                    spline(splinepointscam, linecam, 10, 1.0);
                smoothrender1.re_init_line(splinepoints1);
                    gtime = 0.0;
                    runningplane =1;
                }
        }

		if (key == GLFW_KEY_1 && action == GLFW_RELEASE)
			{
			skmesh.SetNextAnimation(0);
			}
		if (key == GLFW_KEY_2 && action == GLFW_RELEASE)
			{
			skmesh.SetNextAnimation(1);
			}
		if (key == GLFW_KEY_3 && action == GLFW_RELEASE)
			{
			skmesh.SetNextAnimation(2);
			}
		if (key == GLFW_KEY_4 && action == GLFW_RELEASE)
			{
			skmesh.SetNextAnimation(3);
			}
	}
    
    
    class orientation {
    public:
        vec3 ez, ey;
        orientation(){}
        orientation(vec3 eZ, vec3 eY){
            ez = eZ;
            ey = eY;
        }
    };

    vector<orientation> opoints;
    int runningplane = 0;

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{

	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}
	/*Note that any gl calls must always happen after a GL state is initialized */
	
	
	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom(const std::string& resourceDirectory)
	{


		if (!skmesh.LoadMesh(resourceDirectory + "/Back.fbx")) {
			printf("Mesh load failed\n");
			return;
			}
        
        if (!robberMesh.LoadMesh(resourceDirectory + "/PainR.fbx")) {
            printf("Mesh load failed\n");
            return;
            }
        
        if (!copMesh.LoadMesh(resourceDirectory + "/No.fbx")) {
            printf("Mesh load failed\n");
            return;
            }
        if (!diamondMesh.LoadMesh(resourceDirectory + "/diamond.fbx")) {
            printf("Mesh load failed\n");
            return;
            }
	
	/*	if (!skmesh.LoadMesh(resourceDirectory + "/boblampclean.md5mesh")) {
 
			printf("Mesh load failed\n");
			return;
		}*/

	
		// Initialize mesh.
		skyShape = make_shared<Shape>();
		skyShape->loadMesh(resourceDirectory + "/sphere.obj");
		skyShape->resize();
		skyShape->init();
        
        plane = make_shared<Shape>();
        string mtldir = resourceDirectory;
        plane->loadMesh(resourceDirectory + "/FA18.obj", &mtldir, stbi_load);
        plane->resize();
        
//        diamond = make_shared<Shape>();
//        mtldir = resourceDirectory;
//        diamond->loadMesh(resourceDirectory + "/diamond.obj", &mtldir, stbi_load);
//        diamond->resize();
        
		// sky texture
		auto strSky = resourceDirectory + "/dresden_square_4k.hdr";
//        auto strSky = resourceDirectory + "/sky.jpg";
		skyTex = SmartTexture::loadTexture(strSky, true);
		if (!skyTex)
			cerr << "error: texture " << strSky << " not found" << endl;
        
        smoothrender1.init();
        linerender1.init();
        
        line1.push_back(vec3(0,0,-3));
        line1.push_back(vec3(-5,0,-23.0848));
        line1.push_back(vec3(5,0,-35.2221));
        line1.push_back(vec3(-5,0,-35.8872));
        line1.push_back(vec3(5,0,-33.4712));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(-5,0,-43.6945));
        line1.push_back(vec3(5,0,-56.0547));
        line1.push_back(vec3(-5,0,-76.4492));
        line1.push_back(vec3(5,0,-76.4492));
        line1.push_back(vec3(-5,0,-99.9773));
        line1.push_back(vec3(5,0,-120.187));
        line1.push_back(vec3(-5,0,-140.747));
        line1.push_back(vec3(5,0,-153.942));
        line1.push_back(vec3(-5,0,-168.747));
        line1.push_back(vec3(5,0,-191.894));
        line1.push_back(vec3(-5,0,-191.894));
        
        
        linecam.push_back(vec3(0,0,-3));
//        linecam.push_back(vec3(0,0,-3));
//        linecam.push_back(vec3(0,0,-23.0848));
//        linecam.push_back(vec3(0,0,-35.2221));
//        linecam.push_back(vec3(0,0,-35.8872));
//        linecam.push_back(vec3(0,0,-33.4712));
//        linecam.push_back(vec3(0,0,-43.6945));
//        linecam.push_back(vec3(0,0,-56.0547));
//        linecam.push_back(vec3(0,0,-76.4492));
//        linecam.push_back(vec3(0,0,-76.4492));
//        linecam.push_back(vec3(0,0,-99.9773));
//        line1.push_back(vec3(0,0,-120.187));
//        line1.push_back(vec3(0,0,-140.747));
//        line1.push_back(vec3(0,0,-153.942));
//        line1.push_back(vec3(0,0,-168.747));
//        line1.push_back(vec3(0,0,-191.894));
        
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        linecam.push_back(vec3(0,15,-3));
        
//        linecam.push_back(vec3(0,0,17.0848));
//        linecam.push_back(vec3(0,0,29.2221));
//        linecam.push_back(vec3(0,0,29.8872));
//        linecam.push_back(vec3(0,0,27.4712));
//        linecam.push_back(vec3(0,0,37.6945));
//        linecam.push_back(vec3(0,0,50.0547));
//        linecam.push_back(vec3(0,0,70.4492));
//        linecam.push_back(vec3(0,0,70.4492));
//        linecam.push_back(vec3(0,0,93.9773));
//        linecam.push_back(vec3(0,0,114.187));
//        linecam.push_back(vec3(0,0,134.747));
//        linecam.push_back(vec3(0,0,147.942));
//        linecam.push_back(vec3(0,0,162.747));
//        linecam.push_back(vec3(0,0,185.894));
//        linecam.push_back(vec3(0,0,188.894));
        
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        opoints.push_back(orientation(vec3(0,0,1), vec3(0,1,0)));
        linerender1.re_init_line(line1);
	}
    
    mat4 linint_between_two_orientations(vec3 ez_aka_lookto_1, vec3 ey_aka_up_1, vec3 ez_aka_lookto_2, vec3 ey_aka_up_2, float t)
        {
        mat4 m1, m2;
        quat q1, q2;
        vec3 ex, ey, ez;
        ey = ey_aka_up_1;
        ez = ez_aka_lookto_1;
        ex = cross(ey, ez);
        m1[0][0] = ex.x;        m1[0][1] = ex.y;        m1[0][2] = ex.z;        m1[0][3] = 0;
        m1[1][0] = ey.x;        m1[1][1] = ey.y;        m1[1][2] = ey.z;        m1[1][3] = 0;
        m1[2][0] = ez.x;        m1[2][1] = ez.y;        m1[2][2] = ez.z;        m1[2][3] = 0;
        m1[3][0] = 0;            m1[3][1] = 0;            m1[3][2] = 0;            m1[3][3] = 1.0f;
        ey = ey_aka_up_2;
        ez = ez_aka_lookto_2;
        ex = cross(ey, ez);
        m2[0][0] = ex.x;        m2[0][1] = ex.y;        m2[0][2] = ex.z;        m2[0][3] = 0;
        m2[1][0] = ey.x;        m2[1][1] = ey.y;        m2[1][2] = ey.z;        m2[1][3] = 0;
        m2[2][0] = ez.x;        m2[2][1] = ez.y;        m2[2][2] = ez.z;        m2[2][3] = 0;
        m2[3][0] = 0;            m2[3][1] = 0;            m2[3][2] = 0;            m2[3][3] = 1.0f;
        q1 = quat(m1);
        q2 = quat(m2);
        quat qt = slerp(q1, q2, t); //<---
        qt = normalize(qt);
        mat4 mt = mat4(qt);
        //mt = transpose(mt);         //<---
        return mt;
        }


	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_DEPTH_TEST);
		// Initialize the GLSL program.
		psky = std::make_shared<Program>();
		psky->setVerbose(true);
		psky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!psky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		
		psky->addUniform("P");
		psky->addUniform("V");
		psky->addUniform("M");
		psky->addUniform("tex");
		psky->addUniform("camPos");
		psky->addAttribute("vertPos");
		psky->addAttribute("vertNor");
		psky->addAttribute("vertTex");

		skinProg = std::make_shared<Program>();
		skinProg->setVerbose(true);
		skinProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
		if (!skinProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		
		skinProg->addUniform("P");
		skinProg->addUniform("V");
		skinProg->addUniform("M");
		skinProg->addUniform("tex");
		skinProg->addUniform("camPos");
		skinProg->addAttribute("vertPos");
		skinProg->addAttribute("vertNor");
		skinProg->addAttribute("vertTex");
		skinProg->addAttribute("BoneIDs");
		skinProg->addAttribute("Weights");
        
        copProg = std::make_shared<Program>();
        copProg->setVerbose(true);
        copProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
        if (!copProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        
        copProg->addUniform("P");
        copProg->addUniform("V");
        copProg->addUniform("M");
        copProg->addUniform("tex");
        copProg->addUniform("camPos");
        copProg->addAttribute("vertPos");
        copProg->addAttribute("vertNor");
        copProg->addAttribute("vertTex");
        copProg->addAttribute("BoneIDs");
        copProg->addAttribute("Weights");
        
        
        robberProg = std::make_shared<Program>();
        robberProg->setVerbose(true);
        robberProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
        if (!robberProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        
        robberProg->addUniform("P");
        robberProg->addUniform("V");
        robberProg->addUniform("M");
        robberProg->addUniform("tex");
        robberProg->addUniform("camPos");
        robberProg->addAttribute("vertPos");
        robberProg->addAttribute("vertNor");
        robberProg->addAttribute("vertTex");
        robberProg->addAttribute("BoneIDs");
        robberProg->addAttribute("Weights");
        
        diamondProg = std::make_shared<Program>();
        diamondProg->setVerbose(true);
        diamondProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
        if (!diamondProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        
        diamondProg->addUniform("P");
        diamondProg->addUniform("V");
        diamondProg->addUniform("M");
        diamondProg->addUniform("tex");
        diamondProg->addUniform("camPos");
        diamondProg->addAttribute("vertPos");
        diamondProg->addAttribute("vertNor");
        diamondProg->addAttribute("vertTex");
        diamondProg->addAttribute("BoneIDs");
        diamondProg->addAttribute("Weights");
        
        
        pplane1 = std::make_shared<Program>();
        pplane1->setVerbose(false);
        pplane1->setShaderNames(resourceDirectory + "/plane_vertex.glsl", resourceDirectory + "/plane_frag.glsl");
        if (!pplane1->init())
            {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
            }
        pplane1->addUniform("P");
        pplane1->addUniform("V");
        pplane1->addUniform("M");
        pplane1->addUniform("campos");
        pplane1->addAttribute("vertPos");
        pplane1->addAttribute("vertNor");
        pplane1->addAttribute("vertTex");
	}


	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
    double gtime = 0;
    vec3 pos1, pos2;
	void render()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		double frametime = get_last_elapsed_time();
		static double totaltime = 0;
		totaltime += frametime;
        gtime += frametime;

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now
		
		glm::mat4 V, M, P; //View, Model and Perspective matrix
        double realtime = gtime * 5.;
        int itime = realtime;
        if(runningplane) {
            float t = realtime - itime;
            
            vec3 a = splinepointscam[itime];
            vec3 b = splinepointscam[itime + 1];
//            mycam.pos = a*(1.0f -t) + b*t;
//            mycam.pos = vec3(0,15,-10);
            
            //TransPlane = translate(mat4(1), pos);
            
            double realtime_o = realtime/(splinepointscam.size()/linecam.size());
            int itime_o = realtime_o;
            float t_o = realtime_o - itime_o;
            
            orientation ao = opoints[itime_o];
            orientation bo = opoints[itime_o +1];
        }
		V = mycam.process(frametime);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);		
		if (width < height)
			{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect,  1.0f / aspect, -2.0f, 100.0f);
			}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		auto sangle = -3.1415926f / 2.0f;
		glm::mat4 RotateXSky = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 camp = -mycam.pos;
		glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
		glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));

		M = TransSky  * RotateXSky * SSky;

		// Draw the sky using GLSL.
		psky->bind();
		GLuint texLoc = glGetUniformLocation(psky->pid, "tex");
		skyTex->bind(texLoc);	
		glUniformMatrix4fv(psky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(psky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(psky->getUniform("camPos"), 1, &mycam.pos[0]);

		glDisable(GL_DEPTH_TEST);
		skyShape->draw(psky, false);
		glEnable(GL_DEPTH_TEST);	
		skyTex->unbind();
		psky->unbind();
		
        
//        glm::mat4 TransPlane = glm::translate(glm::mat4(1.0f), vec3(0,0,-3));
//        glm::mat4 SPlane = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
//        sangle = -3.1415926 / 2.;
//        glm::mat4 RotateXPlane = glm::rotate(glm::mat4(1.0f), sangle, vec3(1,0,0));
//
//        if(itime +1 >= splinepoints1.size())
//            runningplane = 0;
//        mat4 Ro = mat4(1);
//        if(runningplane) {
//            float t = realtime - itime;
//
//            vec3 a = splinepoints1[itime];
//            vec3 b = splinepoints1[itime + 1];
//            pos1 = a*(1.0f -t) + b*t;
//
//            TransPlane = translate(mat4(1), pos1);
//
//            double realtime_o = realtime/(splinepoints1.size()/line1.size());
//            int itime_o = realtime_o;
//            float t_o = realtime_o - itime_o;
//
//            orientation ao = opoints[itime_o];
//            orientation bo = opoints[itime_o +1];
//
//            Ro = linint_between_two_orientations(ao.ez, ao.ey, bo.ez, bo.ey, t_o);
//        }
//
//        M = TransPlane*Ro*RotateXPlane*SPlane;

//        pplane1->bind();
//        glUniformMatrix4fv(pplane1->getUniform("P"), 1, GL_FALSE, &P[0][0]);
//        glUniformMatrix4fv(pplane1->getUniform("V"), 1, GL_FALSE, &V[0][0]);
//        glUniformMatrix4fv(pplane1->getUniform("M"), 1, GL_FALSE, &M[0][0]);
//        glUniform3fv(pplane1->getUniform("campos"), 1, &mycam.pos[0]);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, Texture2);
//        plane->draw(pplane1,false);            //render!!!!!!!
//        pplane1->unbind();
        
        
        glm::mat4 Trans = glm::translate(glm::mat4(1.0f), vec3(-0.2, -15, -30));
        sangle = 3.1415926/2. ;
        glm::mat4 RotX = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
        glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(10.3f, 10.3f, 10.3f));
        
        
        M = Trans  * RotX * Scale;
        
        
        // draw the skinned mesh
        if(runningplane){
            skinProg->bind();
            texLoc = glGetUniformLocation(skinProg->pid, "tex");
            sangle = 0*-3.1415926f / 2.0f;
            double height = gtime;
            if(height > 3)
                height = 3;
            Trans = glm::translate(glm::mat4(1.0f), vec3(-6.0, -17, -3));
            RotX = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
            Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));


            M = Trans * RotX* Scale;

            glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
            glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
            glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
            glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            skmesh.setBoneTransformations(skinProg->pid, totaltime*0.8);
//            skmesh.Render(texLoc);
            skinProg->unbind();

            copProg->bind();
            texLoc = glGetUniformLocation(copProg->pid, "tex");
            sangle = -3.1415926f / 2.0f;
            Trans = glm::translate(glm::mat4(1.0f), vec3(-0.5, -17, -3));
            RotX = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
            Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));


            M = Trans  * RotX* Scale;

            glUniform3fv(copProg->getUniform("camPos"), 1, &mycam.pos[0]);
            glUniformMatrix4fv(copProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
            glUniformMatrix4fv(copProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
            glUniformMatrix4fv(copProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            copMesh.setBoneTransformations(copProg->pid, totaltime*0.8);
            copMesh.Render(texLoc);
            copProg->unbind();

            robberProg->bind();
            texLoc = glGetUniformLocation(robberProg->pid, "tex");
            sangle = -3.1415926f / 1.0f;
            Trans = glm::translate(glm::mat4(1.0f), vec3(-0.5, -17, -4));
            RotX = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
//            Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));


            M = Trans * RotX * Scale;

            glUniform3fv(robberProg->getUniform("camPos"), 1, &mycam.pos[0]);
            glUniformMatrix4fv(robberProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
            glUniformMatrix4fv(robberProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
            glUniformMatrix4fv(robberProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            robberMesh.setBoneTransformations(robberProg->pid, totaltime*0.8);
            robberMesh.Render(texLoc);
            robberProg->unbind();
            
            diamondProg->bind();
            texLoc = glGetUniformLocation(diamondProg->pid, "tex");
            sangle = -3.1415926f / 1.0f;
            Trans = glm::translate(glm::mat4(1.0f), vec3(-0.5, -17, -4));
            RotX = glm::rotate(glm::mat4(1.0f), sangle, vec3(0, 1, 0));
            Scale = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f));


            M = Trans * RotX * Scale;

            glUniform3fv(diamondProg->getUniform("camPos"), 1, &mycam.pos[0]);
            glUniformMatrix4fv(diamondProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
            glUniformMatrix4fv(diamondProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
            glUniformMatrix4fv(diamondProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            diamondMesh.setBoneTransformations(diamondProg->pid, totaltime*0.8);
            diamondMesh.Render(texLoc);
            diamondProg->unbind();
        }
	}
};

//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../../resources"; // Where the resources are loaded from
	std::string missingTexture = "missing.jpg";
	
	SkinnedMesh::setResourceDir(resourceDir);
	SkinnedMesh::setDefaultTexture(missingTexture);
	
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

