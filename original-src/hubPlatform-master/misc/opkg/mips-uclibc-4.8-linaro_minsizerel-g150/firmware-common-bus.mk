define Package/firmware-common-bus
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-bus
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += librt (=0.9.33.2-1),
	EXTRA_DEPENDS += libspread (=4.4.0-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +librt
	DEPENDS += +libspread
	DEPENDS += +libstdcpp
endef

define Package/firmware-common-bus/description
	Just firmware-common-bus package
endef

define Package/firmware-common-bus/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-common-bus/* $(1)/
endef

$(eval $(call BuildPackage,firmware-common-bus))
