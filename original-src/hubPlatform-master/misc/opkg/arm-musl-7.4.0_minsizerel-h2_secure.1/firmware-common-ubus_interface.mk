define Package/firmware-common-ubus_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-ubus_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-jsonc (=1.0),
	EXTRA_DEPENDS += firmware-common-libubus (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-jsonc
	DEPENDS += +firmware-common-libubus
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-ubus_interface/description
	Just firmware-common-ubus_interface package
endef

define Package/firmware-common-ubus_interface/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-ubus_interface/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-ubus_interface))
