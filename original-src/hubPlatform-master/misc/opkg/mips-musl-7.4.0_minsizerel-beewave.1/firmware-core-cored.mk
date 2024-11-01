define Package/firmware-core-cored
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-cored
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-daemon_module (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-hub_settings (=1.0),
	EXTRA_DEPENDS += firmware-common-json_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-system_status (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_group_operations (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_house_modes (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_model (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_policies (=1.0),
	EXTRA_DEPENDS += firmware-core-update_manager_interface (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-daemon_module
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-hub_settings
	DEPENDS += +firmware-common-json_utils
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-system_status
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-cored_group_operations
	DEPENDS += +firmware-core-cored_house_modes
	DEPENDS += +firmware-core-cored_interface
	DEPENDS += +firmware-core-cored_model
	DEPENDS += +firmware-core-cored_policies
	DEPENDS += +firmware-core-update_manager_interface
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-cored/description
	Just firmware-core-cored package
endef

define Package/firmware-core-cored/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-cored/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-cored))
