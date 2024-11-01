define Package/firmware-common-hub_settings
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-hub_settings
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +librt
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
