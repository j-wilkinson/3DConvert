#ifndef _YES_3D_MODEL_XML
#define _YES_3D_MODEL_XML

#include "CFileExportSTUFormat.h"
#include "C3DModelDataStructures.h"
#include "tinyxml2.h"

class C3DModelXML
{
public:
    C3DModelXML();
    virtual ~C3DModelXML();

    bool ExportToSTUFormat(const std::string &path);
    bool ExportToSTUFormat(const std::string &path, bool bFlipUV = true);
    bool ExportToSTUFormat(const std::string &path, bool bFlipUV, bool bLoadCollsionModel, bool bFlipOnX, bool bFlipOnY, bool bFlipOnZ);
    void SetDefaultSolidColor(float fRed, float fGreen, float fBlue) { m_SolidColor[0] = fRed; m_SolidColor[1] = fGreen;  m_SolidColor[2] = fBlue; }

private:

    void ParseVectorString(const char* str, std::vector<glm::vec3> *array, bool is2element = false);
    void WriteVertexChunk(const std::string &path, std::string sName, tinyxml2::XMLElement* pXmlModel, std::vector<glm::vec3> *vertices);
    void ExportSceneTree();

    struct MeshEntry {
        MeshEntry()
        {
            uBaseVertex = 0;
        }
        uint32_t uBaseVertex;
    };
    uint32_t m_TotalMeshCount;
    uint32_t m_uSubModelCount;
    uint32_t m_uSubModelVertexCount;

    std::vector<MeshEntry> m_Entries;

    CFileExportSTUFormat m_Export;
    uint32_t m_uUniqueOBJUnknownID;
    float m_SolidColor[3];
    tinyxml2::XMLDocument * m_pXmlDocument;
    std::vector<std::string> m_Textures;
    bool m_bFlipUVonY;
    bool m_bFlipOnX;
    bool m_bFlipOnY;
    bool m_bFlipOnZ;
};

#endif // _YES_3D_MODEL_XML