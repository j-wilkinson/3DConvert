#include "C3DModelOBJ.h"
#include "C3DModelDataStructures.h"
#include <climits>
#include <iostream>


#define STU_EXPORT_SEQUENTIAL 1 //When enabled we write to the file at each model (much better memory usage, but may be slightly slower)

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define LOG_ERROR(...) printf("C3DModelOBJ:"); printf(__VA_ARGS__);
#define LOG_INFO(...) printf("C3DModelOBJ:"); printf(__VA_ARGS__);

#define WRITE_VALUE(x)          Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x)); uSize += sizeof(x);
#define WRITE_VALUES(x, c)      Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x) * (c)); uSize += sizeof(x) * (c);
#define WRITE_BYTES(x, y, c)    Data.insert(Data.end(), (uint8_t *)(x), (uint8_t *)(x) + sizeof(y) * (c)); uSize += sizeof(y) * (c);

static const std::string LOG_TAG("C3DModelOBJ");

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3])
{
    float v10[3];
    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];

    float v20[3];
    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];

    N[0] = v20[1] * v10[2] - v20[2] * v10[1];
    N[1] = v20[2] * v10[0] - v20[0] * v10[2];
    N[2] = v20[0] * v10[1] - v20[1] * v10[0];

    float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
        float len = sqrtf(len2);

        N[0] /= len;
        N[1] /= len;
    }
}

C3DModelOBJ::C3DModelOBJ() :
  m_bFlipUVonY(false),
  m_TotalMeshCount(0),
  m_uSubModelCount(0),
  m_uSubModelVertexCount(0)
{
    m_SolidColor[0] = 0.5f;
    m_SolidColor[1] = 0.5f;
    m_SolidColor[2] = 0.5f;
}

C3DModelOBJ::~C3DModelOBJ()
{
}

bool C3DModelOBJ::ExportToSTUFormat(const std::string &path)
{
    m_uSubModelCount = 0;
    m_uSubModelVertexCount = 0;
    return ExportToSTUFormat(path, true);
}

static void WriteVertexChunk(CFileExportSTUFormat Export, const std::string &path, bool bFlipUV, std::string sName, tinyobj::mesh_t mesh, tinyobj::attrib_t attrib)
{
    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;
    float bmin[3], bmax[3];

    bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
    bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

    if (attrib.normals.size() > 0)
    {
        VertexDataWithNormals Vertex;
        Data.reserve(sizeof(VertexDataWithNormals) * mesh.indices.size());
        if (Data.capacity() < (sizeof(VertexDataWithNormals) * mesh.indices.size()))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", path.c_str());
            return;
        }
        for (size_t f = 0; f < mesh.indices.size(); f++)
        {
            tinyobj::index_t idx0 = mesh.indices[f];

            int f0 = 3 * idx0.vertex_index;
            assert(f0 >= 0);

            Vertex.position.x = attrib.vertices[f0 + 0];
            Vertex.position.y = attrib.vertices[f0 + 1];
            Vertex.position.z = attrib.vertices[f0 + 2];
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            assert(idx0.normal_index >= 0);
            int fn0 = 3 * idx0.normal_index;
            Vertex.normal.x = attrib.normals[fn0 + 0];
            Vertex.normal.y = attrib.normals[fn0 + 1];
            Vertex.normal.z = attrib.normals[fn0 + 2];
            if (attrib.texcoords.size() > 0 && idx0.texcoord_index >= 0)
            {
                Vertex.texcoord.x = attrib.texcoords[2 * idx0.texcoord_index];
                Vertex.texcoord.y = bFlipUV ? 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1] : attrib.texcoords[2 * idx0.texcoord_index + 1];
            }
            WRITE_VALUE(Vertex);
            //LOG_ERROR("Vx( %.02f,  %.02f, %.02f )", Vertex.position.x, Vertex.position.y, Vertex.position.z);
        }
    }
    else
    {
        VertexDataTextured Vertex;
        Data.reserve(sizeof(VertexDataTextured) * mesh.indices.size());
        if (Data.capacity() < (sizeof(VertexDataTextured) * mesh.indices.size()))
        {
            LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", path.c_str());
            return;
        }
        for (size_t f = 0; f < mesh.indices.size(); f++)
        {
            tinyobj::index_t idx0 = mesh.indices[f];

            int f0 = 3 * idx0.vertex_index;
            assert(f0 >= 0);

            Vertex.position.x = attrib.vertices[f0 + 0];
            Vertex.position.y = attrib.vertices[f0 + 1];
            Vertex.position.z = attrib.vertices[f0 + 2];
            bmin[0] = std::min(Vertex.position.x, bmin[0]);
            bmin[1] = std::min(Vertex.position.y, bmin[1]);
            bmin[2] = std::min(Vertex.position.z, bmin[2]);
            bmax[0] = std::max(Vertex.position.x, bmax[0]);
            bmax[1] = std::max(Vertex.position.y, bmax[1]);
            bmax[2] = std::max(Vertex.position.z, bmax[2]);
            if (attrib.texcoords.size() > 0 && idx0.texcoord_index >= 0)
            {
                Vertex.texcoord.x = attrib.texcoords[2 * idx0.texcoord_index];
                Vertex.texcoord.y = bFlipUV ? 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1] : attrib.texcoords[2 * idx0.texcoord_index + 1];
            }
            WRITE_VALUE(Vertex);
        }
    }
#if STU_EXPORT_SEQUENTIAL
    std::string sSTUPath = path;
    sSTUPath.append(".stu");
    Export.AppendChunkToFile(sSTUPath, sName, &Data.at(0), (uint32_t)Data.size());
#else
    Export.WriteChunk(sName, &Data.at(0), (uint32_t)Data.size());
#endif
    std::vector< uint8_t >().swap(Data);

    //Write BBox chunk:
    WRITE_VALUE(bmin[0]);
    WRITE_VALUE(bmin[1]);
    WRITE_VALUE(bmin[2]);
    WRITE_VALUE(bmax[0]);
    WRITE_VALUE(bmax[1]);
    WRITE_VALUE(bmax[2]);

#if STU_EXPORT_SEQUENTIAL
    Export.AppendChunkToFile(sSTUPath, sName + "BB", &Data.at(0), (int32_t)Data.size());
#else
    Export.WriteChunk(sName + "BB", &Data.at(0), (int32_t)Data.size());
#endif
    std::vector< uint8_t >().swap(Data);
}

bool C3DModelOBJ::ExportToSTUFormat(const std::string &path, bool bFlipUV)
{
    uint32_t NumVertices = 0;
    uint32_t uSize = 0;
    uint32_t uValue;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;

    m_uSubModelCount = 0;
    m_uSubModelVertexCount = 0;
    m_bFlipUVonY = bFlipUV;
    m_Entries.clear();
    m_TotalMeshCount = 0;

    std::vector<tinyobj::material_t> materials;
    std::map<std::string, uint32_t> textures;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::string MaterialPath = path.substr(0, path.find_last_of("/") + 1);

    std::string err;
    bool ret =
        tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), MaterialPath.c_str());
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        LOG_ERROR("Could not load '%s' model.", path.c_str());
        return false;
    }

    LOG_INFO("loading '%s' model.", path.c_str());
    LOG_INFO("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
    LOG_INFO("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
    LOG_INFO("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
    LOG_INFO("# of materials = %d\n", (int)materials.size());
    LOG_INFO("# of shapes    = %d\n", (int)shapes.size());

    m_Entries.resize((int)shapes.size());

    // Append `default` material
    tinyobj::material_t defaultcolor = tinyobj::material_t();
    defaultcolor.ambient[0] = defaultcolor.diffuse[0] = defaultcolor.specular[0] = m_SolidColor[0];
    defaultcolor.ambient[1] = defaultcolor.diffuse[1] = defaultcolor.specular[1] = m_SolidColor[1];
    defaultcolor.ambient[2] = defaultcolor.diffuse[2] = defaultcolor.specular[2] = m_SolidColor[2];
    defaultcolor.shininess = 8.0;
    defaultcolor.dissolve = 1.0;
    materials.push_back(defaultcolor);

    // You.i engine does not support multi-mesh, so for now, let's create a child node per mesh.
    for (size_t s = 0; s < shapes.size(); s++) 
    {
        Data.clear();
        std::string nodeName;
        if (strlen(shapes[s].name.c_str()) > 0)
        {
            nodeName = std::string("OBJ.(") + shapes[s].name.c_str() + "-" + std::to_string(m_uUniqueOBJUnknownID++) + ")";
        }
        else
        {
            nodeName = std::string("OBJ.(UNKNOWN-") + std::to_string(m_uUniqueOBJUnknownID++) + ")";
        }
        LOG_INFO("Found node '%s'\n", nodeName.c_str());

        glm::mat4 matrix;

        uValue = 0; //Always 0  Children
        WRITE_VALUE(uValue);

        uSize += CFileExportSTUFormat::CopyString(nodeName.c_str(), &Data);

        uSize += CFileExportSTUFormat::CopyString(shapes[s].name.c_str(), &Data);

        // node transformation matrix is identity
        WRITE_VALUE(matrix);

        uValue = 1; //Always 1 mesh
        WRITE_VALUE(uValue);

        std::string meshName;
        static uint32_t sUniqueOBJUnknownMeshID = 0;
        if (strlen(shapes[s].name.c_str()) > 0)
        {
            meshName = nodeName + ".mesh(" + shapes[s].name.c_str() + "-" + std::to_string(sUniqueOBJUnknownMeshID++) + ")";
        }
        else
        {
            meshName = nodeName + ".mesh(UNKNOWN-" + std::to_string(sUniqueOBJUnknownMeshID++) + ")";
        }
        std::string VBOName = meshName + ".VBO";
        std::string IBOName = meshName + ".IBO";

        uSize += CFileExportSTUFormat::CopyString(meshName.c_str(), &Data);

        uSize += CFileExportSTUFormat::CopyString(shapes[s].name.c_str(), &Data);

        uValue = 0; //Always no animation
        WRITE_VALUE(uValue);

        uValue = (uint32_t)shapes[s].mesh.indices.size(); //Always triangles, so same as indices
        WRITE_VALUE(uValue);

        uValue = VertexDataType_Normals;
        WRITE_VALUE(uValue);

        std::string sVertexChunkname = std::string("Vx:") + std::to_string(m_uSubModelVertexCount ++);
        WriteVertexChunk(m_Export, path, bFlipUV, sVertexChunkname, shapes[s].mesh, attrib);

        uValue = 0;// shapes[s].mesh.indices.size();
        WRITE_VALUE(uValue);

        //WRITE_VALUES(shapes[s].mesh.indices[0], shapes[s].mesh.indices.size());
            
        uValue = PrimitiveType_TRIANGLE;
        WRITE_VALUE(uValue);

        uValue = 0; //Always 2 sides
        WRITE_VALUE(uValue);

        float AmbientColor[3], DiffuseColor[3], SpecularColor[3];
        float fShininess, fAlpha;

        int current_material_id = shapes[s].mesh.material_ids[0];

        if ((current_material_id < 0) || (current_material_id >= static_cast<int>(materials.size())))
        {
            // Invaid material ID. Use default material.
            current_material_id = (uint32_t)materials.size() - 1; // Default material is added to the last item in `materials`.
        }
        for (size_t i = 0; i < 3; i++)
        {
            AmbientColor[i] = materials[current_material_id].ambient[i];
            DiffuseColor[i] = materials[current_material_id].diffuse[i];
            SpecularColor[i] = materials[current_material_id].specular[i];
        }
        fShininess = materials[current_material_id].shininess;
        fAlpha = materials[current_material_id].dissolve;

        if (fAlpha < 0.001f)
        {
            LOG_ERROR("OBJ Alpha is ZERO!\n");
            fAlpha = 1.0f;
        }

        WRITE_VALUE(AmbientColor);
        WRITE_VALUE(DiffuseColor);
        WRITE_VALUE(SpecularColor);
        WRITE_VALUE(fShininess);
        WRITE_VALUE(fAlpha);

        std::string sFullfilename = materials[current_material_id].diffuse_texname.c_str();
        std::string sFile = CFileExportSTUFormat::RemoveFoldersFromPaths(sFullfilename);
        if (sFile.length() > 0)
        {
            uValue = TextureType_DIFFUSE;
            WRITE_VALUE(uValue);
            CImagePreProcess::ReplaceUnrecognizedInternalTextureFormats(sFile);
            uSize += CFileExportSTUFormat::CopyString(sFile.c_str(), &Data);
        }
        else
        {
            uValue = TextureType_COLOR_DIFFUSE;
            WRITE_VALUE(uValue);
            WRITE_VALUE(fAlpha);
        }

        sFullfilename = materials[current_material_id].bump_texname.c_str();
        sFile = CFileExportSTUFormat::RemoveFoldersFromPaths(sFullfilename);
        if (sFile.length() > 0)
        {
            uValue = TextureType_HEIGHT;
            WRITE_VALUE(uValue);
            CImagePreProcess::ReplaceUnrecognizedInternalTextureFormats(sFile);
            uSize += CFileExportSTUFormat::CopyString(sFile.c_str(), &Data);
        }
        else
        {
        uValue = TextureType_UNKNOWN;   //Skip normal map for now.
        WRITE_VALUE(uValue);
        }

        std::string sChunkname = std::string("Model:") + std::to_string(m_uSubModelCount++);

#if STU_EXPORT_SEQUENTIAL
        std::string sSTUPath = path;
        sSTUPath.append(".stu");
        m_Export.AppendChunkToFile(sSTUPath, sChunkname, &Data.at(0), (uint32_t)Data.size());
#else
        m_Export.WriteChunk(sChunkname, &Data.at(0), (uint32_t)Data.size());
#endif
    }

#if !STU_EXPORT_SEQUENTIAL
    std::string sSTUPath = path;
    sSTUPath.Append(".stu");
    m_Export.ExportFile(sSTUPath);
#endif

    return true;
}
