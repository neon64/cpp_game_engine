#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cassert>
#include <memory>
#include <variant>
#include <bitset>
#include <sstream>
#include <ranges>
#include <iostream>
#include <bitset>
#include <fmt/format.h>

#define LOGURU_WITH_STREAMS 1
#include <loguru/loguru.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/reflection.h>
#include "DirStackFileIncluder.h"

#include "shader_resource_limits.h"
#include "glsl_to_cpp.h"

using namespace fmt::v6;

using namespace std;
using namespace std::filesystem;

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

string toPascalCase(string s) {
    std::transform(s.begin() + 1, s.end(), s.begin() + 1, [](unsigned char c) { return std::tolower(c); });
    return s;
}

string toLowerCase(string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

struct Shader {
    path filePath;
    string versionStatement;
    string source;
    string preprocessedSource;
    glslang::TShader* shaderObject;

    string getUpperCaseStage() const {
        if(shaderObject->getStage() == EShLangVertex) {
            return "VERTEX";
        } else if(shaderObject->getStage() == EShLangFragment) {
            return "FRAGMENT";
        }
        return "UNKNOWN";
    }
};

struct NoValue {};
struct GreaterThanZero {};

struct Definition {
    string name;
    variant<int, bool, GreaterThanZero, NoValue> value;

    Definition(string input) {
        size_t index = input.find_first_of(">=");
        name = input.substr(0, index);
        if(index == string::npos) {
            value = NoValue {};
        } else if(input[index] == '>' && input.substr(index + 1) == "0") {
            value = GreaterThanZero {};
        } else if(input[index] == '=') {
            string strval = input.substr(index + 1);
            if(strval == "true") {
                value = true;
            } else if(strval == "false") {
                value = false;
            } else {
                int ival = stoi(strval);
                LOG_S(INFO) << "parsing integer " << strval << " into " << ival;
                value = ival;
            }
        }
    }
};

ostream& operator<<(ostream& out, const Definition& def) {
    out << "#define " << def.name;
    if(const int* b = get_if<int>(&def.value)) {
        out << " " << *b;
    } else if(bool b2 = get_if<bool>(&def.value)) {
        out << " " << b2;
    } else if(holds_alternative<NoValue>(def.value)) {
        out << " 1";
    } else if(holds_alternative<GreaterThanZero>(def.value)) {
        out << " 3";
    }
    return out;
}

const char *INCLUDE_EXTENSION = "#extension GL_GOOGLE_include_directive : enable\n";

int main(int argc, char *argv[]) {
    if(argc < 4) {
        LOG_S(ERROR) << "not enough arguments, expecting: [name] [input_file_0] [input_file_1] [input_file_2] ... [output_file] (--defines [defines])";
        return 1;
    }

    for(int i = 0; i < argc; i++) {
        LOG_S(9) << "argv[" << i << "] = " << argv[i];
    }

    int i = 1;

    string name = argv[i++];

    vector<Shader> shaders;
    while(i < argc) {
        if(strcmp("--specialize", argv[i]) == 0) {
            i++;
            break;
        }
        path p(argv[i++]);
        shaders.push_back({
            .filePath = p,
            .source = string(""),
            .shaderObject = nullptr
        });
    }

    path output(shaders.back().filePath);
    shaders.pop_back();

    vector<Definition> defines;
    while(i < argc) {
        stringstream parts;
        parts << argv[i];
        for(string part; getline(parts, part, ';'); ) {
            defines.push_back(Definition(part));
        }
        i++;
    }

    stringstream definitions;
    for(auto& def : defines) {
        definitions << def << "\n";
        LOG_S(INFO) << def;
    }
    string definitionsStr = definitions.str();

    glslang::InitializeProcess();

    for(auto& input : shaders) {
        LOG_S(INFO) << "reading shader source from " << input.filePath;
        ifstream in(input.filePath);
        in.exceptions(ifstream::failbit | ifstream::badbit);

        getline(in, input.versionStatement);
        // LOG_S(INFO) << versionStatement;
        input.versionStatement += "\n";
        input.versionStatement += INCLUDE_EXTENSION;

        if(!input.versionStatement.starts_with("#version")) {
            LOG_S(ERROR) << "shader must start with #version statement" << endl;
            return 1;
        };

// TODO: preallocate memory for shader string
//        in.seekg(0, ios::end);
//        input.source.reserve(in.tellg());
//        in.seekg(0, ios::beg);

        input.source = "\n";
        input.source.replace(input.source.end(), input.source.end(), (istreambuf_iterator<char>(in)),
                   istreambuf_iterator<char>());

        EShLanguage shaderStage = get_shader_stage(input.filePath.extension());

        const int clientVersion = 420;

        const char* sources[3] = {
            input.versionStatement.c_str(),
            definitionsStr.c_str(),
            input.source.c_str()
        };
        const int lengths[3] = {
                (int) input.versionStatement.size(),
                (int) definitionsStr.size(),
                (int) input.source.size()
        };
        const char* names[3] = {
            "versionDeclaration",
            "defineStatements",
            "source"
        };

        input.shaderObject = new glslang::TShader(shaderStage);
        input.shaderObject->setStringsWithLengthsAndNames(sources, lengths, names, 3);
        input.shaderObject->setEnvInput(glslang::EShSourceGlsl, shaderStage, glslang::EShClientOpenGL, glslang::EShTargetClientVersion(clientVersion));
        input.shaderObject->setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetClientVersion(clientVersion));
        input.shaderObject->setEnvTarget(glslang::EShTargetNone, glslang::EShTargetLanguageVersion(0));
        input.shaderObject->setAutoMapBindings(true);
        input.shaderObject->setAutoMapLocations(true);
        TBuiltInResource resources = DefaultTBuiltInResource;
        EShMessages messages = EShMsgDefault;

        DirStackFileIncluder includer;
        includer.pushExternalLocalDirectory(input.filePath.parent_path().c_str());

        bool result = input.shaderObject->parse(&resources, clientVersion, false, messages, includer);
        if(result) {
            LOG_S(INFO) << "compilation for " << input.filePath << " succeeded";
        } else {
            LOG_S(ERROR) << "compilation for " << input.filePath << " failed";
            LOG_S(ERROR) << input.shaderObject->getInfoLog();
            return 1;
        }

        input.shaderObject->preprocess(&resources, clientVersion, EProfile::ECoreProfile, false, true, messages, &input.preprocessedSource, includer);
        size_t offset = input.preprocessedSource.find(INCLUDE_EXTENSION);
        //
        input.preprocessedSource.replace(offset, strlen(INCLUDE_EXTENSION), "#extension GL_ARB_shading_language_include : enable");

        in.close();
    }

    glslang::TProgram *program = new glslang::TProgram();

    for(auto& shader : shaders) {
        program->addShader(shader.shaderObject);
    }

    bool result = program->link(EShMsgDefault);
    if(result) {
        LOG_S(INFO) << "linking succeeded";
    } else {
        LOG_S(ERROR) << "linking failed, info log: " << program->getInfoLog();
    }

    result = program->buildReflection(EShReflectionAllBlockVariables | EShReflectionAllIOVariables);
    LOG_S(INFO) << "build reflection status: " << result;

    LOG_S(INFO) << "opening output file " << output;
    ofstream out;
    out.open(output.replace_extension(".h"));

    ofstream out_impl;
    out_impl.open(output.replace_extension(".cpp"));

    out_impl << "#include " << output.filename().replace_extension(".h") << "\n";

    out << "#pragma once\n";
    out << "// autogenerated from GLSL, do not edit\n";
    out << "#include <glm/glm.hpp>\n";
    out << "#include \"../../src/graphics/OpenGLContext.h\"\n";
    out << "#include \"../../src/graphics/commands.h\"\n";
    out << "#include \"../../src/graphics/Shader.h\"\n";
    out << "#include \"../../src/util.h\"\n";
    out << "#include \"../../src/loader/shaders.h\"\n";
    out << "#include \"../../src/graphics/texturing.h\"\n";
    out << "namespace pipelines { namespace " << name << " {\n";
    out_impl << "#include <memory>\n";
    out_impl << "namespace pipelines { namespace " << name << " {\n";

    for(auto& shader : shaders) {
        string ty = shader.getUpperCaseStage();
        // out << "extern const char* " << ty << "_SHADER;\n";
        out_impl << "const char* " << ty <<  "_SHADER = R\"\"(\n" << shader.preprocessedSource << ")\"\";\n";

        out << "struct " << toPascalCase(ty) << "Shader {\n";
        out << "    string key = " << shader.filePath << ";\n";
        out << "    string getKey() const;\n";
        out << "    shared_ptr<Shader> build(OpenGLContext& context);\n";
        out << "};\n";

        out_impl << "string " << toPascalCase(ty) << "Shader::getKey() const { return key; }\n";
        out_impl << "shared_ptr<Shader> " << toPascalCase(ty) << "Shader::build(OpenGLContext& context) {\n";
        out_impl << "    return make_shared<Shader>(std::move(context.buildShader(ShaderType::" << ty << ", key, " << ty << "_SHADER)));\n";
        out_impl << "}\n";
    }

    out << "class Shaders {\n";
    out << "    ShaderCache& cache;\n";
    out << "public:\n";
    out << "    Shaders(ShaderCache& cache);\n";
    out << "    Shaders(ShaderCache* cache);\n";
    out << "    ShaderStages getStages() const;\n";
    out << "};\n";
    out_impl << "Shaders::Shaders(ShaderCache* cache) : cache(*cache) {}\n";
    out_impl << "Shaders::Shaders(ShaderCache& cache) : cache(cache) {}\n";
    out_impl << "ShaderStages Shaders::getStages() const {\n";
    out_impl << "    return {\n";
    for(auto& shader : shaders) {
        out_impl << "    ." << toLowerCase(shader.getUpperCaseStage()) << " = " << "cache.get(" << toPascalCase(shader.getUpperCaseStage()) << "Shader {}),\n";
    }
    out_impl << "    };\n";
    out_impl << "};\n";

    vector<string> defs;

    vector<Field> uniformBlocks;
    // uniform blocks
    for(int i = 0; i < program->getNumUniformBlocks(); i++) {
        const glslang::TObjectReflection& block = program->getUniformBlock(i);
        if(block.getType()->getQualifier().layoutPacking != glslang::TLayoutPacking::ElpStd140) {
            LOG_S(ERROR) << "uniform block `" << block.name << "` must be layout(std140)";
            return 1;
        }

        Type t = get_type(block.getType(), AlignmentRequirements::STD140, defs);

        string blockName(program->getUniformBlockName(i));
        blockName[0] = tolower(blockName[0]);

        Field f = {
            .name = blockName,
            .type = Type {
                .base = format("const BufferView<{}>", t.base),
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

        // uniforms with an byteOffset are already part of a uniform block
        if(uniform.offset != -1) {
            continue;
        }

        optional<Field> t = Field::create_from_sampler(uniform);
        if(t.has_value()) {
            textures.push_back(t.value());
        } else {
            return 1;
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
    emit_struct(VERTEX_INPUT_STRUCT, AlignmentRequirements::C_DEFAULT, vertexInputs, [](auto o) {}, out);
    vertexBindings.push_back({ .name = VERTEX_INPUT_MEMBER_NAME, .type = Type { .base = format("const VertexBufferBinding<{}>", VERTEX_INPUT_STRUCT), .numElements = nullopt }, .location = nullopt });
    if(instanceInputs.size() > 0) {
        emit_struct(INSTANCE_INPUT_STRUCT, AlignmentRequirements::C_DEFAULT, instanceInputs, [](auto o) {}, out);
        vertexBindings.push_back({ .name = INSTANCE_INPUT_MEMBER_NAME, .type = Type { .base = format("const VertexBufferBinding<{}>", INSTANCE_INPUT_STRUCT), .numElements = nullopt }, .location = nullopt });
    }

    out << "struct VertexBindingPipelineState;\n";
    out << "struct VertexBindingCreateInfo;\n";

    emit_struct("VertexBindings", AlignmentRequirements::C_DEFAULT, vertexBindings, [](auto out) {
        *out << "    using CreateInfo = VertexBindingCreateInfo;\n";
        *out << "    using PipelineState = VertexBindingPipelineState;\n";
    }, out);

    out << "struct VertexBindingPipelineState {\n";
    out << "    void bindAll(const VertexBindings& bindings, BoundVertexArrayGuard& guard, OpenGLContext& context);\n";
    out << "};\n";

    out_impl << "void VertexBindingPipelineState::bindAll(const VertexBindings& bindings, BoundVertexArrayGuard& guard, OpenGLContext& context) {\n";
    out_impl << "    guard.bindVertexBuffer(" << VERTEX_INPUT_BINDING << ", bindings." << VERTEX_INPUT_MEMBER_NAME << ".buffer, bindings." << VERTEX_INPUT_MEMBER_NAME << ".byteOffset, sizeof(" << VERTEX_INPUT_STRUCT << "));\n";
    if(instanceInputs.size() > 0) {
        out_impl << "    guard.bindVertexBuffer(" << INSTANCE_INPUT_BINDING << ", bindings." << INSTANCE_INPUT_MEMBER_NAME
            << ".buffer, bindings." << INSTANCE_INPUT_MEMBER_NAME << ".byteOffset, sizeof(" << INSTANCE_INPUT_STRUCT
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
    out_impl << "    context.withBoundVertexArray(array, [](auto guard) {\n";
    write_attributes(attrs, out_impl);

    if(instanceInputs.size() > 0) {
        out_impl << "        guard.setBindingDivisor(" << INSTANCE_INPUT_BINDING << ", 1);\n";
    }

    out_impl << "    });\n";

    out_impl << "    return VertexBindingPipelineState {};\n";
    out_impl << "}\n";

    out << "struct ResourceBindingPipelineState;\n";
    out << "struct ResourceBindingCreateInfo;\n";

    vector<Field> both;
    both.insert(both.end(), uniformBlocks.begin(), uniformBlocks.end());
    both.insert(both.end(), textures.begin(), textures.end());
    emit_struct("ResourceBindings", AlignmentRequirements::C_DEFAULT, both, [](auto out) {
        *out << "    using CreateInfo = ResourceBindingCreateInfo;\n";
        *out << "    using PipelineState = ResourceBindingPipelineState;\n";
    }, out);

    out << "struct ResourceBindingPipelineState {\n";
    out << "    void bindAll(const ResourceBindings& bindings, OpenGLContext& context);\n";

    out << "};\n";

    out_impl << "void ResourceBindingPipelineState::bindAll(const ResourceBindings& bindings, OpenGLContext& context) {\n";
    for(auto& block : uniformBlocks) {
        out_impl << "    context.bindUniformBuffer(bindings." << block.name << ".buffer, " << block.location.value() << ", bindings." << block.name << ".byteOffset, sizeof(" << block.originalType->getTypeName() << "));\n";
    }
    for(auto& tex : textures) {
        if(tex.type.numElements.has_value()) {
            out_impl << "    for(int i = 0; i < " << tex.type.numElements.value() << "; i++) {\n";
            out_impl << "        context.bindTextureAndSampler(" << tex.location.value() << " + i, bindings." << tex.name << "[i]);\n";
            out_impl << "    }\n";
        } else {
            out_impl << "    context.bindTextureAndSampler(" << tex.location.value() << ", bindings." << tex.name << ");\n";
        }
    }
    out_impl << "}\n";

    out << "struct ResourceBindingCreateInfo {\n";
    out << "    ResourceBindingPipelineState init();\n";
    out << "};\n";

    out_impl << "ResourceBindingPipelineState ResourceBindingCreateInfo::init() {\n";
    out_impl << "    return ResourceBindingPipelineState {};\n";
    out_impl << "}\n";

    out << "using Pipeline = GraphicsPipeline<VertexBindings, ResourceBindings>;\n";
    out << "using Create = GraphicsPipelineCreateInfo<VertexBindings, ResourceBindings, Shaders>;\n";
    out << "using DrawCmd = DrawCommand<VertexBindings, ResourceBindings>;\n";

    delete program;

    for(auto& shader : shaders) {
        delete shader.shaderObject;
    }

    out << "}}";
    out_impl << "}}";

    out.close();

    out_impl.close();

    return 0;
}