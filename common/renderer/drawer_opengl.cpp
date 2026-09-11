#include "drawer_opengl.hpp"

#include <GL/glew.h>

namespace
{
    inline void emitVertex(
        const geometry::Point3f& point
    )
    {
        glVertex3f(
            point[0],
            point[1],
            point[2]
        );
    }

    inline void setColor(
        const geometry::Color& color
    )
    {
        glColor4f(
            color.r,
            color.g,
            color.b,
            color.a
        );
    }
}

void DrawerOpenGL::drawVertex(
    const geometry::Point3f& point,
    const geometry::Color& color,
    float size
)
{
    glPointSize(size);

    setColor(color);

    glBegin(GL_POINTS);

    emitVertex(point);

    glEnd();
}

void DrawerOpenGL::drawLine(
    const geometry::Point3f& a,
    const geometry::Point3f& b,
    const geometry::Color& color,
    float width
)
{
    glLineWidth(width);

    setColor(color);

    glBegin(GL_LINES);

    emitVertex(a);
    emitVertex(b);

    glEnd();
}

void DrawerOpenGL::drawFace(
    const geometry::Point3f& a,
    const geometry::Point3f& b,
    const geometry::Point3f& c,
    const geometry::Color& color
)
{
    setColor(color);

    glBegin(GL_TRIANGLES);

    emitVertex(a);
    emitVertex(b);
    emitVertex(c);

    glEnd();
}

void DrawerOpenGL::drawMesh(
    const geometry::Mesh3f& mesh,
    const geometry::Color& color
)
{
    const auto& vertices =
        mesh.getVertices();

    const auto& edges =
        mesh.getEdges();

    const auto& faces =
        mesh.getFaces();

    if (!faces.empty())
    {
        setColor(color);

        glBegin(GL_TRIANGLES);

        for (const auto& face : faces)
        {
            emitVertex(
                vertices[
                    face.indices[0]
                ]
            );

            emitVertex(
                vertices[
                    face.indices[1]
                ]
            );

            emitVertex(
                vertices[
                    face.indices[2]
                ]
            );
        }

        glEnd();
    }

    if (!edges.empty())
    {
        setColor({
            0.0f,
            0.0f,
            0.0f,
            1.0f
        });

        glLineWidth(1.0f);

        glBegin(GL_LINES);

        for (const auto& edge : edges)
        {
            emitVertex(
                vertices[
                    edge.v1
                ]
            );

            emitVertex(
                vertices[
                    edge.v2
                ]
            );
        }

        glEnd();
    }

    if (!vertices.empty())
    {
        setColor({
            1.0f,
            0.0f,
            0.0f,
            1.0f
        });

        glPointSize(5.0f);

        glBegin(GL_POINTS);

        for (const auto& vertex : vertices)
        {
            emitVertex(vertex);
        }

        glEnd();
    }
}