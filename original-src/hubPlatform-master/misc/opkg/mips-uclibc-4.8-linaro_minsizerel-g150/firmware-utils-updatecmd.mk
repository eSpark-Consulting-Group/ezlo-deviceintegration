define Package/firmware-utils-updatecmd
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-updatecmd
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-platform_configuration (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-platform_configuration
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-updatecmd/description
	Just firmware-utils-updatecmd package
endef

define Package/firmware-utils-updatecmd/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-updatecmd/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-updatecmd))
