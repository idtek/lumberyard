/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/MeshInfo.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshInfo, EMStudio::UIAllocator, 0)

    MeshInfo::MeshInfo(EMotionFX::Actor* actor, EMotionFX::Node* node, unsigned int lodLevel, EMotionFX::Mesh* mesh)
        : m_lod(lodLevel)
    {
        // is deformable flag
        m_isDeformable = actor->CheckIfHasDeformableMesh(lodLevel, node->GetNodeIndex());
            
        // vertices, indices and polygons etc.
        m_verticesCount = mesh->GetNumVertices();
        m_indicesCount = mesh->GetNumIndices();
        m_polygonsCount = mesh->GetNumPolygons();
        m_isTriangleMesh = mesh->CheckIfIsTriangleMesh();
        m_isQuadMesh = mesh->CheckIfIsQuadMesh();
        m_orgVerticesCount = mesh->GetNumOrgVertices();

        if (m_orgVerticesCount)
        {
            m_vertexDupeRatio = mesh->GetNumVertices() / (float)mesh->GetNumOrgVertices();
        }
        else
        {
            m_vertexDupeRatio = 0.0f;
        }

        // skinning influences
        {
            AZStd::vector<uint32> vertexCounts;
            const uint32 maxNumInfluences = mesh->CalcMaxNumInfluences(vertexCounts);
            for (uint32 i = 0; i < maxNumInfluences; ++i)
            {
                m_verticesByInfluences.emplace_back(vertexCounts[i]);
            }
                
        }
        
        // sub meshes
        const uint32 numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(i);
            m_submeshes.emplace_back(actor, lodLevel, subMesh);
        }

        // vertex attribute layers
        const uint32 numVertexAttributeLayers = mesh->GetNumVertexAttributeLayers();
        AZStd::string tmpString;
        for (uint32 i = 0; i < numVertexAttributeLayers; ++i)
        {
            EMotionFX::VertexAttributeLayer* attributeLayer = mesh->GetVertexAttributeLayer(i);

            const uint32 attributeLayerType = attributeLayer->GetType();
            switch (attributeLayerType)
            {
            case EMotionFX::Mesh::ATTRIB_POSITIONS:
                tmpString = "Vertex positions";
                break;
            case EMotionFX::Mesh::ATTRIB_NORMALS:
                tmpString = "Vertex normals";
                break;
            case EMotionFX::Mesh::ATTRIB_TANGENTS:
                tmpString = "Vertex tangents";
                break;
            case EMotionFX::Mesh::ATTRIB_UVCOORDS:
                tmpString = "Vertex uv coordinates";
                break;
            case EMotionFX::Mesh::ATTRIB_COLORS32:
                tmpString = "Vertex colors in 32-bits";
                break;
            case EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS:
                tmpString = "Original vertex numbers";
                break;
            case EMotionFX::Mesh::ATTRIB_COLORS128:
                tmpString = "Vertex colors in 128-bits";
                break;
            case EMotionFX::Mesh::ATTRIB_BITANGENTS:
                tmpString = "Vertex bitangents";
                break;
            default:
                tmpString = AZStd::string::format("Unknown data (TypeID=%d)", attributeLayerType);
            }

            if (attributeLayer->GetNameString().size() > 0)
            {
                tmpString += AZStd::string::format(" [%s]", attributeLayer->GetName());
            }

            m_attributeLayers.emplace_back(attributeLayer->GetTypeString(), tmpString);
        }


        // shared vertex attribute layers
        const uint32 numSharedVertexAttributeLayers = mesh->GetNumSharedVertexAttributeLayers();
        for (uint32 i = 0; i < numSharedVertexAttributeLayers; ++i)
        {
            EMotionFX::VertexAttributeLayer* attributeLayer = mesh->GetSharedVertexAttributeLayer(i);

            const uint32 attributeLayerType = attributeLayer->GetType();
            switch (attributeLayerType)
            {
            case EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID:
                tmpString = "Skinning info";
                break;
            default:
                tmpString = AZStd::string::format("Unknown data (TypeID=%d)", attributeLayerType);
            }

            if (attributeLayer->GetNameString().size() > 0)
            {
                tmpString += AZStd::string::format(" [%s]", attributeLayer->GetName());
            }

            m_sharedAttributeLayers.emplace_back(attributeLayer->GetTypeString(), tmpString);
        }
    }

    void MeshInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MeshInfo>()
            ->Version(1)
            ->Field("lod", &MeshInfo::m_lod)
            ->Field("isDeformable", &MeshInfo::m_isDeformable)
            ->Field("verticesCount", &MeshInfo::m_verticesCount)
            ->Field("indicesCount", &MeshInfo::m_indicesCount)
            ->Field("polygonsCount", &MeshInfo::m_polygonsCount)
            ->Field("isTriangleMesh", &MeshInfo::m_isTriangleMesh)
            ->Field("isQuadMesh", &MeshInfo::m_isQuadMesh)
            ->Field("orgVerticesCount", &MeshInfo::m_orgVerticesCount)
            ->Field("vertexDupeRatio", &MeshInfo::m_vertexDupeRatio)
            ->Field("verticesByInfluence", &MeshInfo::m_verticesByInfluences)
            ->Field("submeshes", &MeshInfo::m_submeshes)
            ->Field("attributeLayers", &MeshInfo::m_attributeLayers)
            ->Field("sharedAttributeLayers", &MeshInfo::m_sharedAttributeLayers)
           ;
        
        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<MeshInfo>("Mesh info", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_lod, "LOD level", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_isDeformable, "Deformable", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_verticesCount, "Vertices", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_indicesCount, "Indices", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_polygonsCount, "Polygons", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_isTriangleMesh, "Is triangle mesh", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_isQuadMesh, "Is quad mesh", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_orgVerticesCount, "Org vertices", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_vertexDupeRatio, "Vertex dupe ratio", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_verticesByInfluences, "Vertices by influence", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_submeshes, "Sub meshes", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_attributeLayers, "Attribute layers", "")
            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshInfo::m_sharedAttributeLayers, "Shared attribute layers", "")
            ;

    }

} // namespace EMStudio
