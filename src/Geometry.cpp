#include "Geometry.h"
#include <cstring>

Geometry::Geometry(){}

Geometry::Geometry(const Geometry& copy)
{
    *this = copy;
}

Geometry::Geometry(Geometry&& mvt) noexcept
{
    m_nbVertices = mvt.m_nbVertices;
    m_vertices   = mvt.m_vertices;
    m_normals    = mvt.m_normals;
    m_uvs        = mvt.m_uvs;

    mvt.m_vertices   = mvt.m_normals = mvt.m_uvs = nullptr;
    mvt.m_nbVertices = 0;
}

Geometry& Geometry::operator=(const Geometry& copy)
{
    if(this != &copy)
    {
        clear();
        if (copy.getNbVertices() == 0) return *this;

        m_nbVertices = copy.m_nbVertices;
        m_vertices = (float*)malloc((uint64_t)getNbVertices()*3*sizeof(float));
        if(m_vertices != nullptr)
            memcpy(m_vertices, copy.m_vertices, (uint64_t)getNbVertices()*3*sizeof(float));
        
        m_normals = (float*)malloc((uint64_t)getNbVertices()*3*sizeof(float));
        if(m_normals != nullptr)
            memcpy(m_normals, copy.m_normals, (uint64_t)getNbVertices()*3*sizeof(float));
        
        m_uvs = (float*)malloc((uint64_t)getNbVertices()*2*sizeof(float));
        if(m_uvs != nullptr)
            memcpy(m_uvs, copy.m_uvs, (uint64_t)getNbVertices()*2*sizeof(float));
    }

    return *this;
}

Geometry::~Geometry()
{
    clear();
}

void Geometry::clear()
{
    if(m_vertices)
        free(m_vertices);
    if(m_normals)
        free(m_normals);
    if(m_uvs)
        free(m_uvs);
    m_vertices = m_normals = m_uvs = nullptr;
    m_nbVertices = 0;
}