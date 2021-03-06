// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "WebSocketServerModuleApi.h"

#include "WebSocketFwd.h"
#include "TundraProtocolModuleFwd.h"

#include "Win.h"
#include "SyncState.h"
#include "SceneFwd.h"
#include "AttributeChangeType.h"
#include "EntityAction.h"
#include "MsgEntityAction.h"

#include <kNetFwd.h>
#include <kNet/Types.h>

#include <QObject>

namespace WebSocket
{
/// Performs synchronization of the changes in a scene between the server and the client.
/** SyncManager and SceneSyncState combined can be used to implement prioritization logic on how and when
    a sync state is filled per client connection. SyncManager object is only exposed to scripting on the server. */
class WEBSOCKET_SERVER_MODULE_API SyncManager : public QObject, public enable_shared_from_this<SyncManager>
{
    Q_OBJECT

public:
    explicit SyncManager(WebSocketServerModule* owner);
    ~SyncManager();
    
    /// Register to entity/component change signals from a specific scene and start syncing them
    void RegisterToScene(ScenePtr scene);
    
    /// Accumulate time & send pending sync messages if enough time passed from last update
    void Update(f64 frametime);
    
public slots:
    /// Create new replication state for user and dirty it (server operation only)
    void OnUserConnected(WebSocket::UserConnection *connection, QVariantMap *data);
    
    /// Web client entity action handler. Triggers the action to the Entity.
    void OnClientEntityAction(WebSocket::UserConnection *source, MsgEntityAction action);
    
    /// Current scene.
    ScenePtr GetRegisteredScene() const { return scene_.lock(); }
    
    /// Set update period (seconds)
    void SetUpdatePeriod(float period);

    /// Get update period
    float GetUpdatePeriod() const { return updatePeriod_; }

    /// Returns SceneSyncState for a client connection.
    /** @note This slot is only exposed on Server, other wise will return null ptr.
        @param int connection ID of the client. */
    SceneSyncState* SceneState(uint connectionId) const;
    SceneSyncState* SceneState(WebSocket::UserConnection *connection) const; /**< @overload @param connection Client connection.*/

signals:
    /// This signal is emitted when a new user connects and a new SceneSyncState is created for the connection.
    /// @note See signals of the SceneSyncState object to build prioritization logic how the sync state is filled.
    void SceneStateCreated(UserConnection *user, SceneSyncState *state);
    
private slots:
    /// Trigger EC sync because of component attributes changing
    void OnAttributeChanged(IComponent* comp, IAttribute* attr, AttributeChange::Type change);

    /// Trigger EC sync because of component attribute added
    void OnAttributeAdded(IComponent* comp, IAttribute* attr, AttributeChange::Type change);

    /// Trigger EC sync because of component attribute removed
    void OnAttributeRemoved(IComponent* comp, IAttribute* attr, AttributeChange::Type change);
    
    /// Trigger EC sync because of component added to entity
    void OnComponentAdded(Entity* entity, IComponent* comp, AttributeChange::Type change);
    
    /// Trigger EC sync because of component removed from entity
    void OnComponentRemoved(Entity* entity, IComponent* comp, AttributeChange::Type change);
    
    /// Trigger sync of entity creation
    void OnEntityCreated(Entity* entity, AttributeChange::Type change);
    
    /// Trigger sync of entity removal
    void OnEntityRemoved(Entity* entity, AttributeChange::Type change);

    /// Trigger sync of entity action.
    void OnActionTriggered(Entity *entity, const QString &action, const QStringList &params, EntityAction::ExecTypeField type);

    /// Trigger sync of entity action to specific user
    void OnUserActionTriggered(WebSocket::UserConnection* user, Entity *entity, const QString &action, const QStringList &params);

    /// Handle a Kristalli protocol message
    void HandleKristalliMessage(UserConnection* source, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

private:
    /// Queue a message to the receiver from a given DataSerializer.
    void QueueMessage(UserConnection* connection, kNet::message_id_t id, bool reliable, bool inOrder, kNet::DataSerializer& ds);
    
    /// Craft a component full update, with all static and dynamic attributes.
    void WriteComponentFullUpdate(kNet::DataSerializer& ds, ComponentPtr comp);
    
    /// Handle entity action message.
    void HandleEntityAction(UserConnection* source, MsgEntityAction& msg);
    /// Handle create entity message.
    void HandleCreateEntity(UserConnection* source, const char* data, size_t numBytes);
    /// Handle create components message.
    void HandleCreateComponents(UserConnection* source, const char* data, size_t numBytes);
    /// Handle create attributes message.
    void HandleCreateAttributes(UserConnection* source, const char* data, size_t numBytes);
    /// Handle edit attributes message.
    void HandleEditAttributes(UserConnection* source, const char* data, size_t numBytes);
    /// Handle remove attributes message.
    void HandleRemoveAttributes(UserConnection* source, const char* data, size_t numBytes);
    /// Handle remove components message.
    void HandleRemoveComponents(UserConnection* source, const char* data, size_t numBytes);
    /// Handle remove entities message.
    void HandleRemoveEntity(UserConnection* source, const char* data, size_t numBytes);

    // @todo Implement for websocket
    //void ReplicateRigidBodyChanges(UserConnection* destination, SceneSyncState* state);

    /// Process one sync state for changes in the scene
    /** \todo For now, sends all changed entities/components. In the future, this shall be subject to interest management
        @param destination MessageConnection where to send the messages
        @param state Syncstate to process */
    void ProcessSyncState(UserConnection* destination, SceneSyncState* state);
    
    /// Validate the scene manipulation action. If returns false, it is ignored
    /** @param source Where the action came from
        @param messageID Network message id
        @param entityID What entity it affects */
    bool ValidateAction(UserConnection* source, unsigned messageID, entity_id_t entityID);
    
    /// Get a syncstate that matches the messageconnection, for reflecting arrived changes back
    /** For client, this will always be server_syncstate_. */
    SceneSyncState* GetSceneSyncState(UserConnection* connection);

    /// Owning module
    WebSocketServerModule* owner_;
    
    /// Framework pointer
    Framework* framework_;
    
    /// Scene pointer
    SceneWeakPtr scene_;
    
    /// Time period for update, default 1/30th of a second
    float updatePeriod_;
    /// Time accumulator for update
    float updateAcc_;
    
    /// Server sync state (client only)
    SceneSyncState server_syncstate_;
    
    /// Fixed buffers for crafting messages
    char createEntityBuffer_[64 * 1024];
    char createCompsBuffer_[64 * 1024];
    char editAttrsBuffer_[64 * 1024];
    char createAttrsBuffer_[16 * 1024];
    char attrDataBuffer_[16 * 1024];
    char removeCompsBuffer_[1024];
    char removeEntityBuffer_[1024];
    char removeAttrsBuffer_[1024];
    std::vector<u8> changedAttributes_;
};

}
