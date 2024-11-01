define Package/firmware-common-common_binding_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-common_binding_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_events_subscriber (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-plugin_events_subscriber
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +libc
	DEPENDS += +libpthread
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
