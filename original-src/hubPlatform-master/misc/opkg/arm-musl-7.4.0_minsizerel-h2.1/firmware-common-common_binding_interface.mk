define Package/firmware-common-common_binding_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-common_binding_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_events_subscriber (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-plugin_events_subscriber
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-common_binding_interface/description
	Just firmware-common-common_binding_interface package
endef

define Package/firmware-common-common_binding_interface/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-common_binding_interface/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-common_binding_interface))
