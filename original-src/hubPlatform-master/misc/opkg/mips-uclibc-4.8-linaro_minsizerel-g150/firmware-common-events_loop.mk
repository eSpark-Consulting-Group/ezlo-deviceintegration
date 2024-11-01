define Package/firmware-common-events_loop
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-events_loop
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-events_loop/description
	Just firmware-common-events_loop package
endef

define Package/firmware-common-events_loop/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-events_loop/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-events_loop))
