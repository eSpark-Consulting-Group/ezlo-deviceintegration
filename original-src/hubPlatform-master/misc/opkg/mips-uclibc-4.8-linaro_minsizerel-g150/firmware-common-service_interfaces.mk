define Package/firmware-common-service_interfaces
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-service_interfaces
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-hub_settings (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-plugin_events_subscriber (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libatomic (=4.8-linaro-1),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-hub_settings
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-plugin_events_subscriber
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libatomic
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-service_interfaces/description
	Just firmware-common-service_interfaces package
endef

define Package/firmware-common-service_interfaces/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-service_interfaces/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-service_interfaces))
