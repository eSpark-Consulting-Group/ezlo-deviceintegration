define Package/firmware-utils-poweroff
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-poweroff
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-poweroff/description
	Just firmware-utils-poweroff package
endef

define Package/firmware-utils-poweroff/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-poweroff/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-poweroff))
