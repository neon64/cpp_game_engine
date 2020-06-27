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
#include "Program.h"
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

    Shader v = Shader::build(GL_VERTEX_SHADER, "awesome new shader", "foovoid main() {}");
    Shader f = Shader::build(GL_FRAGMENT_SHADER, "awesome new fragment shader", "void main() {}");
    Program p = Program::build({ std::move(v), std::move(f) });

    while(!window.shouldClose()) {
        // run loop
        context.submit(ClearCommand::clearColorAndDepth(ColorRGBA(1.0, 0.0, 1.0, 1.0), 1.0f));

        window.swapBuffers();
        Window::pollEvents();
    }

    glfwTerminate();

    import_model("/home/chris/code/java/GameEngine04/res/models/stanfordBunny/mesh.obj");

    std::cout << "Hello, World!" << std::endl;
    return 0;
}
