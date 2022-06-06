#ifndef  GEOMETRY_INC
#define  GEOMETRY_INC

#include <stdlib.h>
#include <stdint.h>

/* \brief Represent a geometry*/
class Geometry
{
    public:
        /* \brief The constructor.*/
        Geometry();

        /* \brief Copy constructor
         * \param copy the object to copy*/
        Geometry(const Geometry& copy);

        /* \brief Move constructor
         * \param mvt the object to move. Do not use it afterward*/
        Geometry(Geometry&& mvt) noexcept;

        /* \brief Assign operator. Make a copy of "copy"
         * \param copy the object to copy
         * \return a reference to "this"*/
        Geometry& operator=(const Geometry& copy);

        /* \brief Destructor. Destroy the data */
        virtual ~Geometry();
        
        /* \brief Get the vertices data of the geometry
         * \return const array on the vertices data. Use getNbVertices to get how many vertices the array contains (size(array) == 3*nbVertices) */
        const float* getVertices() const {return m_vertices;}

        /* \brief Get the normals data of the geometry
         * \return const array on the normals data. Use getNbVertices to get how many vertices the array contains (size(array) == 3*nbVertices) */
        const float* getNormals() const {return m_normals;}

        /* \brief Get the UV mapping data of the geometry
         * \return const array on the UV mapping data. Use getNbVertices to get how many vertices the array contains (size(array) == 2*nbVertices) */
        const float* getUVs() const {return m_uvs;}
        
        /* \brief Get how many vertices this geometry contains
         * \return the number of vertices this geometry contains*/
        uint32_t getNbVertices() const {return m_nbVertices;}

    protected: 
        /* \brief Clear all the tables*/
        void clear();

        uint32_t m_nbVertices = 0;
        float*   m_vertices   = NULL;
        float*   m_normals    = NULL;
        float*   m_uvs        = NULL;
};

#endif
