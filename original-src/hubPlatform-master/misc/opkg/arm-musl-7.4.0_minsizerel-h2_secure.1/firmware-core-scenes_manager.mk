define Package/firmware-core-scenes_manager
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-scenes_manager
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-daemon_module (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-http (=1.0),
	EXTRA_DEPENDS += firmware-common-hub_settings (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-lua_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-common-retry_command_manager (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-cloud_devices (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_group_operations (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_house_modes (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_model (=1.0),
	EXTRA_DEPENDS += firmware-core-extensions_manager_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-external_controllers_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-networkd (=1.0),
	EXTRA_DEPENDS += firmware-core-scenes_engine (=1.0),
	EXTRA_DEPENDS += firmware-core-uid_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-uid_response_builders (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libcurl (=7.58.0-1),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-daemon_module
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-http
	DEPENDS += +firmware-common-hub_settings
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-lua_binding_interface
	DEPENDS += +firmware-common-retry_command_manager
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-cloud_devices
	DEPENDS += +firmware-core-cored_group_operations
	DEPENDS += +firmware-core-cored_house_modes
	DEPENDS += +firmware-core-cored_interface
	DEPENDS += +firmware-core-cored_model
	DEPENDS += +firmware-core-extensions_manager_interface
	DEPENDS += +firmware-core-external_controllers_interface
	DEPENDS += +firmware-core-networkd
	DEPENDS += +firmware-core-scenes_engine
	DEPENDS += +firmware-core-uid_interface
	DEPENDS += +firmware-core-uid_response_builders
	DEPENDS += +libc
	DEPENDS += +libcurl
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-scenes_manager/description
	Just firmware-core-scenes_manager package
endef

define Package/firmware-core-scenes_manager/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-scenes_manager/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-scenes_manager))
