define Package/hub-platform-firmware
	$(call Package/hub-platform/Default)
	TITLE=hub-platform-firmware
	VERSION=2.0.69
	EXTRA_DEPENDS += firmware-common-addon_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-cloud_storage (=1.0),
	EXTRA_DEPENDS += firmware-common-common_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-daemon_module (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-device_firmware_update_manager (=1.0),
	EXTRA_DEPENDS += firmware-common-device_model (=1.0),
	EXTRA_DEPENDS += firmware-common-ember_znet (=1.0),
	EXTRA_DEPENDS += firmware-common-events_loop (=1.0),
	EXTRA_DEPENDS += firmware-common-ffmpeg (=1.0),
	EXTRA_DEPENDS += firmware-common-git_version (=1.0),
	EXTRA_DEPENDS += firmware-common-http (=1.0),
	EXTRA_DEPENDS += firmware-common-http_bridge (=1.0),
	EXTRA_DEPENDS += firmware-common-hub_settings (=1.0),
	EXTRA_DEPENDS += firmware-common-json_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-lua_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-common-lua_storage (=1.0),
	EXTRA_DEPENDS += firmware-common-lua_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-platform_configuration (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_events_subscriber (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-service_metrics_data (=1.0),
	EXTRA_DEPENDS += firmware-common-smtp_client (=1.0),
	EXTRA_DEPENDS += firmware-common-ssl_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_status (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-common-uci_utils (=1.0),
	EXTRA_DEPENDS += firmware-core-backupd (=1.0),
	EXTRA_DEPENDS += firmware-core-cloud_devices (=1.0),
	EXTRA_DEPENDS += firmware-core-cored (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_group_operations (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_house_modes (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_model (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_policies (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_protect (=1.0),
	EXTRA_DEPENDS += firmware-core-extensions_manager (=1.0),
	EXTRA_DEPENDS += firmware-core-extensions_manager_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-external_controllers (=1.0),
	EXTRA_DEPENDS += firmware-core-external_controllers_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-hardware_manager (=1.0),
	EXTRA_DEPENDS += firmware-core-icc_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-lua_cored (=1.0),
	EXTRA_DEPENDS += firmware-core-lua_extensions_manager (=1.0),
	EXTRA_DEPENDS += firmware-core-lua_house_modes (=1.0),
	EXTRA_DEPENDS += firmware-core-networkd (=1.0),
	EXTRA_DEPENDS += firmware-core-notifications (=1.0),
	EXTRA_DEPENDS += firmware-core-scenes_engine (=1.0),
	EXTRA_DEPENDS += firmware-core-scenes_manager (=1.0),
	EXTRA_DEPENDS += firmware-core-uid (=1.0),
	EXTRA_DEPENDS += firmware-core-uid_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-uid_response_builders (=1.0),
	EXTRA_DEPENDS += firmware-core-update_manager (=1.0),
	EXTRA_DEPENDS += firmware-core-update_manager_interface (=1.0),
	EXTRA_DEPENDS += firmware-utils-at88util (=1.0),
	EXTRA_DEPENDS += firmware-utils-hardware_test (=1.0),
	EXTRA_DEPENDS += firmware-utils-infocmd (=1.0),
	EXTRA_DEPENDS += firmware-utils-ledcmd (=1.0),
	EXTRA_DEPENDS += firmware-utils-logcmd (=1.0),
	EXTRA_DEPENDS += firmware-utils-logreader (=1.0),
	EXTRA_DEPENDS += firmware-utils-migration_tool (=1.0),
	EXTRA_DEPENDS += firmware-utils-poweroff (=1.0),
	EXTRA_DEPENDS += firmware-utils-system_monitor (=1.0),
	EXTRA_DEPENDS += firmware-utils-updatecmd (=1.0),
	EXTRA_DEPENDS += firmware-utils-zwflash (=1.0),
	DEPENDS += +firmware-common-addon_interfaces
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-cloud_storage
	DEPENDS += +firmware-common-common_binding_interface
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-daemon_module
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-device_firmware_update_manager
	DEPENDS += +firmware-common-device_model
	DEPENDS += +firmware-common-ember_znet
	DEPENDS += +firmware-common-events_loop
	DEPENDS += +firmware-common-ffmpeg
	DEPENDS += +firmware-common-git_version
	DEPENDS += +firmware-common-http
	DEPENDS += +firmware-common-http_bridge
	DEPENDS += +firmware-common-hub_settings
	DEPENDS += +firmware-common-json_utils
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-lua_binding_interface
	DEPENDS += +firmware-common-lua_storage
	DEPENDS += +firmware-common-lua_utils
	DEPENDS += +firmware-common-platform_configuration
	DEPENDS += +firmware-common-plugin_events_subscriber
	DEPENDS += +firmware-common-plugin_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-service_metrics_data
	DEPENDS += +firmware-common-smtp_client
	DEPENDS += +firmware-common-ssl_utils
	DEPENDS += +firmware-common-system_status
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-common-uci_utils
	DEPENDS += +firmware-core-backupd
	DEPENDS += +firmware-core-cloud_devices
	DEPENDS += +firmware-core-cored
	DEPENDS += +firmware-core-cored_group_operations
	DEPENDS += +firmware-core-cored_house_modes
	DEPENDS += +firmware-core-cored_interface
	DEPENDS += +firmware-core-cored_model
	DEPENDS += +firmware-core-cored_policies
	DEPENDS += +firmware-core-cored_protect
	DEPENDS += +firmware-core-extensions_manager
	DEPENDS += +firmware-core-extensions_manager_interface
	DEPENDS += +firmware-core-external_controllers
	DEPENDS += +firmware-core-external_controllers_interface
	DEPENDS += +firmware-core-hardware_manager
	DEPENDS += +firmware-core-icc_interface
	DEPENDS += +firmware-core-lua_cored
	DEPENDS += +firmware-core-lua_extensions_manager
	DEPENDS += +firmware-core-lua_house_modes
	DEPENDS += +firmware-core-networkd
	DEPENDS += +firmware-core-notifications
	DEPENDS += +firmware-core-scenes_engine
	DEPENDS += +firmware-core-scenes_manager
	DEPENDS += +firmware-core-uid
	DEPENDS += +firmware-core-uid_interface
	DEPENDS += +firmware-core-uid_response_builders
	DEPENDS += +firmware-core-update_manager
	DEPENDS += +firmware-core-update_manager_interface
	DEPENDS += +firmware-utils-at88util
	DEPENDS += +firmware-utils-hardware_test
	DEPENDS += +firmware-utils-infocmd
	DEPENDS += +firmware-utils-ledcmd
	DEPENDS += +firmware-utils-logcmd
	DEPENDS += +firmware-utils-logreader
	DEPENDS += +firmware-utils-migration_tool
	DEPENDS += +firmware-utils-poweroff
	DEPENDS += +firmware-utils-system_monitor
	DEPENDS += +firmware-utils-updatecmd
	DEPENDS += +firmware-utils-zwflash
endef

define Package/hub-platform-firmware/description
	Just hub-platform-firmware package
endef

define Package/hub-platform-firmware/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/hub-platform-firmware/* $(1)/
endef

$(eval $(call BuildPackage,hub-platform-firmware))
