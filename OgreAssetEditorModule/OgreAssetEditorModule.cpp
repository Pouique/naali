/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   OgreAssetEditorModule.cpp
 *  @brief  OgreAssetEditorModule.provides editing and previewing tools for
 *          OGRE assets such as meshes and material scripts.
 */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "OgreAssetEditorModule.h"
#include "EditorManager.h"
#include "OgreScriptEditor.h"
#include "TexturePreviewEditor.h"
#include "AudioPreviewEditor.h"
#include "MeshPreviewEditor.h"
#include "MaterialWizard.h"

#include "Framework.h"
#include "EventManager.h"
#include "ModuleManager.h"
#include "NetworkEvents.h"
#include "Inventory/InventoryEvents.h"
#include "ResourceInterface.h"
#include "AssetInterface.h"
#include "UiModule.h"
#include "Inworld/InworldSceneController.h"
#include "UiProxyWidget.h"

#include "MemoryLeakCheck.h"

namespace Naali
{

std::string OgreAssetEditorModule::type_name_static_ = "OgreAssetEditor";

OgreAssetEditorModule::OgreAssetEditorModule() :
    ModuleInterface(type_name_static_),
    frameworkEventCategory_(0),
    inventoryEventCategory_(0),
    assetEventCategory_(0),
    resourceEventCategory_(0),
    networkStateEventCategory_(0),
    materialWizard_(0),
    editorManager_(0)
{
}

OgreAssetEditorModule::~OgreAssetEditorModule()
{
}

void OgreAssetEditorModule::Initialize()
{
    // Get event manager.
    eventManager_ = framework_->GetEventManager();
}

void OgreAssetEditorModule::PostInitialize()
{
    frameworkEventCategory_ = eventManager_->QueryEventCategory("Framework");
    inventoryEventCategory_ = eventManager_->QueryEventCategory("Inventory");
    assetEventCategory_ = eventManager_->QueryEventCategory("Asset");
    resourceEventCategory_ = eventManager_->QueryEventCategory("Resource");

    materialWizard_ = new MaterialWizard;
    connect(materialWizard_, SIGNAL(NewMaterial(Inventory::InventoryUploadEventData *)),
        this, SLOT(UploadFile(Inventory::InventoryUploadEventData *)));

    uiModule_ = framework_->GetModuleManager()->GetModule<UiServices::UiModule>();
    if (!uiModule_.expired())
    {
        UiServices::UiWidgetProperties props("Material Wizard", "./data/ui/images/menus/edbutton_MATWIZ_normal.png");
        props.SetMenuGroup("World Tools");

        UiProxyWidget *proxy  = uiModule_.lock()->GetInworldSceneController()->AddWidgetToScene(materialWizard_);
        uiModule_.lock()->GetInworldSceneController()->AddWidgetToMenu(materialWizard_, props);
        connect(proxy, SIGNAL(Closed()), materialWizard_, SLOT(Close()));
    }

    editorManager_ = new EditorManager;
}

void OgreAssetEditorModule::Uninitialize()
{
    SAFE_DELETE(materialWizard_);
    SAFE_DELETE(editorManager_);
    eventManager_.reset();
}

void OgreAssetEditorModule::Update(f64 frametime)
{
    RESETPROFILER;
}

bool OgreAssetEditorModule::HandleEvent(event_category_id_t category_id, event_id_t event_id, Foundation::EventDataInterface* data)
{
    if (category_id == frameworkEventCategory_)
    {
        if (event_id == Foundation::NETWORKING_REGISTERED)
        {
            networkStateEventCategory_ = eventManager_->QueryEventCategory("NetworkState");
            return false;
        }
    }
    if (category_id == inventoryEventCategory_)
    {
        if (event_id == Inventory::Events::EVENT_INVENTORY_ITEM_OPEN)
        {
            // Inventory item requested for opening. Check if we have editor for its type.
            if (uiModule_.expired())
                return false;

            Inventory::InventoryItemOpenEventData *open_item = dynamic_cast<Inventory::InventoryItemOpenEventData *>(data);
            if(!open_item)
                return false;

            /*switch(open_item->assetType)
            {
                case RexTypes::RexAT_Texture:
                {
                    open_item->overrideDefaultHandler = true;
                    break;
                }
            }*/
        }
        if (event_id == Inventory::Events::EVENT_INVENTORY_ITEM_DOWNLOADED)
        {
            if (uiModule_.expired())
                return false;

            // Asset downloaded, pass asset data to the right editor and bring it to front.
            // This event is sent also when asset is already in cache and doens't need to be
            // downloaded. Hence, if there is no editor created yet, create it now.
            Inventory::InventoryItemDownloadedEventData *downloaded = checked_static_cast<Inventory::InventoryItemDownloadedEventData *>(data);
            assert(downloaded);
            asset_type_t at = downloaded->assetType;
            switch(at)
            {
            case RexTypes::RexAT_ParticleScript:
            case RexTypes::RexAT_MaterialScript:
            {
                const QString &id = downloaded->inventoryId.ToString().c_str();
                const QString &name = downloaded->name.c_str();
                if (!editorManager_->Exists(id, at))
                {
                    // Editor not created, create it now.
                    OgreScriptEditor *editor = new OgreScriptEditor(id, at, name);
                    connect(editor, SIGNAL(UploadNewScript(Inventory::InventoryUploadBufferEventData *)),
                        this, SLOT(UploadBuffer(Inventory::InventoryUploadBufferEventData *)));
                    connect(editor, SIGNAL(Closed(const QString &, asset_type_t)),
                        editorManager_, SLOT(Delete(const QString &, asset_type_t)));
                    editorManager_->Add(id, at, editor);
                    editor->HandleAssetReady(downloaded->asset);

                    // Add widget to scene, show and bring to front
                    uiModule_.lock()->GetInworldSceneController()->AddWidgetToScene(editor);
                    uiModule_.lock()->GetInworldSceneController()->ShowProxyForWidget(editor);
                    uiModule_.lock()->GetInworldSceneController()->BringProxyToFront(editor);
                }
                else
                {
                    // Editor already exists, bring it to front.
                    QWidget *editor = editorManager_->GetEditor(id, at);
                    if (editor)
                        uiModule_.lock()->GetInworldSceneController()->BringProxyToFront(editor);
                }
                // Surpress this event.
                downloaded->handled = true; 
                return true;
            }
            case RexTypes::RexAT_SoundVorbis:
            case RexTypes::RexAT_SoundWav:
            {
                const QString &id = downloaded->inventoryId.ToString().c_str();
                const QString &name = downloaded->name.c_str();
                if(!editorManager_->Exists(id, at))
                {
                    AudioPreviewEditor *editor = new AudioPreviewEditor(framework_, id, at, name);
                    QObject::connect(editor, SIGNAL(Closed(const QString &, asset_type_t)),
                            editorManager_, SLOT(Delete(const QString &, asset_type_t)));
                    editorManager_->Add(id, at, editor);
                    editor->HandleAssetReady(downloaded->asset);
                }
                else
                {
                    // Editor already exists, bring it to front.
                    QWidget *editor = editorManager_->GetEditor(id, at);
                    if (editor)
                    {
                        uiModule_.lock()->GetInworldSceneController()->BringProxyToFront(editor);
                        AudioPreviewEditor *audioWidget = qobject_cast<AudioPreviewEditor*>(editor);
                        if(audioWidget)
                            audioWidget->HandleAssetReady(downloaded->asset);
                    }
                }

                downloaded->handled = true;
                return true;
            }
            case RexTypes::RexAT_Mesh:
            {
                const QString &id = downloaded->inventoryId.ToString().c_str();
                const QString &name = downloaded->name.c_str();
                if(!editorManager_->Exists(id, at))
                {
                    MeshPreviewEditor *editor = new MeshPreviewEditor(framework_, id, at, name, downloaded->asset->GetId().c_str());
                    QObject::connect(editor, SIGNAL(Closed(const QString &, asset_type_t)),
                            editorManager_, SLOT(Delete(const QString &, asset_type_t)));
                    editorManager_->Add(id, at, editor);
                    //editor->HandleAssetReady(downloaded->asset);
                }
                else
                {
                    // Editor already exists, bring it to front.
                    QWidget *editor = editorManager_->GetEditor(id, at);
                    if (editor != 0)
                    {
                        uiModule_.lock()->GetInworldSceneController()->BringProxyToFront(editor);
                        MeshPreviewEditor *editorWidget = qobject_cast<MeshPreviewEditor*>(editor);
                        if(editorWidget)
                            editorWidget->RequestMeshAsset(downloaded->asset->GetId().c_str());
                    }
                }

                downloaded->handled = true;
                return true;
            }
            case RexTypes::RexAT_Texture:
            {
                const QString &id = downloaded->inventoryId.ToString().c_str();
                const QString &name = downloaded->name.c_str();
                if(!editorManager_->Exists(id, at))
                {
                    TexturePreviewEditor *editor = new TexturePreviewEditor(framework_, id, at, name, downloaded->asset->GetId().c_str());
                    QObject::connect(editor, SIGNAL(Closed(const QString &, asset_type_t)),
                            editorManager_, SLOT(Delete(const QString &, asset_type_t)));
                    editorManager_->Add(id, at, editor);
                    //editor->RequestTextureAsset(downloaded->asset->GetId());
                }
                else
                {
                    // Editor already exists, bring it to front.
                    QWidget *editor = editorManager_->GetEditor(id, at);
                    if (editor)
                        uiModule_.lock()->GetInworldSceneController()->BringProxyToFront(editor);
                }

                downloaded->handled = true;
                return true;
            }
            default:
                return false;
            }
        }
    }
    else if (category_id == networkStateEventCategory_)
    {
        if (event_id == ProtocolUtilities::Events::EVENT_SERVER_DISCONNECTED)
            editorManager_->DeleteAll();
    }
    else if (category_id == resourceEventCategory_)
    {
        if (event_id == Resource::Events::RESOURCE_CANCELED)
            return false;

        Resource::Events::ResourceReady *res = dynamic_cast<Resource::Events::ResourceReady*>(data);
        assert(res);
        if (!res)
            return false;

        if(res->resource_->GetType() == "Texture")
        {
            QVector<QWidget*> editorList = editorManager_->GetEditorListByAssetType(RexTypes::RexAT_Texture);
            for(uint i = 0; i < editorList.size(); i++)
            {
                TexturePreviewEditor *editorWidget = qobject_cast<TexturePreviewEditor*>(editorList[i]);
                if(editorWidget)
                    editorWidget->HandleResouceReady(res);
            }
        }
        else if(res->resource_->GetType() == "Sound")
        {
            QVector<QWidget*> editorList = editorManager_->GetEditorListByAssetType(RexTypes::RexAT_SoundVorbis);
            for(uint i = 0; i < editorList.size(); i++)
            {
                AudioPreviewEditor *editorWidget = qobject_cast<AudioPreviewEditor*>(editorList[i]);
                if(editorWidget)
                    editorWidget->HandleResouceReady(res);
            }
        }

        if ( res->resource_->GetType() == "Mesh" || res->resource_->GetType() == "OgreMesh")
        {
            QVector<QWidget*> editorList = editorManager_->GetEditorListByAssetType(RexTypes::RexAT_Mesh);
            for(uint i = 0; i < editorList.size(); i++)
            {
                MeshPreviewEditor *editorWidget = qobject_cast<MeshPreviewEditor*>(editorList[i]);
                if(editorWidget != 0)
                    editorWidget->HandleResouceReady(res);
            }
        }
    }

    return false;
}

void OgreAssetEditorModule::UploadFile(Inventory::InventoryUploadEventData *data)
{
    framework_->GetEventManager()->SendEvent(inventoryEventCategory_, Inventory::Events::EVENT_INVENTORY_UPLOAD_FILE, data);
}

void OgreAssetEditorModule::UploadBuffer(Inventory::InventoryUploadBufferEventData *data)
{
    framework_->GetEventManager()->SendEvent(inventoryEventCategory_, Inventory::Events::EVENT_INVENTORY_UPLOAD_BUFFER, data);
}

}

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace Naali;

POCO_BEGIN_MANIFEST(Foundation::ModuleInterface)
    POCO_EXPORT_CLASS(OgreAssetEditorModule)
POCO_END_MANIFEST
