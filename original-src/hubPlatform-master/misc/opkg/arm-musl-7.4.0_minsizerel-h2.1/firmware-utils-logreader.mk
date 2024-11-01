define Package/firmware-utils-logreader
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-logreader
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-common-ubus_interface (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-common-ubus_interface
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-logreader/description
	Just firmware-utils-logreader package
endef

define Package/firmware-utils-logreader/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-logreader/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-logreader))
