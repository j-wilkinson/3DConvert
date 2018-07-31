#include "CFileExportSTUFormat.h"

#include <algorithm>

#define LOG_ERROR(...) printf("C3DModelExport:"); printf(__VA_ARGS__);
#define LOG_INFO(...) printf("C3DModelExport:"); printf(__VA_ARGS__);

#define YI_FLAG_COMPRESS_OUTPUT 0x100

unsigned char CFileExportSTUFormat::m_ucMagic[] = "STU";        //3 Char limit
unsigned char CFileExportSTUFormat::m_ucVersion[] = "0.1";      //3 Char limit

//For this version of this file
static unsigned char gMaxVersionSupported[] = "0.1";

bool CFileExportSTUFormat::EndsWithIgnoreCase(std::string fullString, std::string ending)
{
    std::transform(fullString.begin(), fullString.end(), fullString.begin(), ::tolower);
    std::transform(ending.begin(), ending.end(), ending.begin(), ::tolower);
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else {
        return false;
    }
}

uint32_t CFileExportSTUFormat::CopyString(const char* pString, std::vector< uint8_t > * Target)
{
    const char * pCurrentChar = pString;
    uint32_t nCount = 0;
    while (*pCurrentChar != 0 && nCount < CFileExportSTUFormat::uMaxTextLength)
    {
        Target->insert(Target->end(), pCurrentChar, pCurrentChar + 1);
        pCurrentChar++;
        nCount++;
    };
    if (nCount == CFileExportSTUFormat::uMaxTextLength)
    {
        fprintf(stderr, "CFileExportSTUFormat: String Exceeds uMaxTextLength: %s", pString);
    }
    Target->push_back(0);
    nCount++;

    return nCount;
}

std::string CFileExportSTUFormat::RemoveFoldersFromPaths(const std::string &sPath)
{
    std::string sFile = sPath;
    std::size_t index = sPath.find_last_of('\\');
    if (index != std::string::npos)
    {
        //std::string sDirectory = sPath.substr(0, index);
        sFile = sPath.substr(index + 1);
        /*
        //SR: This does not work since you can't append a fallback directory
        // We still do this since we can migrate the files to the root of the 3D object and it will work as a workaround.

        CYIAssetLocator assetLocator = CYIAssetLoader::GetAssetLocator();
        assetLocator.AddFallbackDirectory(assetLocator.GetBase() + sDirectory);
        assetLocator.Refresh();
        CYIAssetLoader::SetAssetLocator(assetLocator);
        */
    }
    else
    {
        index = sPath.find_last_of('/');
        if (index != std::string::npos)
        {
            //std::string sDirectory = sPath.substr(0, index);
            sFile = sPath.substr(index + 1);
            /*
            //SR: This does not work since you can't append a fallback directory
            // We still do this since we can migrate the files to the root of the 3D object and it will work as a workaround.

            CYIAssetLocator assetLocator = CYIAssetLoader::GetAssetLocator();
            assetLocator.AddFallbackDirectory(assetLocator.GetBase() + sDirectory);
            assetLocator.Refresh();
            CYIAssetLoader::SetAssetLocator(assetLocator);
            */
        }
    }
    return sFile;
}

CFileExportSTUFormat::CFileExportSTUFormat()
{
    m_Buffer.clear();
    m_sVersion = m_ucVersion;
}

CFileExportSTUFormat::~CFileExportSTUFormat()
{
    std::vector< STU_CHUNK *>::iterator it = m_Buffer.begin();
    for (; it != m_Buffer.end(); ++it)
    {
        delete (*it);
    }
    m_Buffer.clear();
}

uint32_t CFileExportSTUFormat::MakeHashFromName(const std::string &sName)
{
#if 01
    const char * str = sName.c_str();
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash * 33) + hash) + c;

    return hash;
#else
    std::tr1::hash<std::string> hash_fn;
    return (uint32_t)hash_fn(sName);
#endif
}

bool CFileExportSTUFormat::AppendChunkToFile(const std::string &path, const std::string &sName, void * pData, uint32_t uLength)
{
    uint32_t uSize = uLength;
    uint32_t uRemainder = 0;
    uint32_t nRealSize = 0;

    bool bResult = false;
    STU_FILE_HEADER FileHeader;

    const char * pPath = path.c_str();
    if (!pPath)
    {
        LOG_ERROR("No filename given.");
        return bResult;
    }

    if (uLength == 0)
    {
        LOG_ERROR("Nothing to write!");
        return bResult;
    }

    FILE * fp = fopen(path.c_str(), "rb");
    if (fp)
    {
        //Read file header
        if (fread(&FileHeader, sizeof(unsigned char), sizeof(FileHeader), fp) != sizeof(FileHeader))
        {
            LOG_ERROR("Cannot read file header.");
            return bResult;
        }
        //Check Magic Bytes
        if (FileHeader.Magic[0] != m_ucMagic[0] || FileHeader.Magic[1] != m_ucMagic[1] || FileHeader.Magic[2] != m_ucMagic[2])
        {
            LOG_ERROR("File magic bytes do not match.");
            return bResult;
        }
        m_sVersion = FileHeader.Version;
        if (FileHeader.Version[0] != gMaxVersionSupported[0] || FileHeader.Version[1] != gMaxVersionSupported[1] || FileHeader.Version[2] != gMaxVersionSupported[2])
        {
            LOG_ERROR("File version is newer than supported by this application.");
            return bResult;
        }

        //Get Filesize info
        uSize = (FileHeader.FileSizeInfo[0] << 24) | (FileHeader.FileSizeInfo[1] << 16) | (FileHeader.FileSizeInfo[2] << 8) | (FileHeader.FileSizeInfo[3]);

        //Check Filesize info - this can be skipped for performance.
        fseek(fp, 0, SEEK_END);
        nRealSize = ftell(fp);
        fseek(fp, sizeof(FileHeader), SEEK_SET);
        if (uSize != (nRealSize - sizeof(FileHeader)))
        {
            LOG_ERROR("File size info does not match.");
            return bResult;
        }
        fclose(fp);
    }
    else
    {
        nRealSize = sizeof(FileHeader);
        uSize = 0;
    }

    //Create new chunk for saving
    STU_CHUNK * Chunk = new STU_CHUNK;
    uint32_t uUsed;

    strncpy(Chunk->Header.Name, sName.c_str(), 19);

    uUsed = uLength;

    Chunk->Header.ChunkSizeInfo[0] = uUsed >> 24;
    uRemainder = uUsed - (Chunk->Header.ChunkSizeInfo[0] << 24);
    Chunk->Header.ChunkSizeInfo[1] = (uRemainder >> 16);
    uRemainder -= (Chunk->Header.ChunkSizeInfo[0] << 16);
    Chunk->Header.ChunkSizeInfo[2] = (uRemainder >> 8);
    uRemainder -= (Chunk->Header.ChunkSizeInfo[0] << 8);
    Chunk->Header.ChunkSizeInfo[3] = uRemainder;
    Chunk->Header.NameHash = MakeHashFromName(Chunk->Header.Name);

    //Update the file heading info
    uSize += sizeof(STU_HEADER);
    uSize += uLength;
    
    FileHeader.FileSizeInfo[0] = uSize >> 24;
    uRemainder = uSize - (FileHeader.FileSizeInfo[0] << 24);
    FileHeader.FileSizeInfo[1] = (uRemainder >> 16);
    uRemainder -= (FileHeader.FileSizeInfo[0] << 16);
    FileHeader.FileSizeInfo[2] = (uRemainder >> 8);
    uRemainder -= (FileHeader.FileSizeInfo[0] << 8);
    FileHeader.FileSizeInfo[3] = uRemainder;

    fp = fopen(path.c_str(), "r+b");
    if (!fp)
    {
        fp = fopen(path.c_str(), "wb");
        if (!fp)
        {
            LOG_ERROR("Could not open file.");
            return bResult;
        }
    }

    //Write updated file header
    if (fwrite(&FileHeader, sizeof(unsigned char), sizeof(FileHeader), fp) != sizeof(FileHeader))
    {
        LOG_ERROR("Cannot write file header.");
        return bResult;
    }

    //Add new data
    fseek(fp, nRealSize, SEEK_SET);

    if (fwrite(&Chunk->Header, sizeof(unsigned char), sizeof(Chunk->Header), fp) != sizeof(Chunk->Header))
    {
        LOG_ERROR("Cannot write chunk header.");
        return bResult;
    }
    if (fwrite(pData, sizeof(unsigned char), uLength, fp) != uLength)
    {
        LOG_ERROR("Cannot write chunk data.");
        return bResult;
    }

    fclose(fp);

    bResult = true;
    return bResult;
}

bool CFileExportSTUFormat::ExportFile(const std::string &path)
{
    bool bResult = false;
    STU_FILE_HEADER FileHeader;

    const char * pPath = path.c_str();
    if (!pPath)
    {
        LOG_ERROR("No filename given.");
        return bResult;
    }

    uint32_t uSize = 0;
    uint32_t uRemainder = 0;
    std::vector< STU_CHUNK *>::iterator Itr = m_Buffer.begin();
    std::vector< STU_CHUNK *>::iterator End = m_Buffer.end();
    while (Itr != End)
    {
        uSize += sizeof(STU_HEADER);
        uSize += (*Itr)->GetDataSize();
        Itr++;
    }
    if (uSize == 0)
    {
        LOG_ERROR("Nothing to write!");
        return bResult;
    }
    FileHeader.FileSizeInfo[0] = uSize >> 24;
    uRemainder = uSize - (FileHeader.FileSizeInfo[0] << 24);
    FileHeader.FileSizeInfo[1] = (uRemainder >> 16);
    uRemainder -= (FileHeader.FileSizeInfo[0] << 16);
    FileHeader.FileSizeInfo[2] = (uRemainder >> 8);
    uRemainder -= (FileHeader.FileSizeInfo[0] << 8);
    FileHeader.FileSizeInfo[3] = uRemainder;

    FILE * fp = fopen(path.c_str(), "wb");
    if (!fp)
    {
        LOG_ERROR("Could not open file.");
        return bResult;
    }
    //Read file header
    if (fwrite(&FileHeader, sizeof(unsigned char), sizeof(FileHeader), fp) != sizeof(FileHeader))
    {
        LOG_ERROR("Cannot write file header.");
        return bResult;
    }

    Itr = m_Buffer.begin();
    while (Itr != End)
    {
        if (fwrite(&(*Itr)->Header, sizeof(unsigned char), sizeof((*Itr)->Header), fp) != sizeof((*Itr)->Header))
        {
            LOG_ERROR("Cannot write chunk header.");
            return bResult;
        }
        if (fwrite((void*)((*Itr)->GetData()), sizeof(unsigned char), (*Itr)->GetDataSize(), fp) != (*Itr)->GetDataSize())
        {
            LOG_ERROR("Cannot write chunk data.");
            return bResult;
        }
        Itr++;
    }

    fclose(fp);

    bResult = true;
    return bResult;
}

bool CFileExportSTUFormat::WriteChunk(const std::string &sName, void * pData, uint32_t uLength)
{
    bool bResult = false;

    STU_CHUNK * Chunk = new STU_CHUNK;
    uint32_t uUsed;

    uint32_t uRemainder;
    strncpy(Chunk->Header.Name, sName.c_str(), 19);
    Chunk->Header.NameHash = MakeHashFromName(Chunk->Header.Name);

    uUsed = uLength;

    Chunk->Header.ChunkSizeInfo[0] = uUsed >> 24;
    uRemainder = uUsed - (Chunk->Header.ChunkSizeInfo[0] << 24);
    Chunk->Header.ChunkSizeInfo[1] = (uRemainder >> 16);
    uRemainder -= (Chunk->Header.ChunkSizeInfo[0] << 16);
    Chunk->Header.ChunkSizeInfo[2] = (uRemainder >> 8);
    uRemainder -= (Chunk->Header.ChunkSizeInfo[0] << 8);
    Chunk->Header.ChunkSizeInfo[3] = uRemainder;

    Chunk->AllocateForData(uLength);
    Chunk->SetData((uint8_t *)pData, uLength);

    m_Buffer.insert(m_Buffer.end(), Chunk);

    bResult = true;

    return bResult;
}
