// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_UiModule_InworldSceneController_h
#define incl_UiModule_InworldSceneController_h

#include "UiModuleApi.h"
#include "UiModuleFwd.h"

#include <QObject>
#include <QList>
#include <QRectF>
#include <QMap>

namespace Foundation
{
    class Framework;
}

namespace UiServices
{
    class UiWidgetProperties;

    class UI_MODULE_API InworldSceneController : public QObject
    {
        Q_OBJECT

    public:
        //! Constructor.
        //! \param framework Framework pointer.
        //! \param ui_view UI view for this scene manager.
        InworldSceneController(Foundation::Framework *framework, QGraphicsView *ui_view);

        //! Destructor.
        ~InworldSceneController();

    public slots:
        //! Adds a Qt Widget to the current scene, returns the added QGraphicsProxyWidget.
        /*! Convenience function if you dont want to bother and define your UiWidgetProperties.
         *
         *  \param  widget QWidget to be added to the scene.
         *  \return Proxy widget if succesfull, otherwise 0

         *  \note   QGraphicsProxyWidget maintains symmetry for the following states:
         *          state, enabled, visible, geometry, layoutDirection, style, palette,
         *          font, cursor, sizeHint, getContentsMargins and windowTitle. If you want to
         *          set some other property/attribute value for the proxy, do it for the proxy
         *          after calling this function.
         */
        QGraphicsProxyWidget *AddWidgetToScene(QWidget *widget);

        //! Adds a Qt proxy widget to the current scene.
        void AddWidgetToScene(QGraphicsProxyWidget *widget);

        //! Adds a Qt Widget to the current scene with Naali widget properties, returns the added QGraphicsProxyWidget
        /*! \param widget QWidget to be added to the scene.
         *  \param widget_properties Properties for the widget.
         *  \return UiProxyWidget if succesfull, otherwise 0
         */
        UiProxyWidget* AddWidgetToScene(QWidget *widget, const UiWidgetProperties &widget_properties);

        //! Adds a already created UiProxyWidget into the scene.
        /*! Please prefer using AddWidgetToScene() with normal QWidget and properties instead of this directly.
         *  \param widget Proxy widget.
        */
        bool AddProxyWidget(QGraphicsProxyWidget *widget);

        //! Remove a proxy widget from scene if it exist there
        /*! Used for removing your widget from scene. The show/hide toggle button will also be removed from the main panel.
         *  Note: Does not delete the proxy widget, after this is done its safe to delete your QWidget (this will delete the proxy also)
         *  \param widget Proxy widget.
        */
        void RemoveProxyWidgetFromScene(QGraphicsProxyWidget *widget);

        //! This is an overload function.
        //! \param widget Widget.
        void RemoveProxyWidgetFromScene(QWidget *widget);

        //! Brings the proxy widget to front in the scene, set focus to it and shows it.
        void BringProxyToFront(QGraphicsProxyWidget *widget) const;

        //! This is an overloaded function.
        //! \param widget Widget.
        void BringProxyToFront(QWidget *widget) const;

        //! Shows the UiProxyWidget of QWidget in the scene.
        //! \param widget Widget.
        void ShowProxyForWidget(QWidget *widget) const;

        //! Hides the UiProxyWidget of QWidget in the scene.
        //! \param widget Widget.
        void HideProxyForWidget(QWidget *widget) const;

        //! Adds a Qt Widget to the settings widget as its own tab
        /*! \param widget QWidget to be added to the settings widget
         *  \param tab_name QString name of the tab shown in widget
         *  \return trued if add succesfull, false otherwise
        */
        bool AddSettingsWidget(QWidget *settings_widget, const QString &tab_name) const;

        //! Get SettingsWidget QObject pointer to make save/cancel connections outside UiModule
        QObject *GetSettingsObject() const;

        //! Get the inworld ui scene
        QGraphicsScene *GetInworldScene() const { return inworld_scene_; }

        //! Please dont call this if you dont know what you are doing
        //! Set the im proxy to world chat widget for show/hide toggling
        void SetImWidget(UiProxyWidget *im_proxy) const;

        //! Set focus to chat line edit
        void SetFocusToChat() const;

        //! Get ControlPanelManager pointer
        CoreUi::ControlPanelManager *GetControlPanelManager()  const { return control_panel_manager_; }

        //! Applies new proxy position
        //! \param new_rect New scene rectangle.
        void ApplyNewProxyPosition(const QRectF &new_rect);

        //Slot triggered by UiProxyWidget ProxyMoved() signal.
        void ProxyWidgetMoved(QGraphicsProxyWidget* proxy_widget, const QPointF &proxy_pos);

        //Slot triggered by UiProxyWidget ProxyUngrabed() signal.
        void ProxyWidgetUngrabed(QGraphicsProxyWidget* proxy_widget, const QPointF &proxy_pos);

        //Slot triggered by UiProxyWidget Closed() or Visible() signals
        void ProxyClosed();

    private:
        Q_DISABLE_COPY(InworldSceneController);

        friend class UiSceneService;

        //! Pointer to main QGraphicsView
        QGraphicsView *ui_view_;

        //! Pointer to inworld widget scene
        QGraphicsScene *inworld_scene_;

        //! Layout manager
        CoreUi::AnchorLayoutManager *layout_manager_;

        //! Menu manager
        CoreUi::MenuManager *menu_manager_;

        //! Control panel manager
        CoreUi::ControlPanelManager *control_panel_manager_;

        //! Core Widgets
        CoreUi::CommunicationWidget *communication_widget_;

        //! Internal list of proxy widgets in scene.
        QList<QGraphicsProxyWidget *> all_proxy_widgets_in_scene_;

        //! Internal list of all docked proxy widgets.
        QList<QGraphicsProxyWidget *> all_docked_proxy_widgets_;

        //! QMap of docked UiProxyWidgets and their original size
        QMap<QGraphicsProxyWidget *, QSizeF> old_proxy_size;

        //! Framework pointer.
        Foundation::Framework *framework_;

        //Store last scene rectangle 
        QRectF last_scene_rect;

        //! Widget that flashes when a UiProxyWidget is dragged to the area specified for docking.
        QWidget *docking_widget_;

        //! QGraphicsProxyWidget from dock_w widget.
        QGraphicsProxyWidget *docking_widget_proxy_;

    private slots:
        //! Slot for applying new ui settings to all proxy widgets
        void ApplyNewProxySettings(int new_opacity, int new_animation_speed) const;

        //! Aligning widgets in the docking area
        void DockLineup();

        //! Deletes widget and the corresponding proxy widget if widget has WA_DeleteOnClose on.
        //! The caller of this slot is retrieved by using QObject::sender().
        void DeleteCallingWidgetOnClose();
    };
}

#endif // incl_UiModule_InworldSceneController_h
