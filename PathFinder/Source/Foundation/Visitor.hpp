#pragma once

namespace Foundation
{

    template<typename ...Ts>
    struct Visitor : Ts ... {
        Visitor(const Ts &... args) : Ts(args)... {}
        using Ts::operator()...;
    };

    template<typename ...Ts>
    auto MakeVisitor(Ts... lambdas) {
        return Visitor<Ts...>(lambdas...);
    }

    /*  template<class Texture>
      void GLFramebuffer::attachTextures(uint16_t mipLevel, const Texture &texture) {
          attachTextureToColorAttachment(texture, ColorAttachment::Automatic, mipLevel);
      }

      template<class Texture, class... Textures>
      void GLFramebuffer::attachTextures(uint16_t mipLevel, const Texture &head, const Textures &... tail) {
          attachTextureToColorAttachment(head, ColorAttachment::Automatic, mipLevel);
          attachTextures(mipLevel, tail...);
      }*/



    template<
        template<class...> class AssociativeContainer,
        class Key,
        class Value
    >
    decltype(auto) Find(const AssocitiveContainer<Key, Value>& container, Key&& key)
    {
        auto iterator = container.find(std::forward<Key>(key));

        if (iterator == container.end())
        {
            return nullptr;
        }

        return &iterator->second;
    }

    template<
        template<class...> class AssociativeContainer,
        class Value,
        class Key,
        class... Keys
    >
    decltype(auto) Find(const AssociativeContainer<Key, Value>& container, Key&& key, Keys&&... keys)
    {
        auto valuePtr = Find(container, std::forward<Key>(keys));

        if (!valuePtr)
        {
            return nullptr;
        }


    }

}

