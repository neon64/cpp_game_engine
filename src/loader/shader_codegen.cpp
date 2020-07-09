#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cassert>
#include <memory>
#include <fmt/format.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/reflection.h>

#include "shader_resource_limits.h"
#include "glsl_to_cpp.h"

using namespace fmt::v6;

using namespace std;
using namespace std::filesystem;

const char *get_shader_type(EShLanguage shaderStage) {
    if(shaderStage == EShLangVertex) {
        return "VERTEX_SHADER";
    } else if(shaderStage == EShLangFragment) {
        return "FRAGMENT_SHADER";
    }
    return "UNKNOWN";
}

EShLanguage get_shader_stage(const path& extension) {
    if (extension == ".vert") {
        return EShLangVertex;
    } else if (extension == ".tesc") {
        return EShLangTessControl;
    } else if (extension == ".tese") {
        return EShLangTessEvaluation;
    } else if (extension == ".geom") {
        return EShLangGeometry;
    } else if (extension == ".frag") {
        return EShLangFragment;
    } else if (extension == ".comp") {
        return EShLangCompute;
    } else {
        assert(0 && "Unknown shader stage");
        return EShLangCount;
    }
}

struct Shader {
    path p;
    string source;
};

int main(int argc, char *argv[]) {
    if(argc < 4) {
        cerr << "not enough arguments, expecting: [name] [input_file_0] [input_file_1] [input_file_2] ... [output_file]" << endl;
        return 1;
    }

    string name = argv[1];
    // std::transform(name.begin(), name.end(), name.begin(), ::toupper);

    vector<Shader> names;
    for(int i = 2; i < argc - 1; i++) {
        path p(argv[i]);
        //cout << "read path " << p << endl;
        names.push_back({
            .p = p,
            .source = string("")
        });
    }

    path output(argv[argc - 1]);

    cout << "opening output file " << output << endl;
    ofstream out;
    out.open(output.replace_extension(".h"));

    ofstream out_impl;
    out_impl.open(output.replace_extension(".cpp"));

    out_impl << "#include " << output.filename().replace_extension(".h") << "\n";

    out << "#pragma once\n";
    out << "// autogenerated from GLSL, do not edit\n";
    out << "#include <glm/glm.hpp>\n";
    out << "#include \"../../src/OpenGLContext.h\"\n";
    out << "#include \"../../src/commands.h\"\n";
    out << "#include \"../../src/texturing.h\"\n";
    out << "namespace pipelines { namespace " << name << " {\n";
    out_impl << "namespace pipelines { namespace " << name << " {\n";

    glslang::InitializeProcess();

    vector<unique_ptr<glslang::TShader>> shaders;

    for(auto& input : names) {
        cout << "reading shader source from " << input.p << endl;
        ifstream in(input.p);
        in.exceptions ( ifstream::failbit | ifstream::badbit );

        in.seekg(0, ios::end);
        input.source.reserve(in.tellg());
        in.seekg(0, ios::beg);

        input.source.assign((istreambuf_iterator<char>(in)),
                   istreambuf_iterator<char>());

        EShLanguage shaderStage = get_shader_stage(input.p.extension());
        const char *ty = get_shader_type(shaderStage);

        const int clientVersion = 0;

        unique_ptr<glslang::TShader> shader = make_unique<glslang::TShader>(shaderStage);

        const char *s = input.source.c_str();
        shader->setStrings(&s, 1);
        shader->setEnvInput(glslang::EShSourceGlsl, shaderStage, glslang::EShClientNone, glslang::EShTargetClientVersion(clientVersion));
        shader->setEnvClient(glslang::EShClientNone, glslang::EShTargetClientVersion(clientVersion));
        shader->setEnvTarget(glslang::EShTargetNone, glslang::EShTargetLanguageVersion(0));
        // shaders.back().setAutoMapBindings(true);
        TBuiltInResource resources = DefaultTBuiltInResource;
        EShMessages messages = EShMsgDefault;
        glslang::TShader::ForbidIncluder includer;
        bool result = shader->parse(&resources, clientVersion, false, messages, includer);
        cout << "compilation for " << input.p << (result ? " successful\n" : " failed\n") << "info log: " <<  shader->getInfoLog() << endl;

        if(!result) {
            return 1;
        }

        shaders.push_back(std::move(shader));

        out << "extern const char* " << ty << ";\n";
        out_impl << "const char* " << ty <<  " = R\"\"(\n" << input.source << ")\"\";\n";

        in.close();
    }

    glslang::TProgram *program = new glslang::TProgram();
    for(auto& shader : shaders) {
        program->addShader(&*shader);
    }

    bool result = program->link(EShMsgDefault);
    cout << "linking status: " << result << ", info log: " << program->getInfoLog() << endl;
    result = program->buildReflection(EShReflectionAllBlockVariables | EShReflectionAllIOVariables);
    cout << "build reflection status: " << result << endl;

    vector<string> defs;

    vector<Field> uniformBlocks;
    // uniform blocks
    for(int i = 0; i < program->getNumUniformBlocks(); i++) {
        const glslang::TObjectReflection& block = program->getUniformBlock(i);

        Type t = get_type(block.getType(), defs);

        string blockName(program->getUniformBlockName(i));
        blockName[0] = tolower(blockName[0]);

        Field f = {
            .name = blockName,
            .type = Type {
                .base = format("const UniformBufferBinding<{}>", t.base),
                .numElements = t.numElements
            },
            .originalType = block.getType(),
            .location = make_optional<int>(block.getBinding())
        };

        uniformBlocks.push_back(f);
    }

    // all non-block uniforms should be texture samplers.
    vector<Field> textures;
    for(int i = 0; i < program->getNumUniformVariables(); i++) {
        const glslang::TObjectReflection& uniform = program->getUniform(i);

        // uniforms with an offset are already part of a uniform block
        if(uniform.offset != -1) {
            continue;
        }

        optional<Field> t = Field::create_from_sampler(uniform);
        if(t.has_value()) {
            textures.push_back(t.value());
        } else {
            cerr << "unsupported top-level uniform `" << uniform.name << "` - uniform blocks should be used instead" << endl;
        }
    }

    vector<Field> vertexInputs;
    vector<Field> instanceInputs;

    // vertex and instance inputs
    for(int i = 0; i < program->getNumPipeInputs(); i++) {
        const glslang::TObjectReflection& input = program->getPipeInput(i);

        optional<Field> field = Field::create_from_pipe_input(input, defs);
        if(field.has_value()) {
            Field f = field.value();
            constexpr std::string_view prefix = "vertex";
            if(f.name.starts_with(prefix) && f.name.size() > prefix.size()) {
                f.name = f.name.substr(prefix.size());
                f.name[0] = tolower(f.name[0]);
                vertexInputs.push_back(f);
            } else {
                instanceInputs.push_back(f);
            }
        }
    }

    for(auto& definition : defs) {
        out << definition;
    }

    const char *VERTEX_INPUT_STRUCT = "VertexInput";
    const char *INSTANCE_INPUT_STRUCT = "InstanceInput";
    const char *VERTEX_INPUT_MEMBER_NAME = "perVertex";
    const char *INSTANCE_INPUT_MEMBER_NAME = "perInstance";
    const int VERTEX_INPUT_BINDING = 0;
    const int INSTANCE_INPUT_BINDING = 1;

    vector<Field> vertexBindings;
    emit_struct(VERTEX_INPUT_STRUCT, vertexInputs, [](auto o) {}, out);
    vertexBindings.push_back({ .name = VERTEX_INPUT_MEMBER_NAME, .type = Type { .base = format("const VertexBufferBinding<{}>", VERTEX_INPUT_STRUCT), .numElements = nullopt }, .location = nullopt });
    if(instanceInputs.size() > 0) {
        emit_struct(INSTANCE_INPUT_STRUCT, instanceInputs, [](auto o) {}, out);
        vertexBindings.push_back({ .name = INSTANCE_INPUT_MEMBER_NAME, .type = Type { .base = format("const VertexBufferBinding<{}>", INSTANCE_INPUT_STRUCT), .numElements = nullopt }, .location = nullopt });
    }

    out << "struct VertexBindingPipelineState;\n";
    out << "struct VertexBindingCreateInfo;\n";

    emit_struct("VertexBindings", vertexBindings, [](auto out) {
        *out << "    using CreateInfo = VertexBindingCreateInfo;\n";
        *out << "    using PipelineState = VertexBindingPipelineState;\n";
    }, out);

    out << "struct VertexBindingPipelineState {\n";
    out << "    void bindAll(const VertexBindings& bindings, VertexArray& array, OpenGLContext& context);\n";
    out << "};\n";

    out_impl << "void VertexBindingPipelineState::bindAll(const VertexBindings& bindings, VertexArray& array, OpenGLContext& context) {\n";
    out_impl << "    context.bindVertexBuffer(array, " << VERTEX_INPUT_BINDING << ", bindings." << VERTEX_INPUT_MEMBER_NAME << ".buffer, bindings." << VERTEX_INPUT_MEMBER_NAME << ".offset, sizeof(" << VERTEX_INPUT_STRUCT << "));\n";
    if(instanceInputs.size() > 0) {
        out_impl << "    context.bindVertexBuffer(array, " << INSTANCE_INPUT_BINDING << ", bindings." << INSTANCE_INPUT_MEMBER_NAME
            << ".buffer, bindings." << INSTANCE_INPUT_MEMBER_NAME << ".offset, sizeof(" << INSTANCE_INPUT_STRUCT
            << "));\n";
    }
    out_impl << "}\n";

    out << "struct VertexBindingCreateInfo {\n";
    out << "    VertexBindingPipelineState init(VertexArray& array, OpenGLContext& context);\n";
    out << "};\n";

    out_impl << "VertexBindingPipelineState VertexBindingCreateInfo::init(VertexArray& array, OpenGLContext& context) {\n";
    vector<VertexAttribute> attrs;
    for(auto& vertexInput : vertexInputs) {
        gather_attributes(vertexInput, VERTEX_INPUT_STRUCT, VERTEX_INPUT_BINDING, attrs);
    }
    for(auto& instanceInput : instanceInputs) {
        gather_attributes(instanceInput, INSTANCE_INPUT_STRUCT, INSTANCE_INPUT_BINDING, attrs);
    }
    write_attributes(attrs, out_impl);

    if(instanceInputs.size() > 0) {
        out_impl << "    context.setVertexArrayBindingDivisor(array, " << INSTANCE_INPUT_BINDING << ", 1);\n";
    }
    out_impl << "    return VertexBindingPipelineState {};\n";
    out_impl << "}\n";

    out << "struct ResourceBindingPipelineState;\n";
    out << "struct ResourceBindingCreateInfo;\n";

    vector<Field> both;
    both.insert(both.end(), uniformBlocks.begin(), uniformBlocks.end());
    both.insert(both.end(), textures.begin(), textures.end());
    emit_struct("ResourceBindings", both, [](auto out) {
        *out << "    using CreateInfo = ResourceBindingCreateInfo;\n";
        *out << "    using PipelineState = ResourceBindingPipelineState;\n";
    }, out);

    out << "struct ResourceBindingPipelineState {\n";
    out << "    void bindAll(const ResourceBindings& bindings, OpenGLContext& context);\n";

    out << "};\n";

    out_impl << "void ResourceBindingPipelineState::bindAll(const ResourceBindings& bindings, OpenGLContext& context) {\n";
    for(auto& block : uniformBlocks) {
        out_impl << "    context.bindUniformBuffer(bindings." << block.name << ".buffer, " << block.location.value() << ", bindings." << block.name << ".offset, sizeof(" << block.originalType->getTypeName() << "));\n";
    }
    for(auto& tex : textures) {
        out_impl << "    context.bindTextureAndSampler(" << tex.location.value() << ", bindings." << tex.name << ".texture, bindings." << tex.name << ".sampler);\n";
    }
    out_impl << "}\n";

    out << "struct ResourceBindingCreateInfo {\n";
    out << "    ResourceBindingPipelineState init();\n";
    out << "};\n";

    out_impl << "ResourceBindingPipelineState ResourceBindingCreateInfo::init() {\n";
    out_impl << "    return ResourceBindingPipelineState {};\n";
    out_impl << "}\n";

    out << "using Pipeline = GraphicsPipeline<VertexBindings, ResourceBindings>;\n";
    out << "using PipelineCreateInfo = GraphicsPipelineCreateInfo<VertexBindings, ResourceBindings>;\n";
    out << "using DrawCmd = DrawCommand<VertexBindings, ResourceBindings>;\n";

    delete program;

    out << "}}";
    out_impl << "}}";

    out.close();

    out_impl.close();

    return 0;
}