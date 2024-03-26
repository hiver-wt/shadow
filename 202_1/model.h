#include"mesh.h"
#include <map>
class Model
{
public:
    std::vector<Mesh> meshes;
    std::map<std::string, GLuint> textureMap;
    Model(std::string filepath)
    {
        load(filepath);
    }
    void load(std::string filepath)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate);
        // �쳣����
        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "��ȡģ�ͳ��ִ���: " << import.GetErrorString() << std::endl;
            exit(-1);
        }
        // ģ���ļ����·��
        std::string rootPath = filepath.substr(0, filepath.find_last_of('/'));

        // ѭ������ mesh
        for (int i = 0; i < scene->mNumMeshes; i++)
        {
            // ���õ�ǰmesh
            meshes.push_back(Mesh());
            Mesh& mesh = meshes.back();

            // ��ȡ assimp �Ķ�ȡ���� aimesh ����
            aiMesh* aimesh = scene->mMeshes[i];

            // ���ǽ����ݴ��ݸ������Զ����mesh
            for (int j = 0; j < aimesh->mNumVertices; j++)
            {
                // ����
                glm::vec3 vvv;
                vvv.x = aimesh->mVertices[j].x;
                vvv.y = aimesh->mVertices[j].y;
                vvv.z = aimesh->mVertices[j].z;
                mesh.vertexPosition.push_back(vvv);

                // ����
                vvv.x = aimesh->mNormals[j].x;
                vvv.y = aimesh->mNormals[j].y;
                vvv.z = aimesh->mNormals[j].z;
                mesh.vertexNormal.push_back(vvv);

                // ��������: �����������롣assimp Ĭ�Ͽ����ж���������� ����ȡ��һ����0������
                glm::vec2 vv(0, 0);
                if (aimesh->mTextureCoords[0])
                {
                    vv.x = aimesh->mTextureCoords[0][j].x;
                    vv.y = aimesh->mTextureCoords[0][j].y;
                }
                mesh.vertexTexcoord.push_back(vv);
            }

            // ����в��ʣ���ô���ݲ���
            if (aimesh->mMaterialIndex >= 0)
            {
                // ��ȡ��ǰ aimesh �Ĳ��ʶ���
                aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];

                // ��ȡ diffuse ��ͼ�ļ�·������ ����ֻȡ1����ͼ ���� 0 ����
                aiString aistr;
                if (material->GetTextureCount(aiTextureType_DIFFUSE) == 0)
                {
                    GLuint tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    GLbyte color[3] = {125 , 125 ,125};
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, color);
                    mesh.diffuseTexture = tex;
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                else
                {
                    material->GetTexture(aiTextureType_DIFFUSE, 0, &aistr);
                    std::string texpath = aistr.C_Str();
                    texpath = rootPath + '/' + texpath;   // ȡ���·��

                    // ���û���ɹ�������ô������
                    if (textureMap.find(texpath) == textureMap.end())
                    {
                        // ��������
                        GLuint tex;
                        glGenTextures(1, &tex);
                        glBindTexture(GL_TEXTURE_2D, tex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                        int textureWidth, textureHeight, nrComponents;

                        unsigned char* data = stbi_load(texpath.c_str(), &textureWidth, &textureHeight, &nrComponents, 0);
                        if (data)
                        {
                            GLenum format;
                            if (nrComponents == 1)
                                format = GL_RED;
                            else if (nrComponents == 3)
                                format = GL_RGB;
                            else if (nrComponents == 4)
                                format = GL_RGBA;
                            glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, data);
                            stbi_image_free(data);
                        }
                        textureMap[texpath] = tex;
                    }

                    // ��������
                    mesh.diffuseTexture = textureMap[texpath];
                }
            }

            // ������Ƭ����
            for (GLuint j = 0; j < aimesh->mNumFaces; j++)
            {
                aiFace face = aimesh->mFaces[j];
                for (GLuint k = 0; k < face.mNumIndices; k++)
                {
                    mesh.index.push_back(face.mIndices[k]);
                }
            }

            mesh.bindData();
        }
    }
    void draw(Shader& shader)
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i].draw(shader);
        }
    }
};
