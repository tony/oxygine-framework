#pragma once
#include "oxygine_include.h"
#include "core/file.h"
#include <map>
#include "Resource.h"
#include "ResAnim.h"

namespace oxygine
{
    class Resources;
    class XmlWalker;
    class CreateResourceContext;
    DECLARE_SMART(NativeTexture, spNativeTexture);

    class ResAtlas: public _Resource
    {
    public:
        static Resource* create(CreateResourceContext& context);
        struct atlas
        {
            spNativeTexture base;
            std::string base_path;

            spNativeTexture alpha;
            std::string alpha_path;
        };


        ResAtlas();
        ~ResAtlas();

        void addAtlas(TextureFormat tf, const std::string& base, const std::string& alpha, int w, int h);
        const atlas& getAtlas(int i) const {return _atlasses[i];}
        int getNum() const { return (int)_atlasses.size(); }

    protected:
        void _restore(Restorable* r, void* user);

        void _load(LoadResourcesContext*);
        void _unload();

        //void loadAtlas(CreateResourceContext& context);
        ResAnim* createEmpty(const XmlWalker& walker, CreateResourceContext& context);
        static void init_resAnim(ResAnim* rs, const std::string& file, pugi::xml_node node);

    protected:
        //settings from xml
        //bool _linearFilter;
        //bool _clamp2edge;

        std::vector<unsigned char> _hitTestBuffer;

        typedef std::vector<atlas> atlasses;
        atlasses _atlasses;
    };

    typedef void(*load_texture_hook)(const std::string& file, spNativeTexture nt, LoadResourcesContext* load_context);
    void set_load_texture_hook(load_texture_hook);


    class ResAtlasGeneric : public ResAtlas
    {
    public:

        //protected:
        void loadAtlas(CreateResourceContext& context);
    };

    class ResAtlasPrebuilt : public ResAtlas
    {
    public:

        ResAtlasPrebuilt(CreateResourceContext& context);

    protected:
    };
}
