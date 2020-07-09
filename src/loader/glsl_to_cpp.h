#pragma once

#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/reflection.h>
#include <iostream>
#include <fstream>
#include <utility>
#include <string>
#include <optional>

using namespace std;

struct Type {
    string base;
    optional<int> numElements;
};

struct Field {
    string name;
    Type type;
    const glslang::TType* originalType;
    optional<int> location;

    static optional<Field> create_from_pipe_input(const glslang::TObjectReflection& field, vector<string>& defs);
    static optional<Field> create_from_sampler(const glslang::TObjectReflection& uniform);
};

struct VertexAttribute {
    int location;
    string offsetExpr;
    string dataFormat;
    int binding;
};

void gather_attributes(const Field &field, const std::string_view& structName, int binding, vector<VertexAttribute>& attrs);

void write_attributes(const vector<VertexAttribute>& attributes, ostream& out);

template<typename F>
void emit_struct(const char* name, vector<Field>& fields, F write_extra_defs, ostream& out) {
    out << "struct " << name << " {\n";
    for(auto& field : fields) {
        out << "    " << field.type.base << " " << field.name;
        if(field.type.numElements.has_value()) {
            out << "[" << field.type.numElements.value() << "]";
        }
        out << ";\n";
    }

    write_extra_defs(&out);

    out << "};\n";
}

Type get_type(const glslang::TType* type, vector<string>& defs);