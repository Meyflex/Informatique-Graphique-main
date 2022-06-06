#include "Circle.h"
#include "logger.h"

Circle::Circle(uint32_t nbEdge) : Geometry()
{
    if(nbEdge < 3)
        ERROR("The parameter 'nbEdge' should be three or greater\n");

	m_vertices = (float*)malloc(9*(uint64_t)nbEdge*sizeof(float));
    m_uvs      = (float*)malloc(6*(uint64_t)nbEdge*sizeof(float));
	m_normals  = (float*)malloc(9*(uint64_t)nbEdge*sizeof(float));
    m_nbVertices = 3*nbEdge;

    const float PI = (float)M_PI;

	for(uint32_t i=0; i < nbEdge; i++)
	{
		float pos[] = {(float)cos(i*2*PI/nbEdge), (float)sin(i*2*PI/nbEdge), 0.0f,
                       0.0f, 0.0f, 0.0f,
                       (float)cos((i+1)*2*PI/nbEdge), (float)sin((i+1)*2*PI/nbEdge), 0.0f};

		for(uint32_t j=0; j < 9; j++)
			m_vertices[9*i+j] = 0.5f*pos[j];
        for(uint32_t j=0; j < 3; j++)
            for(uint32_t k = 0; k < 2; k++)
                m_uvs[6*i+2*j+k] = m_vertices[9*i+3*j+k]+0.5f;

        for(uint32_t j=0; j < 3; j++)
        {
            float normal[] = {0.0f, 0.0f, 1.0f};
            for(uint32_t k=0; k < 3; k++)
                m_normals[9*i+3*j+k] = normal[k];
        }
	}
}
