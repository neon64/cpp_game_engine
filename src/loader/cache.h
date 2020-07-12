

#pragma once    

#include <memory>
#include "../graphics/Shader.h"
#include "../graphics/OpenGLContext.h"

template<typename K, typename T>
class ResourceCache {
    OpenGLContext& context;
    unordered_map<K, shared_ptr<T>> cache;
public:

    ResourceCache(OpenGLContext& context) : context(context) {}

    template<typename Metadata>
    shared_ptr<T> get(Metadata metadata) {
        auto key = metadata.getKey();
        auto it = cache.find(key);
        if(it == cache.end()) {
            cache[key] = metadata.build(context);
        }

        return cache[key];
    }
};