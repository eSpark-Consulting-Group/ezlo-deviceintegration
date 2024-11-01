define Package/firmware-core-update_manager
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-update_manager
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-daemon_module (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-http (=1.0),
	EXTRA_DEPENDS += firmware-common-json_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-extensions_manager_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-hardware_manager (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-daemon_module
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-http
	DEPENDS += +firmware-common-json_utils
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-plugin_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-extensions_manager_interface
	DEPENDS += +firmware-core-hardware_manager
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-update_manager/description
	Just firmware-core-update_manager package
endef

define Package/firmware-core-update_manager/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-update_manager/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-update_manager))
