#include <cassert>
#include <fmt/format.h>
#include <sstream>

#include <glslang/MachineIndependent/gl_types.h>
#include "glsl_to_cpp.h"

using namespace fmt::v6;

Type get_type(const glslang::TType* type, vector<string>& defs) {
    Type output {
        .base = "",
        .numElements = nullopt
    };

    if(type->isSizedArray() && type->getArraySizes()->getNumDims() == 1) {
        output.numElements = type->getArraySizes()->getDimSize(0);
    } else if(type->isArray()) {
        // unhandled
        assert(false);
    }

    if(type->isStruct()) {
        const glslang::TTypeList* fields = type->getStruct();
        vector<Field> mappedFields;

        for(auto& field : *fields) {
            Type fieldType = get_type(field.type, defs);

            auto fieldName = field.type->getFieldName();

            mappedFields.push_back(Field {
                .name = string(fieldName),
                .type = fieldType,
                .location = nullopt
            });
        }

        stringstream out;
        emit_struct(type->getTypeName().c_str(), mappedFields, [](auto o) {}, out);
        defs.push_back(out.str());

        output.base = type->getTypeName();
    } else if(type->isMatrix()) {
        if(type->getBasicTypeString() == "float") {
            if(type->getMatrixRows() == type->getMatrixCols())  {
                output.base = format("glm::mat{}", type->getMatrixRows());
            } else {
                // OpenGL and GLM both use *column-major* ordering for matrices (4x3 is 4 columns, 3 rows)
                output.base = format("glm::mat{}x{}", type->getMatrixCols(), type->getMatrixRows());
            }
        }
    } else if(type->isVector()) {
        if(type->getBasicString() == "float") {
            output.base = format("glm::vec{}", type->getVectorSize());
        } else {
            assert(false);
        }
    } else if(type->isScalarOrVec1()) {
        output.base = type->getBasicString();
    }

    return output;
}

string get_data_format(const std::string_view& basicString, int vectorSize) {
    if(basicString == "float") {
        if(vectorSize == 1) {
            return "DataFormat::R32_SFLOAT";
        } else if(vectorSize == 2) {
            return "DataFormat::R32G32_SFLOAT";
        } else if(vectorSize == 3) {
            return "DataFormat::R32G32B32_SFLOAT";
        } else if(vectorSize == 4) {
            return "DataFormat::R32G32B32A32_SFLOAT";
        }
    }
    assert(false);
    return "DataFormat::UNKNOWN";
}

void gather_attributes(const Field &field, const std::string_view& structName, int binding, vector<VertexAttribute>& attrs) {
    assert(!field.type.numElements.has_value());
    if(field.originalType->isMatrix()) {
        for(int i = 0; i < field.originalType->getMatrixCols(); i++) {
            attrs.push_back({
                    .location = field.location.value() + i,
                    .offsetExpr = format("offsetof({}, {}) + sizeof({})*{}*{}", structName, field.name, field.originalType->getBasicString(), field.originalType->getMatrixRows(), i),
                    .dataFormat = get_data_format(field.originalType->getBasicString(), field.originalType->getMatrixRows()),
                    .binding = binding
            });
        }
    } else {
        attrs.push_back({
                 .location = field.location.value(),
                 .offsetExpr = format("offsetof({}, {})", structName, field.name),
                 .dataFormat = get_data_format(field.originalType->getBasicString(), field.originalType->getVectorSize()),
                 .binding = binding
        });
    }
}

void write_attributes(const vector<VertexAttribute>& attributes, ostream &out) {
    for(auto& attribute : attributes) {
        out << "    context.enableVertexArrayAttribute(array, " <<  attribute.location << ");\n";
    }
    for(auto& attribute : attributes) {
        out << "    context.setVertexArrayAttributeFormat(array, " << attribute.location << ", " << attribute.dataFormat << ", " << attribute.offsetExpr << ");\n";
    }
    for(auto& attribute : attributes) {
        out << "    context.setVertexArrayAttributeBinding(array, " << attribute.location << ", " << attribute.binding << ");\n";
    }
}

optional<Field> Field::create_from_sampler(const glslang::TObjectReflection& uniform) {
    Field f;
    f.name = uniform.name;
    if(!uniform.getType()->getQualifier().hasBinding()) {
        assert(false);
    }
    f.location = make_optional(uniform.getType()->getQualifier().layoutBinding);
    f.originalType = uniform.getType();
    switch(uniform.glDefineType) {
        case GL_SAMPLER_2D:
            f.type = Type { .base = "const TextureBinding<Texture2d>", .numElements = nullopt };
            break;
        case GL_SAMPLER_CUBE:
            f.type = Type { .base = "const TextureBinding<TextureCube>", .numElements = nullopt };
            break;
        default:
            return nullopt;
    }

    return make_optional(f);
}

optional<Field> Field::create_from_pipe_input(const glslang::TObjectReflection& field, vector<string>& defs) {
    Field f;
    f.name = field.name;
    f.type = get_type(field.getType(), defs);
    f.originalType = field.getType();
    if(field.getType()->isBuiltIn()) {
        return nullopt;
    }
    if(!field.getType()->getQualifier().hasLocation()) {
        field.dump();
        assert(false);
    }
    f.location = make_optional(field.getType()->getQualifier().layoutLocation);
    return make_optional(f);
}
