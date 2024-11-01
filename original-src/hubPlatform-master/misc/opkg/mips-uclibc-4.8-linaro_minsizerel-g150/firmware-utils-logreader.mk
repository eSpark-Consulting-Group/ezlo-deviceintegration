define Package/firmware-utils-logreader
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-logreader
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libpthread
	DEPENDS += +librt
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
