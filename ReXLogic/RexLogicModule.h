// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_RexLogicModule_h
#define incl_RexLogicModule_h

#include "ModuleInterface.h"
#include "RexServerConnection.h"
#include "AvatarController.h"

namespace RexLogic
{
    class NetworkEventHandler;
    class InputEventHandler;
    class SceneEventHandler;
    class NetworkStateEventHandler;
    class CameraController;
    class Terrain;

    typedef boost::shared_ptr<RexServerConnection> RexServerConnectionPtr;
    typedef boost::shared_ptr<AvatarController> AvatarControllerPtr;
    typedef boost::shared_ptr<CameraController> CameraControllerPtr;

    typedef boost::weak_ptr<Terrain> TerrainWeakPtr;
    typedef boost::shared_ptr<Terrain> TerrainPtr;

    enum InputController
    {
        Controller_Avatar,
        Controller_Camera
    };

    //! interface for modules
    class MODULE_API RexLogicModule : public Foundation::ModuleInterfaceImpl
    {
    public:
        RexLogicModule();
        virtual ~RexLogicModule();

        virtual void Load();
        virtual void Unload();
        virtual void Initialize();
        virtual void PostInitialize();
        virtual void Uninitialize();

        virtual void Update(Core::f64 frametime);
        
        virtual bool HandleEvent(Core::event_category_id_t category_id, Core::event_id_t event_id, Foundation::EventDataInterface* data);
        
        MODULE_LOGGING_FUNCTIONS;

        //! returns name of this module. Needed for logging.
        static const std::string &NameStatic() { return Foundation::Module::NameFromType(type_static_); }

        static const Foundation::Module::Type type_static_ = Foundation::Module::MT_WorldLogic;
        
        RexServerConnectionPtr GetServerConnection() const { return rexserver_connection_; }

        AvatarControllerPtr GetAvatarController() const { return avatar_controller_; }

        //! switch current input controller, if using avatar controller, switch to camera controller and vice versa
        void SwitchController();

        //! Recreates the terrain. Called at startup.
        void CreateTerrain();

        //! @return The current in-scene terrain object. May return 0 if there isn't one.
        TerrainPtr GetTerrain() { return terrain_; }

    private:        
        //! Event handler for network events.
        NetworkEventHandler *network_handler_;
        
        //! Event handler for network events.
        InputEventHandler *input_handler_; 
        
        //! Event handler for network state events.
        NetworkStateEventHandler *network_state_handler_;
        
        //! Event handler for scene events.
        SceneEventHandler *scene_handler_;
        
        //! Server connection
        RexServerConnectionPtr rexserver_connection_;

        //! Local avatar controller for this client
        AvatarControllerPtr avatar_controller_;

        //! (local) camera controller for this client
        CameraControllerPtr camera_controller_;

        typedef boost::function<bool(Core::event_id_t,Foundation::EventDataInterface*)> LogicEventHandlerFunction;
        typedef std::map<Core::event_category_id_t, LogicEventHandlerFunction> LogicEventHandlerMap;
        
        LogicEventHandlerMap event_handlers_;

        //! current input controller
        InputController current_controller_;

        TerrainPtr terrain_;
    };
}

#endif
