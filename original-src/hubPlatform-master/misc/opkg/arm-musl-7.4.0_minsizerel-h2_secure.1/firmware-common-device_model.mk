define Package/firmware-common-device_model
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-device_model
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-device_model/description
	Just firmware-common-device_model package
endef

define Package/firmware-common-device_model/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-device_model/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-device_model))
