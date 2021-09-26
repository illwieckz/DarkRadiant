#pragma once

#include "ibrush.h"
#include "NodeBase.h"
#include "math/Matrix3.h"
#include "SelectableVertex.h"

namespace textool
{

class FaceNode :
    public NodeBase,
    public IFaceNode
{
private:
    IFace& _face;
    mutable AABB _bounds;

public:
    FaceNode(IFace& face) :
        _face(face)
    {
        for (auto& vertex : _face.getWinding())
        {
            _vertices.emplace_back(vertex.vertex, vertex.texcoord);
        }
    }

    IFace& getFace() override
    {
        return _face;
    }

    void beginTransformation() override
    {
        _face.undoSave();
    }

    void revertTransformation() override
    {
        _face.revertTransform();
    }

    void transform(const Matrix3& transform) override
    {
        for (auto& vertex : _face.getWinding())
        {
            vertex.texcoord = transform * vertex.texcoord;
        }

        Vector3 vertices[3] = { _face.getWinding().at(0).vertex, _face.getWinding().at(1).vertex, _face.getWinding().at(2).vertex };
        Vector2 texcoords[3] = { _face.getWinding().at(0).texcoord, _face.getWinding().at(1).texcoord, _face.getWinding().at(2).texcoord };

        _face.setTexDefFromPoints(vertices, texcoords);
    }

    void transformComponents(const Matrix3& transform) override
    {
        transformSelectedAndRecalculateTexDef([&](Vector2& selectedTexcoord)
        {
            selectedTexcoord = transform * selectedTexcoord;
        });
    }

    void commitTransformation() override
    {
        _face.freezeTransform();
    }

    const AABB& localAABB() const
    {
        _bounds = AABB();

        for (const auto& vertex : _face.getWinding())
        {
            _bounds.includePoint({ vertex.texcoord.x(), vertex.texcoord.y(), 0 });
        }

        return _bounds;
    }

    void testSelect(Selector& selector, SelectionTest& test) override
    {
        // Arrange the UV coordinates in a Vector3 array for testing
        std::vector<Vector3> uvs;
        uvs.reserve(_face.getWinding().size());

        for (const auto& vertex : _face.getWinding())
        {
            uvs.emplace_back(vertex.texcoord.x(), vertex.texcoord.y(), 0);
        }

        test.BeginMesh(Matrix4::getIdentity(), true);

        SelectionIntersection best;
        test.TestPolygon(VertexPointer(uvs.data(), sizeof(Vector3)), uvs.size(), best);

        if (best.isValid())
        {
            Selector_add(selector, *this);
        }
    }

    void render(SelectionMode mode) override
    {
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.3f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        if (mode == SelectionMode::Surface && isSelected())
        {
            glColor3f(1, 0.5f, 0);
        }
        else if (mode == SelectionMode::Vertex)
        {
            glColor3f(0.6f, 0.6f, 0.6f);
        }
        else
        {
            glColor3f(0.8f, 0.8f, 0.8f);
        }

        glBegin(GL_TRIANGLE_FAN);

        for (const auto& vertex : _face.getWinding())
        {
            glVertex2d(vertex.texcoord[0], vertex.texcoord[1]);
        }

        glEnd();
        glDisable(GL_BLEND);

        if (mode == SelectionMode::Vertex)
        {
            renderComponents();
        }
    }

    void expandSelectionToRelated() override
    {
        if (!isSelected())
        {
            return;
        }

        // Expand the selection to all faces with the same brush
        auto& brush = _face.getBrush();

        GlobalTextureToolSceneGraph().foreachNode([&](const INode::Ptr& node)
        {
            auto face = std::dynamic_pointer_cast<FaceNode>(node);

            if (face && &(face->getFace().getBrush()) == &brush)
            {
                face->setSelected(true);
            }

            return true;
        });
    }

    void snapto(float snap) override
    {
        for (auto& vertex : _vertices)
        {
            auto& texcoord = vertex.getTexcoord();
            texcoord.x() = float_snapped(texcoord.x(), snap);
            texcoord.y() = float_snapped(texcoord.y(), snap);
        }

        // Take three vertices and calculate a new tex def
        Vector3 vertices[3];
        Vector2 texcoords[3];

        for (std::size_t i = 0; i < 3; ++i)
        {
            vertices[i] = _vertices[i].getVertex();
            texcoords[i] = _vertices[i].getTexcoord();
        }

        _face.setTexDefFromPoints(vertices, texcoords);
    }

    void snapComponents(float snap) override
    {
        transformSelectedAndRecalculateTexDef([&](Vector2& selectedTexcoord)
        {
            // Snap the selection to the grid
            selectedTexcoord.x() = float_snapped(selectedTexcoord.x(), snap);
            selectedTexcoord.y() = float_snapped(selectedTexcoord.y(), snap);
        });
    }

    void mergeComponentsWith(const Vector2& center) override
    {
        // TODO
    }

private:
    void transformSelectedAndRecalculateTexDef(const std::function<void(Vector2&)>& transform)
    {
        std::vector<std::size_t> selectedIndices;

        // We need to remember the old texture coordinates to determine the fixed points
        std::vector<Vector2> unchangedTexcoords;
        AABB selectionBounds;

        // Manipulate every selected vertex using the given transform
        for (std::size_t i = 0; i < _vertices.size(); ++i)
        {
            auto& vertex = _vertices[i];

            unchangedTexcoords.push_back(vertex.getTexcoord());

            if (!vertex.isSelected()) continue;

            selectionBounds.includePoint({ vertex.getTexcoord().x(), vertex.getTexcoord().y(), 0 });
            selectedIndices.push_back(i);

            // Apply the transform to the selected texcoord
            transform(vertex.getTexcoord());
        }

        if (selectedIndices.empty()) return; // nothing happened

        // Now we need to pick three vertices to calculate the tex def from
        // we have certain options, depending on the number of selected vertices
        auto selectionCount = selectedIndices.size();

        Vector3 vertices[3];
        Vector2 texcoords[3];
        const auto& winding = _face.getWinding();

        if (selectionCount >= 3)
        {
            // Manipulating 3+ vertices means that the whole face is transformed 
            // the same way. We can pick any of the three selected vertices.
            for (std::size_t i = 0; i < 3; ++i)
            {
                vertices[i] = _vertices[selectedIndices[i]].getVertex();
                texcoords[i] = _vertices[selectedIndices[i]].getTexcoord();
            }
        }
        else if (selectionCount == 2)
        {
            // Calculate the center point of the selection and pick the vertex that is farthest from it
            auto farthestIndex = findIndexFarthestFrom(
                { selectionBounds.origin.x(), selectionBounds.origin.y() },
                unchangedTexcoords, selectedIndices);

            for (std::size_t i = 0; i < 2; ++i)
            {
                vertices[i] = _vertices[selectedIndices[i]].getVertex();
                texcoords[i] = _vertices[selectedIndices[i]].getTexcoord();
            }

            vertices[2] = _vertices[farthestIndex].getVertex();
            texcoords[2] = _vertices[farthestIndex].getTexcoord();
        }
        else // selectionCount == 1
        {
            assert(selectionCount == 1);
            std::vector<std::size_t> fixedVerts{ selectedIndices[0] };

            auto secondIndex = findIndexFarthestFrom(unchangedTexcoords[selectedIndices[0]],
                unchangedTexcoords, fixedVerts);
            fixedVerts.push_back(secondIndex);

            // Now we've got two vertices, calculate the center and take the farthest of that one
            auto center = (unchangedTexcoords[secondIndex] + unchangedTexcoords[selectedIndices[0]]) * 0.5;
            auto thirdIndex = findIndexFarthestFrom(center, unchangedTexcoords, fixedVerts);
            fixedVerts.push_back(thirdIndex);

            for (std::size_t i = 0; i < 3; ++i)
            {
                vertices[i] = _vertices[fixedVerts[i]].getVertex();
                texcoords[i] = _vertices[fixedVerts[i]].getTexcoord();
            }
        }

        _face.setTexDefFromPoints(vertices, texcoords);
    }

    // Locates the index of the vertex that is farthest away from the given texcoord
    // the indices contained in exludedIndices are not returned
    static std::size_t findIndexFarthestFrom(const Vector2& texcoord, 
        const std::vector<Vector2>& allCoords, const std::vector<std::size_t>& excludedIndices)
    {
        assert(!allCoords.empty());

        std::size_t farthestIndex = 0;
        double largestDistanceSquared = 0;

        for (std::size_t i = 0; i < allCoords.size(); ++i)
        {
            if (std::find(excludedIndices.begin(), excludedIndices.end(), i) != excludedIndices.end()) continue;

            auto candidateDistanceSquared = (allCoords[i] - texcoord).getLengthSquared();

            if (candidateDistanceSquared > largestDistanceSquared)
            {
                farthestIndex = i;
                largestDistanceSquared = candidateDistanceSquared;
            }
        }

        return farthestIndex;
    }
};

}
