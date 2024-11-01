define Package/firmware-common-bus
	$(call Package/hub-platform/Default)
	TITLE=firmware-common-bus
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libspread (=4.4.0-1),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +libc
	DEPENDS += +libgcc
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
