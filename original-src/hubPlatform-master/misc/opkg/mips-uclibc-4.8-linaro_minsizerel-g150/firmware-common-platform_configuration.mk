define Package/firmware-common-platform_configuration
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-platform_configuration
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-platform_configuration/description
	Just firmware-common-platform_configuration package
endef

define Package/firmware-common-platform_configuration/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-platform_configuration/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-platform_configuration))
