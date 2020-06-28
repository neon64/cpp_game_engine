#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "errors.h"
#include "Window.h"
#include "OpenGLContext.h"
#include "pipeline.h"
#include "commands.h"

using namespace std;

bool import_model(const std::string& filepath) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);

    if(!scene) {
        cout << importer.GetErrorString() << endl;
        return false;
    }

    for(int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *material = scene->mMaterials[i];
        cout << "found material: " << material->GetName().C_Str() << endl;
        for(int j = 0; j < material->mNumProperties; j++) {
            aiMaterialProperty *prop = material->mProperties[j];
            aiString *key = &prop->mKey;
            if(prop->mType == aiPropertyTypeInfo::aiPTI_String) {
                cout << key->C_Str() << " = " << ((aiString *) prop->mData)->C_Str() << endl;
            } else {
                cout << key->C_Str() << " = " << prop->mType << " " << prop->mSemantic << endl;
            }

        }
    }

    for(int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[i];
        cout << "found mesh: " << mesh->mName.C_Str() << endl;
        cout << "vertices: " << mesh->mNumVertices << endl;
    }


    return true;
}

int main() {
    Window window;

    OpenGLContext context(window);

    {
        auto vertexShader = Shader::build(ShaderType::VERTEX, "awesome new shader", "foovoid main() {}");
        auto fragmentShader = Shader::build(ShaderType::FRAGMENT, "awesome new fragment shader", "void main() {}");
//    Program p = Program::build({ std::move(v), std::move(f) });

        GraphicsPipelineCreateInfo pipelineCreateInfo = {
                .shaders = {
                        .vertex = vertexShader,
                        .fragment = fragmentShader
                },
                .vertexInput = {
                },
                .inputAssembly {
                        .topology = PrimitiveTopology::TRIANGLES
                },
                .rasterizer {
                },
                .depthStencil {
                        .depthTest = nullopt,
                        .depthMask = true
                },
                .colorBlend {
                },
        };

        shared_ptr<GraphicsPipeline> pipeline = context.buildPipeline(pipelineCreateInfo);

        while(!window.shouldClose()) {
            // run loop
            context.submit(ClearCommand(ColorRGBA(1.0, 0.0, 1.0, 1.0), 1.0f));

            context.submit({ .pipeline = pipeline });

            window.swapBuffers();
            Window::pollEvents();
        }
    }

    glfwTerminate();

    //import_model("/home/chris/code/java/GameEngine04/res/models/stanfordBunny/mesh.obj");

    // std::cout << "Hello, World!" << std::endl;

    return 0;
}
