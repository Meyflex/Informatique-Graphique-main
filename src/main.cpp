//SDL Libraries
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

//OpenGL Libraries
#include <GL/glew.h>
#include <GL/gl.h>

//GML libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL_image.h>
#include <cstdint>
#include <vector>
#include <stack>

#include "Shader.h"
#include "logger.h"

#include "Sphere.h"

#define WIDTH     1600
#define HEIGHT    900
#define FRAMERATE 60
#define TIME_PER_FRAME_MS  (1.0f/FRAMERATE * 1e3)
#define INDICE_TO_PTR(x) ((void*)(x))

struct Material {
    glm::vec3 color;
    float ka;
    float kd;
    float ks;
    float alpha;
    GLuint texture = 0;
};
struct objet {
    GLuint vboID = 0;
    Geometry* geometry = nullptr;
    glm::mat4 localMatrix = glm::mat4(1.0f);
    glm::mat4 propagatedMatrix = glm::mat4(1.0f);
    std::vector<objet*> children;
    Material material;
    int etoile = 0;
};



struct Light {
    glm::vec3 position;
    glm::vec3 color;
};

void draw(objet& go, Shader* shader, std::stack<glm::mat4>& matrices, glm::vec3& cameraPosition, glm::mat4& view, glm::mat4& projection, glm::vec3 lightposition[]) {
    glm::mat4 model = matrices.top() * go.localMatrix;
    glm::mat4 mvp = projection * view * model;
    glUseProgram(shader->getProgramID());
    {
        Material sphereMtl;
        sphereMtl = go.material;
        Light light;
        if (go.etoile == 1) {
            light = { lightposition[0], {1.0f, 1.0f, 1.0f} };
        }
        else {
            light = { lightposition[1], {1.0f, 1.0f, 1.0f} };
        }
        glBindBuffer(GL_ARRAY_BUFFER, go.vboID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, go.material.texture);
        GLint vPosition = glGetAttribLocation(shader->getProgramID(), "vPosition");
        glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vPosition);
        GLint vNormal = glGetAttribLocation(shader->getProgramID(), "vNormal");
        glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, INDICE_TO_PTR(go.geometry->getNbVertices() * 3 * sizeof(float)));
        glEnableVertexAttribArray(vNormal);
        GLint UV = glGetAttribLocation(shader->getProgramID(), "Vuv");
        glVertexAttribPointer(UV, 2, GL_FLOAT, GL_FALSE, 0, INDICE_TO_PTR(go.geometry->getNbVertices() * 6 * sizeof(float)));
        glEnableVertexAttribArray(UV);
        GLint uTexture = glGetAttribLocation(shader->getProgramID(), "uTexture");
        glVertexAttribPointer(uTexture, 2, GL_FLOAT, GL_FALSE, 0, INDICE_TO_PTR(go.geometry->getNbVertices() * 6 * sizeof(float)));
        glEnableVertexAttribArray(uTexture);
        GLint uMVP = glGetUniformLocation(shader->getProgramID(), "uMVP");
        GLint uModel = glGetUniformLocation(shader->getProgramID(), "uModel");
        GLint uInvModel3x3 = glGetUniformLocation(shader->getProgramID(), "uInvModel3x3");
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(uInvModel3x3, 1, GL_FALSE, glm::value_ptr(glm::mat3(glm::inverse(model))));
        GLint uMtlColor = glGetUniformLocation(shader->getProgramID(), "uMtlColor");
        GLint uMtlCts = glGetUniformLocation(shader->getProgramID(), "uMtlCts");
        GLint uLightPos = glGetUniformLocation(shader->getProgramID(), "uLightPos");
        GLint uLightColor = glGetUniformLocation(shader->getProgramID(), "uLightColor");
        GLint uCameraPosition = glGetUniformLocation(shader->getProgramID(), "uCameraPosition");
        glUniform3fv(uMtlColor, 1, glm::value_ptr(sphereMtl.color));
        glUniform4f(uMtlCts, sphereMtl.ka, sphereMtl.kd, sphereMtl.ks, sphereMtl.alpha);
        glUniform3fv(uLightPos, 1, glm::value_ptr(light.position));
        glUniform3fv(uLightColor, 1, glm::value_ptr(light.color));
        glUniform3fv(uCameraPosition, 1, glm::value_ptr(cameraPosition));


        glDrawArrays(GL_TRIANGLES, 0, go.geometry->getNbVertices());

    }
    glUseProgram(0);
    matrices.push(matrices.top() * go.propagatedMatrix);
    for (int i = 0; i < go.children.size(); i++) {
        draw(*(go.children[i]), shader, matrices, cameraPosition, view, projection, lightposition);
    }
    matrices.pop();


}

int main(int argc, char* argv[])
{
    ////////////////////////////////////////
    //SDL2 / OpenGL Context initialization : 
    ////////////////////////////////////////

    //Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        ERROR("The initialization of the SDL failed : %s\n", SDL_GetError());
        return 0;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        ERROR("The initialization of the SDL_image failed : %s\n", IMG_GetError());
        return EXIT_FAILURE;
    }



    //Create a Window
    SDL_Window* window = SDL_CreateWindow("Diving Throught Space",                           //Titre
        SDL_WINDOWPOS_UNDEFINED,               //X Position
        SDL_WINDOWPOS_UNDEFINED,               //Y Position
        WIDTH, HEIGHT,                         //Resolution
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN); //Flags (OpenGL + Show)

//Initialize OpenGL Version (version 3.0)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    //Initialize the OpenGL Context (where OpenGL resources (Graphics card resources) lives)
    SDL_GLContext context = SDL_GL_CreateContext(window);

    //Tells GLEW to initialize the OpenGL function with this version
    glewExperimental = GL_TRUE;
    glewInit();


    //Start using OpenGL to draw something on screen
    glViewport(0, 0, WIDTH, HEIGHT); //Draw on ALL the screen

    //The OpenGL background color (RGBA, each component between 0.0f and 1.0f)
    glClearColor(0.0, 0.0, 0.0, 1.0); //Full Black  

    glEnable(GL_DEPTH_TEST); //Active the depth test





    SDL_Surface* imgSun = IMG_Load("Assets/8k_sun.jpg");

    SDL_Surface* imgMercury = IMG_Load("Assets/8k_mercury.jpg");
    SDL_Surface* imgSertAr = IMG_Load("Assets/elcockpito.png");
    SDL_Surface* ImgVenus = IMG_Load("Assets/8k_venus_surface.jpg");
    SDL_Surface* ImgTerre = IMG_Load("Assets/myP.png");
    SDL_Surface* ImgMars = IMG_Load("Assets/8k_mars.jpg");
    SDL_Surface* ImgJupiter = IMG_Load("Assets/8k_jupiter.jpg");
    SDL_Surface* ImgSaturne = IMG_Load("Assets/8k_saturn.jpg");
    SDL_Surface* ImgUranus = IMG_Load("Assets/2k_uranus.jpg");
    SDL_Surface* ImgNeptune = IMG_Load("Assets/2k_neptune.jpg");
    SDL_Surface* ImgMoon = IMG_Load("Assets/8k_moon.jpg");
    SDL_Surface* ImgStar = IMG_Load("Assets/8k_stars.jpg");

    //sci fi part 
    SDL_Surface* Imgpandora = IMG_Load("Assets/pandora.jpg");
    SDL_Surface* Imgcoruscant = IMG_Load("Assets/coruscant.jpeg");
    SDL_Surface* Imgdiana = IMG_Load("Assets/diana.jpg");
    SDL_Surface* Imganubis = IMG_Load("Assets/anubis.jpg");

    SDL_Surface* Imgloki = IMG_Load("Assets/loki.png");
    SDL_Surface* ImgdeathStar = IMG_Load("Assets/death-star.png");
    SDL_Surface* Imgnaruto = IMG_Load("Assets/naruto.jpg");
    SDL_Surface* Imgisis = IMG_Load("Assets/isis.jpg");



    SDL_Surface* rgbImgSun = SDL_ConvertSurfaceFormat(imgSun, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgMercure = SDL_ConvertSurfaceFormat(imgMercury, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImg2 = SDL_ConvertSurfaceFormat(imgSertAr, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgVenus = SDL_ConvertSurfaceFormat(ImgVenus, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgTerre = SDL_ConvertSurfaceFormat(ImgTerre, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgMars = SDL_ConvertSurfaceFormat(ImgMars, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgJupiter = SDL_ConvertSurfaceFormat(ImgJupiter, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgSaturne = SDL_ConvertSurfaceFormat(ImgSaturne, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgUranus = SDL_ConvertSurfaceFormat(ImgUranus, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgNeptune = SDL_ConvertSurfaceFormat(ImgNeptune, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgMoon = SDL_ConvertSurfaceFormat(ImgMoon, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgSTAR = SDL_ConvertSurfaceFormat(ImgStar, SDL_PIXELFORMAT_RGBA32, 0);

    //scifi part 
    SDL_Surface* rgbImgpandora = SDL_ConvertSurfaceFormat(Imgpandora, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgcoruscant = SDL_ConvertSurfaceFormat(Imgcoruscant, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgdiana = SDL_ConvertSurfaceFormat(Imgdiana, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImganubis = SDL_ConvertSurfaceFormat(Imganubis, SDL_PIXELFORMAT_RGBA32, 0);

    SDL_Surface* rgbloki = SDL_ConvertSurfaceFormat(Imgloki, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgdeathStar = SDL_ConvertSurfaceFormat(ImgdeathStar, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgnaruto = SDL_ConvertSurfaceFormat(Imgnaruto, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_Surface* rgbImgisis = SDL_ConvertSurfaceFormat(Imgisis, SDL_PIXELFORMAT_RGBA32, 0);



    SDL_FreeSurface(imgSun);
    SDL_FreeSurface(imgSertAr);
    SDL_FreeSurface(imgMercury);
    SDL_FreeSurface(ImgVenus);
    SDL_FreeSurface(ImgTerre);
    SDL_FreeSurface(ImgMars);
    SDL_FreeSurface(ImgJupiter);
    SDL_FreeSurface(ImgSaturne);
    SDL_FreeSurface(ImgUranus);
    SDL_FreeSurface(ImgNeptune);
    SDL_FreeSurface(ImgMoon);
    SDL_FreeSurface(ImgStar);

    //scifi part 
    SDL_FreeSurface(Imgpandora);
    SDL_FreeSurface(Imgcoruscant);
    SDL_FreeSurface(Imgdiana);
    SDL_FreeSurface(Imganubis);

    SDL_FreeSurface(Imgloki);
    SDL_FreeSurface(ImgdeathStar);
    SDL_FreeSurface(Imgnaruto);
    SDL_FreeSurface(Imgisis);

    //sicfi part 
    GLuint texturepandora;
    glGenTextures(1, &texturepandora);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, texturepandora);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgpandora->w, rgbImgSun->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgpandora->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgpandora);


    GLuint texturecoruscant;
    glGenTextures(1, &texturecoruscant);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, texturecoruscant);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgcoruscant->w, rgbImgcoruscant->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgcoruscant->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgcoruscant);



    GLuint texturediana;
    glGenTextures(1, &texturediana);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, texturediana);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgdiana->w, rgbImgdiana->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgdiana->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgdiana);


    GLuint textureanubis;
    glGenTextures(1, &textureanubis);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, textureanubis);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImganubis->w, rgbImganubis->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImganubis->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImganubis);


    GLuint textureloki;
    glGenTextures(1, &textureloki);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, textureloki);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbloki->w, rgbloki->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbloki->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbloki);


    GLuint texturedeathStar;
    glGenTextures(1, &texturedeathStar);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, texturedeathStar);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgdeathStar->w, rgbImgdeathStar->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgdeathStar->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgdeathStar);


    GLuint texturenaruto;
    glGenTextures(1, &texturenaruto);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, texturenaruto);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgnaruto->w, rgbImgnaruto->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgnaruto->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgnaruto);


    GLuint textureisis;
    glGenTextures(1, &textureisis);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, textureisis);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgisis->w, rgbImgisis->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgisis->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgisis);







    GLuint textureSun;
    glGenTextures(1, &textureSun);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, textureSun);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgSun->w, rgbImgSun->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgSun->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgSun);

    GLuint textureSTAR;
    glGenTextures(1, &textureSTAR);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, textureSTAR);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgSTAR->w, rgbImgSTAR->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgSTAR->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgSTAR);


    GLuint TextureMercure;
    glGenTextures(1, &TextureMercure);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, TextureMercure);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgMercure->w, rgbImgMercure->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgMercure->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgMercure);

    GLuint TextureVenus;
    glGenTextures(1, &TextureVenus);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, TextureVenus);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgVenus->w, rgbImgVenus->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgVenus->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgVenus);



    GLuint TextureTerre;
    glGenTextures(1, &TextureTerre);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, TextureTerre);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgTerre->w, rgbImgTerre->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgTerre->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgTerre);



    GLuint TextureMars;
    glGenTextures(1, &TextureMars);
    std::cout << glGetError() << std::endl;
    glBindTexture(GL_TEXTURE_2D, TextureMars);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgMars->w, rgbImgMars->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgMars->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgMars);

    GLuint TextureJupiter;
    glGenTextures(1, &TextureJupiter);
    std::cout << glGetError() << std::endl;

    glBindTexture(GL_TEXTURE_2D, TextureJupiter);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgJupiter->w, rgbImgJupiter->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgJupiter->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgJupiter);

    GLuint TextureSaturne;
    glGenTextures(1, &TextureSaturne);
    std::cout << glGetError() << std::endl;

    glBindTexture(GL_TEXTURE_2D, TextureSaturne);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgSaturne->w, rgbImgSaturne->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgSaturne->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgSaturne);

    GLuint TextureUranus;
    glGenTextures(1, &TextureUranus);
    std::cout << glGetError() << std::endl;

    glBindTexture(GL_TEXTURE_2D, TextureUranus);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgUranus->w, rgbImgUranus->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgUranus->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgUranus);

    GLuint TextureNeptune;
    glGenTextures(1, &TextureNeptune);
    std::cout << glGetError() << std::endl;

    glBindTexture(GL_TEXTURE_2D, TextureNeptune);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgNeptune->w, rgbImgNeptune->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgNeptune->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgNeptune);


    GLuint TextureMoon;
    glGenTextures(1, &TextureMoon);
    std::cout << glGetError() << std::endl;

    glBindTexture(GL_TEXTURE_2D, TextureMoon);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgbImgMoon->w, rgbImgMoon->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)rgbImgMoon->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    SDL_FreeSurface(rgbImgMoon);


    Sphere sphere(32, 32);





    GLuint vboSphereID;
    glGenBuffers(1, &vboSphereID);
    glBindBuffer(GL_ARRAY_BUFFER, vboSphereID);
    glBufferData(GL_ARRAY_BUFFER, sphere.getNbVertices() * (3 + 3 + 2) * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere.getNbVertices() * 3 * sizeof(float), sphere.getVertices());

    glBufferSubData(GL_ARRAY_BUFFER, sphere.getNbVertices() * 3 * sizeof(float) + sphere.getNbVertices() * 3 * sizeof(float), sphere.getNbVertices() * 2 * sizeof(float), sphere.getUVs());
    glBufferSubData(GL_ARRAY_BUFFER, sphere.getNbVertices() * 3 * sizeof(float), sphere.getNbVertices() * 3 * sizeof(float), sphere.getNormals());
    glBindBuffer(GL_ARRAY_BUFFER, 0);



    objet sunGO;
    objet sunDeux;
    objet etoileGO;
    objet etoileinvi;
    objet MercureGO;
    objet VenusGO;
    objet EarthGO;
    objet MarsGO;
    objet JupiterGO;
    objet SaturneGO;
    objet UranusGO;
    objet NeptuneGO;
    objet MoonGO;

    objet pandoraGO;
    objet coruscantGO;
    objet dianaGO;
    objet anubisGO;
    objet lokiGO;
    objet deathStarGO;
    objet narutoGO;
    objet isisGO;




    Material sphereMtl{ { 0.0f, 0.0f, 0.0f }, 1.0f, 0.0f, 0.0f, 1 };
    sunGO.material = Material{ {1.0f, 1.0f, 1.0f}, 1.0f, 0.0f, 0.0f, 1 ,textureSun };
    sunDeux.material = Material{ {1.0f, 1.0f, 1.0f}, 1.0f, 0.0f, 0.0f, 1 ,texturedeathStar };
    etoileinvi.material = Material{ {1.0f, 1.0f, 1.0f}, 1.0f, 0.0f, 0.0f, 1 ,textureSun };
    MercureGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.0f, 1 ,TextureMercure };
    MercureGO.etoile = 2;
    VenusGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.0f, 1 ,TextureVenus };
    VenusGO.etoile = 2;
    EarthGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.9f, 0.0f, 1 ,TextureTerre };
    EarthGO.etoile = 2;
    MarsGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.0f, 1 ,TextureMars };
    MarsGO.etoile = 2;
    JupiterGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.4f, 1 ,TextureJupiter };
    JupiterGO.etoile = 2;
    SaturneGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.3f, 1 ,TextureSaturne };
    SaturneGO.etoile = 2;
    UranusGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.1f, 1 ,TextureUranus };
    UranusGO.etoile = 2;
    NeptuneGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.0f, 1 ,TextureNeptune };
    NeptuneGO.etoile = 2;
    MoonGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.50f, 1, TextureMoon };
    MoonGO.etoile = 2;
    etoileGO.material = Material{ {0.0f, 0.0f, 0.0f}, 1.0f, 0.0f, 0.0f, 1, textureSTAR };
    etoileGO.etoile = 2;

    pandoraGO.material = Material{ {0.0f, 0.0f, 0.0f},0.2f, 0.8f, 0.50f, 1, texturepandora };
    pandoraGO.etoile = 1;
    coruscantGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.50f, 1, texturecoruscant };
    coruscantGO.etoile = 1;

    dianaGO.material = Material{ {0.0f, 0.0f, 0.0f},0.2f, 0.8f, 0.50f, 1, texturediana };
    dianaGO.etoile = 1;
    anubisGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.50f, 1, textureanubis };
    anubisGO.etoile = 1;

    lokiGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.50f, 1, textureloki };
    lokiGO.etoile = 1;


    narutoGO.material = Material{ {0.0f, 0.0f, 0.0f},0.2f, 0.8f, 0.50f, 1, texturenaruto };
    narutoGO.etoile = 1;
    isisGO.material = Material{ {0.0f, 0.0f, 0.0f}, 0.2f, 0.8f, 0.50f, 1, textureisis };
    isisGO.etoile = 1;




    etoileGO.children.push_back(&etoileinvi);
    etoileinvi.children.push_back(&sunGO);
    etoileinvi.children.push_back(&sunDeux);
    sunGO.children.push_back(&MercureGO);
    sunGO.children.push_back(&VenusGO);
    sunGO.children.push_back(&EarthGO);
    sunGO.children.push_back(&MarsGO);
    sunGO.children.push_back(&JupiterGO);
    sunGO.children.push_back(&SaturneGO);
    sunGO.children.push_back(&UranusGO);
    sunGO.children.push_back(&NeptuneGO);

    sunDeux.children.push_back(&pandoraGO);
    sunDeux.children.push_back(&coruscantGO);

    sunDeux.children.push_back(&dianaGO);
    sunDeux.children.push_back(&anubisGO);

    sunDeux.children.push_back(&lokiGO);
    

    /* sunGO.children.push_back(&narutoGO);
     sunGO.children.push_back(&isisGO);*/


    EarthGO.children.push_back(&MoonGO);
    etoileGO.children.push_back(&etoileinvi);

    dianaGO.geometry = &sphere;
    dianaGO.vboID = vboSphereID;

    anubisGO.geometry = &sphere;
    anubisGO.vboID = vboSphereID;

    lokiGO.geometry = &sphere;
    lokiGO.vboID = vboSphereID;


    narutoGO.geometry = &sphere;
    isisGO.vboID = vboSphereID;

    isisGO.geometry = &sphere;
    isisGO.vboID = vboSphereID;


    etoileinvi.geometry = &sphere;
    etoileinvi.vboID = vboSphereID;
    sunDeux.geometry = &sphere;
    sunDeux.vboID = vboSphereID;
    pandoraGO.geometry = &sphere;
    pandoraGO.vboID = vboSphereID;

    coruscantGO.geometry = &sphere;
    coruscantGO.vboID = vboSphereID;

    sunGO.geometry = &sphere;
    sunGO.vboID = vboSphereID;

    etoileGO.geometry = &sphere;
    etoileGO.vboID = vboSphereID;

    MercureGO.geometry = &sphere;
    MercureGO.vboID = vboSphereID;

    VenusGO.geometry = &sphere;
    VenusGO.vboID = vboSphereID;

    EarthGO.geometry = &sphere;
    EarthGO.vboID = vboSphereID;

    MarsGO.geometry = &sphere;
    MarsGO.vboID = vboSphereID;

    JupiterGO.geometry = &sphere;
    JupiterGO.vboID = vboSphereID;

    SaturneGO.geometry = &sphere;
    SaturneGO.vboID = vboSphereID;

    UranusGO.geometry = &sphere;
    UranusGO.vboID = vboSphereID;

    NeptuneGO.geometry = &sphere;
    NeptuneGO.vboID = vboSphereID;

    MoonGO.geometry = &sphere;
    MoonGO.vboID = vboSphereID;







    const char* vertPath = "Shaders/color.vert";
    const char* fragPath = "Shaders/color.frag";

    FILE* vert = fopen(vertPath, "r");
    FILE* frag = fopen(fragPath, "r");

    Shader* shader = Shader::loadFromFiles(vert, frag);

    fclose(vert);
    fclose(frag);

    if (shader == nullptr) {
        std::cerr << "The shader 'color' did not compile correctly. Exiting." << std::endl;
        return EXIT_FAILURE;
    }


    float t = 0;

    bool isOpened = true;
    float keyZ = 0.0f;
    float keyQ = 0.0f;
    float keyS = 0.0f;
    float keyD = 0.0f;
    float keySPACE = 0.0f;
    float keyLSHIFT = 0.0f;
    bool keyA = false;
    bool keyE = false;
    float positionX = 0.5f;
    float positionY = 4.0f;
    float positionZ = 20.0f;
    float delta = 0.01f;
    float cameraAngle = 0.0f;
    //Main application loop
    while (isOpened)
    {
        //Time in ms telling us when this frame started. Useful for keeping a fix framerate
        uint32_t timeBegin = SDL_GetTicks();

        //Fetch the SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_CLOSE:
                    isOpened = false;
                    break;
                default:
                    break;
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_z:
                    keyZ = 0.1f;
                    break;
                case SDLK_q:
                    keyQ = 0.1f;
                    break;
                case SDLK_s:
                    keyS = 0.1f;
                    break;
                case SDLK_d:
                    keyD = 0.1f;
                    break;
                case SDLK_SPACE:
                    keySPACE = 0.1f;
                    break;
                case SDLK_LSHIFT:
                    keyLSHIFT = 0.1f;
                    break;
                case SDLK_a:
                    keyA = true;
                    break;
                case SDLK_e:
                    keyE = true;
                    break;
                default:break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_z:
                    keyZ = 0.0f;
                    break;
                case SDLK_q:
                    keyQ = 0.0f;
                    break;
                case SDLK_s:
                    keyS = 0.0f;
                    break;
                case SDLK_d:
                    keyD = 0.0f;
                    break;
                case SDLK_SPACE:
                    keySPACE = 0.0f;
                    break;
                case SDLK_LSHIFT:
                    keyLSHIFT = 0.0f;
                    break;
                case SDLK_a:
                    keyA = false;
                    break;
                case SDLK_e:
                    keyE = false;
                    break;
                default:break;
                }
                break;
                //We can add more event, like listening for the keyboard or the mouse. See SDL_Event documentation for more details
            }

        }
        positionX = positionX + keyD - keyQ;
        positionZ = positionZ - keyZ + keyS;
        positionY = positionY + keySPACE - keyLSHIFT;


        if (keyA) {
            cameraAngle += delta;

        }
        if (keyE) {
            cameraAngle -= delta;

        }

        //Clear the screen : the depth buffer and the color buffer
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);



        glm::mat4 camera(1.0f);
        glm::vec3 cameraPosition(positionX, positionY, positionZ);
        camera = glm::rotate(glm::mat4(1.0f), cameraAngle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(camera, cameraPosition);

       


        glm::mat4 view = glm::inverse(camera);
        glm::mat4 projection = glm::perspective(45.0f, WIDTH / (float)HEIGHT, 0.01f, 1000.0f);
        glm::mat4 model(1.0f);

        std::stack<glm::mat4> matrices;
        glm::vec3 lights[2]{};
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f)); //Warning: We passed from left-handed world coordinate to right-handed world coordinate due to glm::perspective
        model = glm::rotate(model, cameraAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat3 invModel3x3 = glm::inverse(glm::mat3(model));
        matrices.push(glm::mat4(1.0f));

        t += 0.01;
         etoileGO.localMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f));
         sunGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f))*glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 0.0f))*glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
         sunGO.propagatedMatrix= glm::rotate(glm::mat4(1.0f), t * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 0.0f));
         sunDeux.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f))* glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f))*glm::scale(glm::mat4(1.0f), glm::vec3( 2.0f, 2.0f, 2.0f));
             sunDeux.propagatedMatrix = glm::rotate(glm::mat4(1.0f), t * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));

         etoileinvi.localMatrix = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.0f, 1.0f, 0.0f))*glm::scale(glm::mat4(1.0f), glm::vec3(0.001f, 0.001f, 0.001f));
        // etoileinvi.propagatedMatrix = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.0f, 1.0f, 0.0f));
    
       

        MercureGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));;

        VenusGO.localMatrix = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
        
        EarthGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));

        EarthGO.propagatedMatrix = glm::rotate(glm::mat4(1.0f), t * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.0f, 0.0f));
        
        MoonGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 3.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-0.3f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, 0.12f, 0.12f));

        MarsGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.8f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.27f, 0.27f, 0.27f));

        JupiterGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.6f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 0.4f));

        SaturneGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.38f, 0.38f, 0.38f));

        UranusGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.4f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-7.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));

        NeptuneGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));

        pandoraGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.7f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(8.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));


        coruscantGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.4f, 0.4f, 0.42f));

        dianaGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.4f, glm::vec3(0.2f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.2f));

        anubisGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.6f, glm::vec3(0.4f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(9.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.39f, 0.39f, 0.39f));

        lokiGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(7.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.42f, 0.42f, 0.42f));


        narutoGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.68f, 0.68f, 0.68f));

        isisGO.localMatrix = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.68f, 0.68f, 0.68f));



        lights[0] = sunDeux.localMatrix[3];
        lights[1] = sunGO.localMatrix[3];
        ; //Warning: We passed from left-handed world coordinate to right-handed world coordinate due to glm::perspective

        glm::mat4 mvp = projection * view * model;
        draw(etoileGO, shader, matrices, cameraPosition, view, projection, lights);
        // draw(sunGO, shader, matrices,cameraPosition, view, projection);







          //Display on screen (swap the buffer on screen and the buffer you are drawing on)
        SDL_GL_SwapWindow(window);

        //Time in ms telling us when this frame ended. Useful for keeping a fix framerate
        uint32_t timeEnd = SDL_GetTicks();

        //We want FRAMERATE FPS
        if (timeEnd - timeBegin < TIME_PER_FRAME_MS)
            SDL_Delay((uint32_t)(TIME_PER_FRAME_MS)-(timeEnd - timeBegin));
    }

    glDeleteBuffers(1, &vboSphereID);
    delete shader;

    //Free everything
    if (context != NULL)
        SDL_GL_DeleteContext(context);
    if (window != NULL)
        SDL_DestroyWindow(window);

    return 0;
}