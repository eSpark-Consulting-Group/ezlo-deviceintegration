define Package/firmware-common-config_reader
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-config_reader
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-config_reader/description
	Just firmware-common-config_reader package
endef

define Package/firmware-common-config_reader/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-config_reader/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-config_reader))
