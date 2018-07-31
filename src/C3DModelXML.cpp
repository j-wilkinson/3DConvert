#include "C3DModelXML.h"
#include "C3DModelDataStructures.h"
#include <climits>

#define STU_EXPORT_SEQUENTIAL 1 //When enabled we write to the file at each model (much better memory usage, but may be slightly slower)

/*
char                       filePath[250];
std::vector<Ptr<Model> >   Models;
int                        collisionModelCount;
int                        groundCollisionModelCount;
*/

#define LOG_ERROR(...) printf("C3DModelXML:"); printf(__VA_ARGS__);
#define LOG_INFO(...) printf("C3DModelXML:"); printf(__VA_ARGS__);

#define WRITE_VALUE(x)          Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x)); uSize += sizeof(x);
#define WRITE_VALUES(x, c)      Data.insert(Data.end(), (uint8_t *)&(x), (uint8_t *)&(x) + sizeof(x) * (c)); uSize += sizeof(x) * (c);
#define WRITE_BYTES(x, y, c)    Data.insert(Data.end(), (uint8_t *)(x), (uint8_t *)(x) + sizeof(y) * (c)); uSize += sizeof(y) * (c);

static const std::string LOG_TAG("C3DModelXML");

C3DModelXML::C3DModelXML() :
m_TotalMeshCount(0),
m_uSubModelCount(0),
m_uSubModelVertexCount(0),
m_pXmlDocument(NULL),
m_bFlipUVonY(false),
m_bFlipOnX(false),
m_bFlipOnY(false),
m_bFlipOnZ(false)
{
    m_SolidColor[0] = 0.5f;
    m_SolidColor[1] = 0.5f;
    m_SolidColor[2] = 0.5f;
    m_pXmlDocument = new tinyxml2::XMLDocument();
}

C3DModelXML::~C3DModelXML()
{
    delete m_pXmlDocument;
}

void C3DModelXML::ParseVectorString(const char* str, std::vector<glm::vec3> *array, bool is2element)
{
    size_t stride = is2element ? 2 : 3;
    size_t stringLength = strlen(str);
    size_t element = 0;
    float v[3];

    for (size_t j = 0; j < stringLength;)
    {
        size_t k = j + 1;
        for (; k < stringLength; ++k)
        {
            if (str[k] == ' ')
            {
                break;
            }
        }
        char text[20];
        for (size_t l = 0; l < k - j; ++l)
        {
            text[l] = str[j + l];
        }
        text[k - j] = '\0';
        v[element] = (float)atof(text);

        if (element == (stride - 1))
        {
            //we've got all the elements of our vertex, so store them
            glm::vec3 vect;
            vect.x = v[0];
            vect.y = v[1];
            vect.z = is2element ? 0.0f : v[2];
            array->push_back(vect);
        }

        j = k + 1;
        element = (element + 1) % stride;
    }
}

void C3DModelXML::WriteVertexChunk(const std::string &path, std::string sName, tinyxml2::XMLElement* pXmlModel, std::vector<glm::vec3> *vertices)
{
    uint32_t uSize = 0;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;
    VertexDataWithNormals Vertex;
    float bmin[3], bmax[3];

    bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
    bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

    //read the normals
    std::vector<glm::vec3> *normals = new std::vector<glm::vec3>();
    ParseVectorString(pXmlModel->FirstChildElement("normals")->FirstChild()->ToText()->Value(), normals);

    //read the texture coords
    std::vector<glm::vec3> *diffuseUVs = new std::vector<glm::vec3>();
    int         diffuseTextureIndex = -1;
    tinyxml2::XMLElement* pXmlCurMaterial = pXmlModel->FirstChildElement("material");

    while (pXmlCurMaterial != NULL)
    {
        if (pXmlCurMaterial->Attribute("name", "diffuse"))
        {
            pXmlCurMaterial->FirstChildElement("texture")->
                QueryIntAttribute("index", &diffuseTextureIndex);
            if (diffuseTextureIndex > -1)
            {
                ParseVectorString(pXmlCurMaterial->FirstChildElement("texture")->
                    FirstChild()->ToText()->Value(), diffuseUVs, true);
            }
        }
        pXmlCurMaterial = pXmlCurMaterial->NextSiblingElement("material");
    }

    Data.reserve(sizeof(VertexDataWithNormals) * vertices->size());
    if (Data.capacity() < (sizeof(VertexDataWithNormals) * vertices->size()))
    {
        LOG_ERROR("Ran out of memory while exporting vertices from '%s' model.", path.c_str());
        return;
    }

    if (vertices->size() != normals->size())
    {
        LOG_ERROR("The model does not have a consistent number of normals to vertices.", path.c_str());
        return;
    }
    if ((vertices->size() != diffuseUVs->size()) && (diffuseTextureIndex > -1))
    {
        LOG_ERROR("The model does not have a consistent number of UVs to vertices.", path.c_str());
        return;
    }
    for (size_t vertexIndex = 0; vertexIndex < vertices->size(); ++vertexIndex)
    {
        Vertex.position.x = m_bFlipOnX ? -vertices->at(vertexIndex).x : vertices->at(vertexIndex).x;
        Vertex.position.y = m_bFlipOnY ? -vertices->at(vertexIndex).y : vertices->at(vertexIndex).y;
        Vertex.position.z = m_bFlipOnZ ? -vertices->at(vertexIndex).z : vertices->at(vertexIndex).z;

        Vertex.normal.x = m_bFlipOnX ? -normals->at(vertexIndex).x : normals->at(vertexIndex).x;
        Vertex.normal.y = m_bFlipOnY ? -normals->at(vertexIndex).y : normals->at(vertexIndex).y;
        Vertex.normal.z = m_bFlipOnZ ? -normals->at(vertexIndex).z : normals->at(vertexIndex).z;

        if (diffuseTextureIndex > -1)
        {
            Vertex.texcoord.x = diffuseUVs->at(vertexIndex).x;
            Vertex.texcoord.y = diffuseUVs->at(vertexIndex).y;
        }

        bmin[0] = std::min(Vertex.position.x, bmin[0]);
        bmin[1] = std::min(Vertex.position.y, bmin[1]);
        bmin[2] = std::min(Vertex.position.z, bmin[2]);
        bmax[0] = std::max(Vertex.position.x, bmax[0]);
        bmax[1] = std::max(Vertex.position.y, bmax[1]);
        bmax[2] = std::max(Vertex.position.z, bmax[2]);

        WRITE_VALUE(Vertex);
    }

#if STU_EXPORT_SEQUENTIAL
    std::string sSTUPath = path;
    sSTUPath.append(".stu");
    m_Export.AppendChunkToFile(sSTUPath, sName, &Data.at(0), (uint32_t)Data.size());
#else
    Export.WriteChunk(sName, &Data.at(0), Data.size());
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
    m_Export.AppendChunkToFile(sSTUPath, sName + "BB", &Data.at(0), (uint32_t)Data.size());
#else
    Export.WriteChunk(sName + "BB", &Data.at(0), Data.size());
#endif
    std::vector< uint8_t >().swap(Data);
}

bool C3DModelXML::ExportToSTUFormat(const std::string &path)
{
    m_uSubModelCount = 0;
    m_uSubModelVertexCount = 0;
    return ExportToSTUFormat(path, true, false, false, false, false);
}


bool C3DModelXML::ExportToSTUFormat(const std::string &path, bool bFlipUV)
{
    return ExportToSTUFormat(path, bFlipUV, false, false, false, false);
}

bool C3DModelXML::ExportToSTUFormat(const std::string &path, bool bFlipUV, bool bLoadCollsionModel, bool bFlipOnX, bool bFlipOnY, bool bFlipOnZ)
{
    uint32_t NumVertices = 0;
    uint32_t uSize = 0;
    uint32_t uValue;
    uint8_t bytes[128] = { 0 };
    std::vector< uint8_t > Data;
    int textureCount = 0;
    int modelCount = 0;

    m_bFlipOnX = bFlipOnX;
    m_bFlipOnY = bFlipOnY;
    m_bFlipOnZ = bFlipOnZ;
    m_uSubModelCount = 0;
    m_uSubModelVertexCount = 0;
    m_bFlipUVonY = bFlipUV;
    m_Entries.clear();
    m_TotalMeshCount = 0;

    if (m_pXmlDocument->LoadFile(path.c_str()) != 0)
    {
        LOG_ERROR("Could not load '%s' model.", path.c_str());
        return false;
    }

    std::string MaterialPath = path.substr(0, path.find_last_of("/") + 1);

    // Load the textures
    tinyxml2::XMLElement* pXmlTexture = m_pXmlDocument->FirstChildElement("scene")->FirstChildElement("textures");
    if (pXmlTexture)
    {
        pXmlTexture->QueryIntAttribute("count", &textureCount);
        pXmlTexture = pXmlTexture->FirstChildElement("texture");
    }

    for (int i = 0; i < textureCount; ++i)
    {
        const char* textureName = pXmlTexture->Attribute("fileName");
        m_Textures.push_back(textureName);
        pXmlTexture = pXmlTexture->NextSiblingElement("texture");
    }

    // Load the models
    m_pXmlDocument->FirstChildElement("scene")->FirstChildElement("models")->QueryIntAttribute("count", &modelCount);

    tinyxml2::XMLElement* pXmlModel = m_pXmlDocument->FirstChildElement("scene")->FirstChildElement("models")->FirstChildElement("model");
    for (int i = 0; i < modelCount; ++i)
    {
        Data.clear();
        bool isCollisionModel = false;
        pXmlModel->QueryBoolAttribute("isCollisionModel", &isCollisionModel);

        if (isCollisionModel && !bLoadCollsionModel)
        {
            continue;
        }
        const char* name = pXmlModel->Attribute("name");
        
        std::string nodeName;
        if (strlen(name) > 0)
        {
            nodeName = std::string("XML.(") + name + "-" + std::to_string(m_uUniqueOBJUnknownID++) + ")";
        }
        else
        {
            nodeName = std::string("XML.(UNKNOWN-") + std::to_string(m_uUniqueOBJUnknownID++) + ")";
        }
        LOG_INFO("Found node '%s'\n", nodeName.c_str());

        glm::mat4 matrix;

        uValue = 0; //Always 0  Children
        WRITE_VALUE(uValue);

        uSize += CFileExportSTUFormat::CopyString(nodeName.c_str(), &Data);

        uSize += CFileExportSTUFormat::CopyString(name, &Data);

        // node transformation matrix is identity
        WRITE_VALUE(matrix);

        uValue = 1; //Always 1 mesh
        WRITE_VALUE(uValue);

        std::string meshName;
        static uint32_t sUniqueOBJUnknownMeshID = 0;
        if (strlen(name) > 0)
        {
            meshName = nodeName + ".mesh(" + name + "-" + std::to_string(sUniqueOBJUnknownMeshID++) + ")";
        }
        else
        {
            meshName = nodeName + ".mesh(UNKNOWN-" + std::to_string(sUniqueOBJUnknownMeshID++) + ")";
        }
        std::string VBOName = meshName + ".VBO";
        std::string IBOName = meshName + ".IBO";

        uSize += CFileExportSTUFormat::CopyString(meshName.c_str(), &Data);

        uSize += CFileExportSTUFormat::CopyString(name, &Data);

        uValue = 0; //Always no animation
        WRITE_VALUE(uValue);

        //read the vertices
        std::vector<glm::vec3> *vertices = new std::vector<glm::vec3>();
        ParseVectorString(pXmlModel->FirstChildElement("vertices")->FirstChild()->ToText()->Value(), vertices);

        uValue = (uint32_t)vertices->size();
        WRITE_VALUE(uValue);

        uValue = VertexDataType_Normals;
        WRITE_VALUE(uValue);

        std::string sVertexChunkname = std::string("Vx:") + std::to_string(m_uSubModelVertexCount++);
        WriteVertexChunk(path, sVertexChunkname, pXmlModel, vertices);

        std::vector<uint16_t> indices;

        // Read the vertex indices for the triangles
        const char* indexStr = pXmlModel->FirstChildElement("indices")->FirstChild()->ToText()->Value();

        size_t stringLength = strlen(indexStr);

        for (size_t j = 0; j < stringLength;)
        {
            size_t k = j + 1;
            for (; k < stringLength; ++k)
            {
                if (indexStr[k] == ' ')
                    break;
            }
            char text[20];
            for (size_t l = 0; l < k - j; ++l)
            {
                text[l] = indexStr[j + l];
            }
            text[k - j] = '\0';

            indices.push_back((unsigned short)atoi(text));
            j = k + 1;
        }

        uValue = (uint32_t)indices.size();
        WRITE_VALUE(uValue);
        if (uValue)
        {
            WRITE_VALUES(indices[0], (uint32_t)indices.size());
        }

        uValue = PrimitiveType_TRIANGLE;
        WRITE_VALUE(uValue);

        uValue = 0; //Always 2 sides
        WRITE_VALUE(uValue);

        glm::vec3 AmbientColor(0.25f);
        glm::vec3 DiffuseColor(1.0f);
        glm::vec3 SpecularColor(1.0f);
        float fShininess = 8.0f;
        float fAlpha = 1.0f;

        WRITE_VALUE(AmbientColor);
        WRITE_VALUE(DiffuseColor);
        WRITE_VALUE(SpecularColor);
        WRITE_VALUE(fShininess);
        WRITE_VALUE(fAlpha);

        int         diffuseTextureIndex = -1;
        tinyxml2::XMLElement* pXmlCurMaterial = pXmlModel->FirstChildElement("material");

        while (pXmlCurMaterial != NULL)
        {
            if (pXmlCurMaterial->Attribute("name", "diffuse"))
            {
                pXmlCurMaterial->FirstChildElement("texture")->
                    QueryIntAttribute("index", &diffuseTextureIndex);
            }
            pXmlCurMaterial = pXmlCurMaterial->NextSiblingElement("material");
        }
        if (m_Textures.size() > 0 && diffuseTextureIndex == -1)
        {
            diffuseTextureIndex = 0;
        }
        if (diffuseTextureIndex > -1)
        {
            std::string sFullfilename = m_Textures[diffuseTextureIndex];
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
        }
        else
        {
            uValue = TextureType_COLOR_DIFFUSE;
            WRITE_VALUE(uValue);
            WRITE_VALUE(fAlpha);
        }

        uValue = TextureType_UNKNOWN;   //Skip normal map for now.
        WRITE_VALUE(uValue);

        std::string sChunkname = std::string("Model:") + std::to_string(m_uSubModelCount++);

#if STU_EXPORT_SEQUENTIAL
        std::string sSTUPath = path;
        sSTUPath.append(".stu");
        m_Export.AppendChunkToFile(sSTUPath, sChunkname, &Data.at(0), (uint32_t)Data.size());
#else
        m_Export.WriteChunk(sChunkname, &Data.at(0), (uint32_t)Data.size());
#endif
        pXmlModel = pXmlModel->NextSiblingElement("model");
    }

#if !STU_EXPORT_SEQUENTIAL
    std::string sSTUPath = path;
    sSTUPath.Append(".stu");
    m_Export.ExportFile(sSTUPath);
#endif

    return true;
}
