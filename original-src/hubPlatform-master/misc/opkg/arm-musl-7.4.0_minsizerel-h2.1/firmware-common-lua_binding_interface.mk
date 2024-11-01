define Package/firmware-common-lua_binding_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-lua_binding_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-addon_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-common_binding_interface (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_events_subscriber (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += liblua (=5.3.5-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-addon_interfaces
	DEPENDS += +firmware-common-common_binding_interface
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-plugin_events_subscriber
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +liblua
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-lua_binding_interface/description
	Just firmware-common-lua_binding_interface package
endef

define Package/firmware-common-lua_binding_interface/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-lua_binding_interface/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-lua_binding_interface))
