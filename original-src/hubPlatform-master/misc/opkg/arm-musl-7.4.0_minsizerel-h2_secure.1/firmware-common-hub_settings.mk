define Package/firmware-common-hub_settings
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-hub_settings
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-hub_settings/description
	Just firmware-common-hub_settings package
endef

define Package/firmware-common-hub_settings/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-hub_settings/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-hub_settings))
