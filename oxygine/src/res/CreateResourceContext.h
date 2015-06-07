#pragma once
#include "oxygine_include.h"
#include <string>
#include "pugixml/pugixml.hpp"
#include "core/Object.h"

namespace oxygine
{

    class Resources;
    /**internal class*/


    class XmlWalker
    {
    public:
        XmlWalker(const std::string* xmlFolder, const std::string& path, float scaleFactor, bool load, bool alpha, pugi::xml_node xml, pugi::xml_node meta);

        bool empty() const {return _root.empty();}

        const std::string&  getCurrentFolder() const { return _path; }
        const std::string&  getXmlFolder() const { return *_xmlFolder; }
        std::string         getPath(const char* attrName) const;
        pugi::xml_node      getNode() const {return _root;}
        pugi::xml_node      getMeta() const {return _rootMeta;}
        float               getScaleFactor() const {return _scaleFactor;}
        bool                getLoad() const {return _load;}
        bool                getAlphaHitTest() const { return _alphaHitTest; }
        const char*         getType() const {return _root.name();}

        void            checkSetAttributes();


        XmlWalker       next();
        pugi::xml_node  nextMeta();

    private:
        void            _checkSetAttributes(pugi::xml_node node);
        std::string connectPath(const char* currentPath, const char* str);

        const std::string* _xmlFolder;
        std::string _path;

        pugi::xml_node _root;
        pugi::xml_node _last;

        pugi::xml_node _rootMeta;
        pugi::xml_node _lastMeta;

        bool _notStarted;
        bool _notStartedMeta;

        float _scaleFactor;
        bool _load;
        bool _alphaHitTest;
    };

    class CreateResourceContext//todo rename
    {
    public:
        CreateResourceContext(): resources(0), xml_name(0), prebuilt_folder(0),
            walker(0, "", 1.0f, true, false, pugi::xml_node(), pugi::xml_node())
        {

        }

        Resources* resources;
        //float scale_factor;

        XmlWalker walker;

        //pugi::xml_node node;
        //pugi::xml_node meta;

        const std::string* xml_name;
        //const string *folder;
        const std::string* prebuilt_folder;
    };

    DECLARE_SMART(MemoryTexture, spMemoryTexture);
    DECLARE_SMART(NativeTexture, spNativeTexture);

    /**internal class*/
    class LoadResourcesContext
    {
    public:
        virtual ~LoadResourcesContext() {}

        virtual void createTexture(spMemoryTexture src, spNativeTexture dest) = 0;
        virtual bool isNeedProceed(spNativeTexture t) = 0;
    };

    class SingleThreadResourcesContext: public LoadResourcesContext
    {
    public:
        static SingleThreadResourcesContext instance;
        void createTexture(spMemoryTexture src, spNativeTexture dest);
        bool isNeedProceed(spNativeTexture t);
    };

    class RestoreResourcesContext: public LoadResourcesContext
    {
    public:
        static RestoreResourcesContext instance;
        void createTexture(spMemoryTexture src, spNativeTexture dest);
        bool isNeedProceed(spNativeTexture t);
    };
}
